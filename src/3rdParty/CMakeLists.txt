add_subdirectory(FastSignals)

# Build SalomeMesh for all Platforms since heavily patched
if (BUILD_SMESH)
    add_subdirectory(salomesmesh)
endif()

add_subdirectory(lazy_loader)

if(NOT FREECAD_USE_EXTERNAL_E57FORMAT)
    add_subdirectory(libE57Format)
endif()

if(BUILD_MATERIAL_EXTERNAL)
    add_subdirectory(lru-cache)
endif()

if (BUILD_ASSEMBLY AND NOT FREECAD_USE_EXTERNAL_ONDSELSOLVER)
    if( NOT EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/OndselSolver/CMakeLists.txt" )
        message(FATAL_ERROR "The OndselSolver git submodule is not available. Please run
        git submodule update --init" )
    endif()
    add_subdirectory(OndselSolver)
endif()
if(BUILD_PDF_EXPORT_3D AND NOT FREECAD_USE_EXTERNAL_LIBPRC)
    if( NOT EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/libPRC/CMakeLists.txt" )
        message(FATAL_ERROR "The libPRC git submodule is not available. Please run
        git submodule update --init" )
    endif()
    add_subdirectory(libPRC)
endif()

if(BUILD_PDF_EXPORT_3D AND NOT FREECAD_USE_EXTERNAL_LIBHARU)
    if( NOT EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/libharu/CMakeLists.txt" )
        message(FATAL_ERROR "The libharu git submodule is not available. Please run
        git submodule update --init" )
    endif()
    add_subdirectory(libharu)
endif()

if (BUILD_TRACY_FRAME_PROFILER)
    if( NOT EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/tracy/CMakeLists.txt" )
        message(FATAL_ERROR "The Tracy git directory is not available. Please clone it manually." )
    endif()

    set(TRACY_STATIC OFF)
    add_subdirectory(tracy)
endif()
