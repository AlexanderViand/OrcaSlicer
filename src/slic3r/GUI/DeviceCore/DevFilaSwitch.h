// DevFilaSwitch — ported from BambuStudio (bambu/master src/slic3r/GUI/DeviceCore/DevFilaSwitch.h)
//
// Represents the FilaSwitch (FS01) hardware on X2D-class printers: a small filament-track
// switcher that routes one of two AMS feeds to one of two extruders. Used by the AMS
// parser so AMS units reporting `info` with extruder_id=0xE (meaning "routed via this
// switch") are kept in the AmsList instead of being dropped as "uninitialized".
//
// This is a minimal subset of BambuLab's class. UI consumers (StatusPanel widgets,
// AMS mapping popups, calibration wizard) are deliberately *not* hooked up yet —
// the goal of this port is to fix the AMS parser. Surface-level accessors like
// GetInA_Slot()/GetOutA_Extruder() rely on MachineObject APIs (get_tray, etc.) that
// OrcaSlicer doesn't expose with matching signatures, so they're left as stubs returning
// nullopt. Resurfacing them requires either adding adapter methods on MachineObject or
// porting the consuming UI files in a later phase.

#pragma once
#include "DevDefs.h"

#include <optional>
#include <set>
#include <nlohmann/json.hpp>

namespace Slic3r
{
class MachineObject;
class DevExtder;
class DevAmsTray;

class DevFilaSwitch
{
public:
    enum class CaliStatus : int
    {
        CALI_IDLE = 0,
        CALI_STEPING = 1,
    };

    enum class CaliStep : int
    {
        CALI_IDLE = 0,
        CALI_SWITCHING = 1,
        CALI_SWITCH_CHECK = 2,
        CALI_FILA_CHECK = 3,
        CALI_FILA_TO_AMS = 4,
        CALI_FILA_TO_SWITCH = 5,
        CALI_FILA_BACK = 9,
        CALI_FINISHED = 14,
    };

    // Numeric values match the values in info bits 24..27 of the AMS `info` field
    // (see DevFilaSystem.cpp parser) and the values reported in `fila_switch.info`.
    enum SwitchPos : int
    {
        POS_IN_B = 0,
        POS_IN_A = 1,
    };

public:
    explicit DevFilaSwitch(MachineObject* owner);
    virtual ~DevFilaSwitch() = default;

    bool IsInstalled() const { return m_is_installed; }
    bool IsReady() const;

    std::optional<bool>         IsInA_HasFilament() const { return m_in_a_has_filament; }
    std::optional<bool>         IsInB_HasFilament() const { return m_in_b_has_filament; }

    std::optional<DevAmsSlotId> GetInA_SlotId() const { return m_in_a_slot; }
    std::optional<DevAmsSlotId> GetInB_SlotId() const { return m_in_b_slot; }

    std::optional<int>          GetOutA_ExtruderId() const { return m_out_a_extruder_id; }
    std::optional<int>          GetOutB_ExtruderId() const { return m_out_b_extruder_id; }

    CaliStatus GetCaliStatus() const { return m_cali_status; }

    void Reset();
    void ParseFilaSwitchInfo(const nlohmann::json& print_jj);

private:
    MachineObject* m_owner = nullptr;

    bool m_is_installed = false;

    std::optional<bool> m_in_a_has_filament;
    std::optional<bool> m_in_b_has_filament;

    std::optional<DevAmsSlotId> m_in_a_slot;
    std::optional<DevAmsSlotId> m_in_b_slot;

    std::optional<int> m_out_a_extruder_id;
    std::optional<int> m_out_b_extruder_id;

    CaliStatus m_cali_status = CaliStatus::CALI_IDLE;
};

} // namespace Slic3r
