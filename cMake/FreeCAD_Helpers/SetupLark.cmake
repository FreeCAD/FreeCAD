macro(SetupLark)
    # ------------------------------ Lark ------------------------------

    if(BUILD_BIM)
        find_package(LARK MODULE REQUIRED)
    endif()

endmacro()
