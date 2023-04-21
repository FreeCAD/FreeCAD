macro(SetupOpenCasCade)
# -------------------------------- OpenCasCade --------------------------------

    find_package(OCC)
    if(NOT OCC_FOUND)
        message(FATAL_ERROR "================================================================\n"
                            "Neither OpenCASCADE Community Edition nor OpenCASCADE was found!\n"
                            "================================================================\n")
    endif(NOT OCC_FOUND)

endmacro(SetupOpenCasCade)
