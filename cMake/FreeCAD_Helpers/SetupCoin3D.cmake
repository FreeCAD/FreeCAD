macro(SetupCoin3D)
    # -------------------------------- Coin3D --------------------------------

    if (WIN32 AND MINGW)
        find_path(COIN3D_INCLUDE_DIRS Inventor/So.h)
        find_library(COIN3D_LIBRARIES Coin)
    endif ()

    # Try CONFIG mode -- Coin supports very old CMake files, which emits noisy warnings. Silence them.
    set(CMAKE_WARN_DEPRECATED_OLD_STATE ${CMAKE_WARN_DEPRECATED})
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    find_package(Coin CONFIG)
    set(CMAKE_WARN_DEPRECATED ${CMAKE_WARN_DEPRECATED_OLD_STATE} CACHE BOOL "" FORCE)
    if (Coin_FOUND)
        set(COIN3D_INCLUDE_DIRS ${Coin_INCLUDE_DIR})
        set(COIN3D_LIBRARIES ${Coin_LIBRARIES})
        set(COIN3D_LIB_DIRS ${Coin_LIB_DIR})
    else ()
        # Try MODULE mode (FindCoin3D.cmake, included by CMake)
        find_package(Coin3D)
        if (NOT COIN3D_FOUND)
            message(FATAL_ERROR "Could not find Coin3D")
        endif ()
    endif ()

    IF (NOT COIN3D_VERSION)
        file(READ "${COIN3D_INCLUDE_DIRS}/Inventor/C/basic.h" _coin3d_basic_h)
        string(REGEX MATCH "define[ \t]+COIN_MAJOR_VERSION[ \t]+([0-9?])" _coin3d_major_version_match "${_coin3d_basic_h}")
        set(COIN3D_MAJOR_VERSION "${CMAKE_MATCH_1}")
        string(REGEX MATCH "define[ \t]+COIN_MINOR_VERSION[ \t]+([0-9?])" _coin3d_minor_version_match "${_coin3d_basic_h}")
        set(COIN3D_MINOR_VERSION "${CMAKE_MATCH_1}")
        string(REGEX MATCH "define[ \t]+COIN_MICRO_VERSION[ \t]+([0-9?]+)" _coin3d_micro_version_match "${_coin3d_basic_h}")
        set(COIN3D_MICRO_VERSION "${CMAKE_MATCH_1}")
        set(COIN3D_VERSION "${COIN3D_MAJOR_VERSION}.${COIN3D_MINOR_VERSION}.${COIN3D_MICRO_VERSION}")
    ENDIF ()
endmacro(SetupCoin3D)

macro(SetupPivy)
    # -------------------------------- Pivy --------------------------------

    IF (FREECAD_CHECK_PIVY) # do not make pivy a host dependency for cross compiling
        IF (NOT PIVY_VERSION)
            message(STATUS "Checking Pivy version by importing it in a Python program...")
            execute_process(
                    COMMAND ${Python3_EXECUTABLE} -c "import pivy as p; print(p.__version__,end='')"
                    OUTPUT_VARIABLE PIVY_VERSION
                    RESULT_VARIABLE RETURN_CODE)
            if (RETURN_CODE EQUAL 0)
                message(STATUS "Found Pivy ${PIVY_VERSION}")
            else ()
                message(ERROR "Failed to import Pivy using ${Python3_EXECUTABLE}")
            endif ()
        ENDIF ()

        message(STATUS "Checking Pivy Coin3D version by importing it in a Python program...")
        execute_process(
                COMMAND ${Python3_EXECUTABLE} -c "import pivy as p; print(p.SoDB.getVersion(),end='')"
                OUTPUT_VARIABLE PIVY_COIN_VERSION
                RESULT_VARIABLE RETURN_CODE)
        if (RETURN_CODE EQUAL 0)
            message(STATUS "Found Pivy Coin3D ${PIVY_COIN_VERSION}")
        else ()
            message(ERROR "Failed to get Pivy Coin3D version using ${Python3_EXECUTABLE}")
        endif ()

        if (${PIVY_COIN_VERSION} MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
            set(PIVY_COIN_MAJOR_VERSION ${CMAKE_MATCH_1})
            set(PIVY_COIN_MINOR_VERSION ${CMAKE_MATCH_2})
            set(PIVY_COIN_MICRO_VERSION ${CMAKE_MATCH_3})
            set(PIVY_COIN_VERSION "${PIVY_COIN_MAJOR_VERSION}.${PIVY_COIN_MINOR_VERSION}.${PIVY_COIN_MICRO_VERSION}")
        else ()
            message(FATAL_ERROR "Failed to match Pivy Coin3D version string output")
        endif ()

        if (NOT (
            (${COIN3D_MAJOR_VERSION} EQUAL ${PIVY_COIN_MAJOR_VERSION}) AND
            (${COIN3D_MINOR_VERSION} EQUAL ${PIVY_COIN_MINOR_VERSION}) AND
            (${COIN3D_MICRO_VERSION} EQUAL ${PIVY_COIN_MICRO_VERSION})))
            message(FATAL_ERROR "Coin3D version ${COIN3D_VERSION} mismatches Pivy Coin3D ${PIVY_COIN_VERSION}.")
        endif ()
    endif(FREECAD_CHECK_PIVY)
endmacro(SetupPivy)

macro(SetupBundledCoinPivy)
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/coin/CMakeLists.txt"
        OR NOT EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/pivy/CMakeLists.txt")
        message(FATAL_ERROR
            "Bundled Coin/Pivy git submodules are not available. "
            "Please run:\n"
            "  git submodule update --init --recursive"
        )
    endif ()

    find_package(EXPAT QUIET)
    if (EXPAT_FOUND)
        if (DEFINED EXPAT_VERSION_STRING AND NOT EXPAT_VERSION_STRING STREQUAL "")
            set(FREECAD_COIN_EXPAT_SOURCE "external (${EXPAT_VERSION_STRING})")
            set(_freecad_coin_expat_message "Bundled Coin will use system Expat ${EXPAT_VERSION_STRING}")
        else ()
            set(FREECAD_COIN_EXPAT_SOURCE "external")
            set(_freecad_coin_expat_message "Bundled Coin will use system Expat")
        endif ()
        message(STATUS "${_freecad_coin_expat_message}")
    else ()
        set(FREECAD_COIN_EXPAT_SOURCE "bundled fallback")
        message(WARNING
            "Bundled Coin could not find a system Expat. "
            "Coin will fall back to its vendored Expat sources. "
            "This can fail on some platforms; install Expat development files "
            "or make Expat discoverable to CMake."
        )
    endif ()

    set(USE_EXTERNAL_EXPAT ON CACHE BOOL "Use system Expat for bundled Coin" FORCE)
    set(FREETYPE_RUNTIME_LINKING OFF CACHE BOOL "Disable FreeType runtime linking for bundled Coin" FORCE)
    set(COIN_BUILD_TESTS OFF CACHE BOOL "Build bundled Coin tests" FORCE)
    set(COIN_INSTALL OFF CACHE BOOL "Disable standalone install rules for bundled Coin" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/src/3rdParty/coin" "${CMAKE_BINARY_DIR}/src/3rdParty/coin")
    if (NOT DEFINED COIN_VERSION OR COIN_VERSION STREQUAL "")
        message(FATAL_ERROR "Bundled Coin did not define COIN_VERSION")
    endif ()
    set(COIN3D_VERSION "${COIN_VERSION}")
    # Match external Coin usage: FreeCAD targets should treat Coin headers as third-party headers.
    target_include_directories(Coin
        SYSTEM INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/3rdParty/coin/include>"
        "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src/3rdParty/coin/include>"
    )
    if (UNIX AND NOT APPLE)
        # Coin directly loads sibling libraries such as libEGL.  The RUNPATH of
        # a library that loads Coin is not used to resolve Coin's dependencies.
        # Keep the bundled library relocatable in both Pixi and AppImage installs.
        set_property(TARGET Coin PROPERTY INSTALL_RPATH "$ORIGIN")
    endif ()
    install(TARGETS Coin
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_SKIP
    )

    set(PIVY_USE_SOQT OFF CACHE BOOL "Build bundled Pivy SoQt bindings" FORCE)
    set(PIVY_PACKAGE_INSTALL_DIR "Mod/pivy"
        CACHE STRING "Install directory for bundled Pivy inside FreeCAD packages" FORCE)
    if (APPLE)
        set(PIVY_MODULE_INSTALL_RPATH "@loader_path/../../${CMAKE_INSTALL_LIBDIR}"
            CACHE STRING "RUNPATH for bundled Pivy extension modules in FreeCAD packages" FORCE)
    elseif (UNIX)
        set(PIVY_MODULE_INSTALL_RPATH "$ORIGIN/../../${CMAKE_INSTALL_LIBDIR}"
            CACHE STRING "RUNPATH for bundled Pivy extension modules in FreeCAD packages" FORCE)
    endif ()
    set(PIVY_PACKAGE_OUTPUT_DIR "${PROJECT_BINARY_DIR}/Mod/pivy"
        CACHE PATH "Build-tree output directory for bundled Pivy" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/src/3rdParty/pivy" "${CMAKE_BINARY_DIR}/src/3rdParty/pivy")
    set_property(TARGET coin PROPERTY INSTALL_REMOVE_ENVIRONMENT_RPATH TRUE)
    if (NOT DEFINED PIVY_VERSION OR PIVY_VERSION STREQUAL "")
        message(FATAL_ERROR "Bundled Pivy did not define PIVY_VERSION")
    endif ()
endmacro(SetupBundledCoinPivy)

macro(SetupCoinPivy)
    if (FREECAD_USE_EXTERNAL_COIN_PIVY)
        set(FREECAD_COIN3D_SOURCE "external")
        set(FREECAD_PIVY_SOURCE "external")
        SetupCoin3D()
        SetupPivy()
    else ()
        set(FREECAD_COIN3D_SOURCE "bundled")
        set(FREECAD_PIVY_SOURCE "bundled")
        SetupBundledCoinPivy()
    endif ()
endmacro(SetupCoinPivy)
