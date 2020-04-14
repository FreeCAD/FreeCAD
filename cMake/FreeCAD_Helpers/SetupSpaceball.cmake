macro(SetupSpaceball)
# ------------------------------ Spaceball -------------------------------

    if (WIN32)
        #future
    else(WIN32)
        find_package(Spnav)
    endif(WIN32)

endmacro(SetupSpaceball)
