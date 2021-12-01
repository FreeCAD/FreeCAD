macro(SetupDoxygen)
# -------------------------------- Doxygen ----------------------------------

    find_package(Doxygen)

    if (NOT DOXYGEN_FOUND)
        message("=====================================================\n"
                "Doxygen not found, will not build documentation.     \n"
                "=====================================================\n")
    endif(NOT DOXYGEN_FOUND)

endmacro(SetupDoxygen)
