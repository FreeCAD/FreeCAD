macro(SetupCoin3D)
# -------------------------------- Coin3D --------------------------------

    find_package(Coin3D REQUIRED)
    if(NOT COIN3D_FOUND)
        message(FATAL_ERROR "=================\n"
                            "Coin3D not found.\n"
                            "=================\n")
    endif(NOT COIN3D_FOUND)

endmacro(SetupCoin3D)
