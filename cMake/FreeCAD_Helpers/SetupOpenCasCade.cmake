macro(SetupOpenCasCade)
    find_package(OCC)
    if(NOT OCC_FOUND)
        message(FATAL_ERROR "================================================================\n"
                            "OpenCASCADE not found!\n"
                            "================================================================\n")
    endif()
endmacro()
