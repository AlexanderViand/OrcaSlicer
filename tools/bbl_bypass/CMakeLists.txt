# Builds libbbl_bypass — a tiny interposing dylib that makes Bambu's networking
# plugin think it's running inside BambuStudio.app for signature-verification
# purposes. See shim.c for the rationale.
#
# Macros recap: macOS's `__DATA,__interpose` section is honored as soon as the
# containing dylib is loaded by dyld, regardless of how it was loaded. So
# linking this dylib into OrcaSlicer at build time has the same effect as
# loading it via DYLD_INSERT_LIBRARIES, with the major advantage that no
# environment variable is required to launch the app from /Applications.
#
# Apple builds only — the BBL plugin signature gate only exists on macOS.

if(NOT APPLE)
    return()
endif()

add_library(bbl_bypass SHARED shim.c)

set_target_properties(bbl_bypass PROPERTIES
    OUTPUT_NAME "bbl_bypass"
    # Universal binary so the same OrcaSlicer.app works on both Apple Silicon
    # and Intel hosts, matching the BBL plugin's universal build.
    OSX_ARCHITECTURES "arm64;x86_64"
    # The dylib ships alongside the main binary in Contents/MacOS/, so the
    # install_name needs to resolve relative to the executable. We want the
    # build-tree dylib (not just an install-step copy) to have this name,
    # so set both INSTALL_NAME_DIR and BUILD_WITH_INSTALL_NAME_DIR.
    INSTALL_NAME_DIR "@executable_path"
    BUILD_WITH_INSTALL_NAME_DIR TRUE
)

# Keep the dylib's interpose section intact through the linker. Without this
# the linker may discard the unreferenced __interpose entries even though
# they have __attribute__((used)).
target_link_options(bbl_bypass PRIVATE "-Wl,-no_dead_strip_inits_and_terms")

# Place the build output directly in OrcaSlicer.app/Contents/MacOS/ so it
# travels with the binary by default (no separate install rule needed during
# development; CPack/packaging step still handles distribution).
