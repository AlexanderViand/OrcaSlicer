// DevFilaSwitch — ported from BambuStudio (bambu/master src/slic3r/GUI/DeviceCore/DevFilaSwitch.cpp)
//
// See DevFilaSwitch.h for the rationale. This implementation tracks the parsed state and
// exposes IsInstalled()/IsReady()/slot+extruder lookups for the parser; UI consumer methods
// (GetInA_Slot etc.) that would need OrcaSlicer-specific MachineObject adapters are left as
// stubs returning std::nullopt and can be wired up later.

#include "DevFilaSwitch.h"

#include "DevFilaSystem.h"
#include "DevUtil.h"
#include "slic3r/GUI/DeviceManager.hpp"

#include <boost/log/trivial.hpp>

namespace Slic3r {

DevFilaSwitch::DevFilaSwitch(MachineObject* owner) : m_owner(owner) {}

void DevFilaSwitch::Reset()
{
    m_is_installed = false;
    m_in_a_has_filament.reset();
    m_in_b_has_filament.reset();
    m_in_a_slot.reset();
    m_in_b_slot.reset();
    m_out_a_extruder_id.reset();
    m_out_b_extruder_id.reset();
    m_cali_status = CaliStatus::CALI_IDLE;
}

bool DevFilaSwitch::IsReady() const
{
    if (!m_is_installed) {
        BOOST_LOG_TRIVIAL(debug) << "[FilaSwitch] IsReady: not installed";
        return false;
    }

    if (!m_owner) return false;
    auto* fila_system = m_owner->GetFilaSystem();
    if (!fila_system) return false;

    const auto& ams_list = fila_system->GetAmsList();
    for (const auto& ams_item : ams_list) {
        if (!ams_item.second) continue;
        if (ams_item.second->GetBindedExtruderSet().empty()) return false;
        if (!ams_item.second->GetSwitcherPos().has_value()) return false;
    }

    return true;
}

void DevFilaSwitch::ParseFilaSwitchInfo(const nlohmann::json& print_jj)
{
    // `aux` is a 32-bit-packed status word in newer Bambu pushes. Bit 29 of its hex string
    // value indicates the FilaSwitch is physically installed on the printer.
    if (print_jj.contains("aux")) {
        try {
            const auto& info_bits = print_jj["aux"].get<std::string>();
            const bool installed_now = (DevUtil::get_flag_bits(info_bits, 29, 1) == 1);
            if (m_is_installed != installed_now) {
                m_is_installed = installed_now;
                if (!m_is_installed) Reset();
            }
        } catch (...) {
            ; // bad format, ignore
        }
    }

    if (!print_jj.contains("device") || !print_jj["device"].contains("fila_switch")) return;
    const auto& fila_switch_jj = print_jj["device"]["fila_switch"];

    // Inputs: two AMS slot references (each a packed int with ams_id in bits 8..15 and slot_id
    // in bits 0..7). -1 means "no source attached on this side."
    if (fila_switch_jj.contains("in") && fila_switch_jj["in"].is_array()) {
        try {
            const auto& in_vec = fila_switch_jj["in"].get<std::vector<int>>();
            if (in_vec.size() == 2) {
                if (in_vec[0] != -1) {
                    DevAmsSlotId slot_id;
                    slot_id.first  = DevUtil::get_flag_bits(in_vec[0], 8, 8);
                    slot_id.second = DevUtil::get_flag_bits(in_vec[0], 0, 8);
                    m_in_b_slot    = slot_id;
                } else {
                    m_in_b_slot.reset();
                }

                if (in_vec[1] != -1) {
                    DevAmsSlotId slot_id;
                    slot_id.first  = DevUtil::get_flag_bits(in_vec[1], 8, 8);
                    slot_id.second = DevUtil::get_flag_bits(in_vec[1], 0, 8);
                    m_in_a_slot    = slot_id;
                } else {
                    m_in_a_slot.reset();
                }
            }
        } catch (...) {
            ;
        }
    }

    // Outputs: two extruder ids. 0xE means "not currently routing to that side."
    if (fila_switch_jj.contains("out") && fila_switch_jj["out"].is_array()) {
        try {
            const auto& out_vec = fila_switch_jj["out"].get<std::vector<int>>();
            if (out_vec.size() == 2) {
                if (out_vec[0] != 0xE) {
                    m_out_b_extruder_id = out_vec[0];
                } else {
                    m_out_b_extruder_id.reset();
                }

                if (out_vec[1] != 0xE) {
                    m_out_a_extruder_id = out_vec[1];
                } else {
                    m_out_a_extruder_id.reset();
                }
            }
        } catch (...) {
            ;
        }
    }

    if (fila_switch_jj.contains("stat")) {
        try {
            m_cali_status = static_cast<CaliStatus>(fila_switch_jj["stat"].get<int>());
        } catch (...) {
            ;
        }
    }

    if (fila_switch_jj.contains("info")) {
        try {
            const int info_bits = fila_switch_jj["info"].get<int>();
            m_in_b_has_filament = (DevUtil::get_flag_bits(info_bits, 0, 1) != 0);
            m_in_a_has_filament = (DevUtil::get_flag_bits(info_bits, 0, 1) != 0);
        } catch (...) {
            ;
        }
    }
}

} // namespace Slic3r
