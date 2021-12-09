macro(SetupSwig)
# -------------------------------- Swig ----------------------------------

    find_package(SWIG)

    if (NOT SWIG_FOUND)
        message("=====================================================\n"
                "SWIG not found, will not build SWIG binding for pivy.\n"
                "=====================================================\n")
    endif(NOT SWIG_FOUND)

endmacro(SetupSwig)
