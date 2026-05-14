// bbl_bypass_shim.dylib
//
// Make BBL's libbambu_networking.dylib think it's running inside BambuStudio.app
// (the signing-info check uses _NSGetExecutablePath() to locate the host binary,
// then SecStaticCodeCreateWithPath/SecCodeCopySigningInformation to validate its
// code signature against BambuLab's Developer ID).
//
// Load via DYLD_INSERT_LIBRARIES. Only redirects when the call comes from within
// libbambu_networking.dylib itself; every other caller sees the real result.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>

#define DYLD_INTERPOSE(_replacement, _replacee)                                                 \
    __attribute__((used)) static struct {                                                       \
        const void* replacement;                                                                \
        const void* replacee;                                                                   \
    } _interpose_##_replacee __attribute__((section("__DATA,__interpose"))) =                   \
        {(const void*)(unsigned long)&_replacement, (const void*)(unsigned long)&_replacee};

// The host binary we want BBL's plugin to see. Must be installed on disk with a
// valid BambuLab Developer ID signature.
static const char kBblHostPath[] = "/Applications/BambuStudio.app/Contents/MacOS/BambuStudio";

// Match callers from libbambu_networking.dylib (handles both the OrcaSlicer-served
// libbambu_networking_*.dylib variant and the canonical libbambu_networking.dylib).
static int caller_is_bbl_plugin(void* return_addr) {
    Dl_info info;
    if (dladdr(return_addr, &info) == 0) return 0;
    if (!info.dli_fname) return 0;
    return strstr(info.dli_fname, "libbambu_networking") != NULL;
}

static int shim_NSGetExecutablePath(char* buf, uint32_t* bufsize) {
    if (caller_is_bbl_plugin(__builtin_return_address(0))) {
        const size_t need = sizeof(kBblHostPath);  // includes trailing NUL
        if (!buf || !bufsize || *bufsize < need) {
            if (bufsize) *bufsize = (uint32_t)need;
            return -1;
        }
        memcpy(buf, kBblHostPath, need);
        *bufsize = (uint32_t)need;

        // Visible breadcrumb the first time this fires.
        static int once = 0;
        if (!once) {
            once = 1;
            fprintf(stderr, "[bbl_bypass] redirected _NSGetExecutablePath -> %s\n", kBblHostPath);
        }
        return 0;
    }
    return _NSGetExecutablePath(buf, bufsize);
}

DYLD_INTERPOSE(shim_NSGetExecutablePath, _NSGetExecutablePath);
