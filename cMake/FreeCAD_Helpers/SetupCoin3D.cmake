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
        string(REGEX MATCH "define[ \t]+COIN_MICRO_VERSION[ \t]+([0-9?])" _coin3d_micro_version_match "${_coin3d_basic_h}")
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
