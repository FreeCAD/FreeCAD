macro(SetupCoin3D)
# -------------------------------- Coin3D --------------------------------

    if (WIN32 AND MINGW)
        find_path(COIN3D_INCLUDE_DIRS Inventor/So.h)
        find_library(COIN3D_LIBRARIES Coin)
    endif ()

    find_package(Coin3D REQUIRED)
    if(NOT COIN3D_FOUND)
        message(FATAL_ERROR "=================\n"
                            "Coin3D not found.\n"
                            "=================\n")
    endif(NOT COIN3D_FOUND)

    IF(NOT PIVY_VERSION)
      execute_process (COMMAND ${Python3_EXECUTABLE} -c "import pivy as p; print(p.__version__,end='')" OUTPUT_VARIABLE PIVY_VERSION)
    ENDIF()

endmacro(SetupCoin3D)
