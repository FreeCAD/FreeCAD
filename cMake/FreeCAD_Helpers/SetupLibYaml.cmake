macro(SetupYamlCpp)
# -------------------------------- YamlCpp --------------------------------

    find_package(yaml-cpp REQUIRED)
    if(NOT yaml-cpp_FOUND)
        message(FATAL_ERROR "==================\n"
                            "YamlCpp not found.\n"
                            "==================\n")
    endif(NOT yaml-cpp_FOUND)

endmacro(SetupYamlCpp)
