# Build SalomeMesh for all Platforms since heavily patched
if (BUILD_SMESH AND NOT FREECAD_USE_EXTERNAL_SMESH)
    add_subdirectory(salomesmesh)
endif()

if (BUILD_BGFX)
    set(BGFX_LIBRARY_TYPE "SHARED")
    add_subdirectory(bgfx)
endif()

if (BUILD_DILIGENT)
    add_subdirectory(Diligent/DiligentCore)
    add_subdirectory(Diligent/DiligentFX)
    add_subdirectory(Diligent/DiligentTools)
endif()

add_subdirectory(lazy_loader)
