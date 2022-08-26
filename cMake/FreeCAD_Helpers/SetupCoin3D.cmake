macro(SetupCoin3D)
# -------------------------------- Coin3D --------------------------------

    if (WIN32 AND MINGW)
        find_path(COIN3D_INCLUDE_DIRS Inventor/So.h)
        find_library(COIN3D_LIBRARIES Coin)
    endif ()

    # Try MODULE mode
    find_package(Coin3D)
    if(NOT COIN3D_FOUND)
        # Try CONFIG mode
        find_package(Coin CONFIG REQUIRED)
        if (Coin_FOUND)
            set (COIN3D_INCLUDE_DIRS ${Coin_INCLUDE_DIR})
            set (COIN3D_LIBRARIES ${Coin_LIBRARIES})
        else()
            message(FATAL_ERROR "=================\n"
                                "Coin3D not found.\n"
                                "=================\n")
        endif()
    endif(NOT COIN3D_FOUND)

    IF(NOT COIN3D_VERSION)
      file(READ "${COIN3D_INCLUDE_DIRS}/Inventor/C/basic.h" _coin3d_basic_h)
      string(REGEX MATCH "define[ \t]+COIN_MAJOR_VERSION[ \t]+([0-9?])" _coin3d_major_version_match "${_coin3d_basic_h}")
      set(COIN3D_MAJOR_VERSION "${CMAKE_MATCH_1}")
      string(REGEX MATCH "define[ \t]+COIN_MINOR_VERSION[ \t]+([0-9?])" _coin3d_minor_version_match "${_coin3d_basic_h}")
      set(COIN3D_MINOR_VERSION "${CMAKE_MATCH_1}")
      string(REGEX MATCH "define[ \t]+COIN_MICRO_VERSION[ \t]+([0-9?])" _coin3d_micro_version_match "${_coin3d_basic_h}")
      set(COIN3D_MICRO_VERSION "${CMAKE_MATCH_1}")
      set(COIN3D_VERSION "${COIN3D_MAJOR_VERSION}.${COIN3D_MINOR_VERSION}.${COIN3D_MICRO_VERSION}")
    ENDIF()

    IF(NOT PIVY_VERSION)
      execute_process (COMMAND ${Python3_EXECUTABLE} -c "import pivy as p; print(p.__version__,end='')" OUTPUT_VARIABLE PIVY_VERSION)
    ENDIF()

endmacro(SetupCoin3D)
