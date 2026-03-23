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

if (BUILD_TRACY_FRAME_PROFILER)
    if( NOT EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/tracy/CMakeLists.txt" )
        message(FATAL_ERROR "The Tracy git directory is not available. Please clone it manually." )
    endif()

    set(TRACY_STATIC OFF)
    add_subdirectory(tracy)
endif()

if (NOT FREECAD_USE_EXTERNAL_COIN_PIVY)
    set(USE_EXTERNAL_EXPAT ON)
    set(FREETYPE_RUNTIME_LINKING OFF)
    set(PIVY_USE_QT6 ON)
    add_subdirectory(coin)
    add_subdirectory(pivy)

    # TODO: Currently these commands run everytime because the target coin does the
    # same This is not very ideal for continuous development and should be improved

    # Copy the pivy folder from build to build/Mod so that it can be picked by
    # python as adding the build directory to pythonpath might not be wise
    # Can we use some other/new directory for this?
    add_custom_target(CopyPivyFiles ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_BINARY_DIR}/pivy
            ${PROJECT_BINARY_DIR}/Mod/pivy

        DEPENDS coin
    )

    # The cmake target pivy uses to generate the bindings is called "coin"
    # While the target coin3d itself uses is called "Coin"
    get_target_property(Pivy_BINARY_DIR coin BINARY_DIR)
    if (MSVC)
        set(COIN_LIB _coin.dll)
    else()
        set(COIN_LIB _coin.so)
    endif()
    add_custom_target(CopyCoinShim ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${Pivy_BINARY_DIR}/${COIN_LIB}
            ${PROJECT_BINARY_DIR}/Mod/pivy/

        DEPENDS CopyPivyFiles
    )
endif()
