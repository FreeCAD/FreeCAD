macro(SetupCoin3D)
# -------------------------------- Coin3D --------------------------------

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
