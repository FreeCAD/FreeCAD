macro(SetupXercesC)
# -------------------------------- XercesC --------------------------------

    find_package(XercesC REQUIRED)
    if(NOT XercesC_FOUND)
        message(FATAL_ERROR "==================\n"
                            "XercesC not found.\n"
                            "==================\n")
    endif(NOT XercesC_FOUND)

endmacro(SetupXercesC)
