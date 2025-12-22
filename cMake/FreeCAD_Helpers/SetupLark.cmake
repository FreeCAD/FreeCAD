macro(SetupLark)
    # ------------------------------ Lark ------------------------------

    find_package(LARK)
    if(LARK_FOUND)
        message(STATUS "Found Lark: version ${LARK_VERSION}")
    else(LARK_FOUND)
        message(FATAL_ERROR
            "\nPython module 'lark' not found but is required for the BIM workbench.\n"
            "Install it using your ${Python3_EXECUTABLE} package manager ie. pip\n"
            "Or on Ubuntu/Debian: apt install python3-lark\n"
        )
    endif()

endmacro()
