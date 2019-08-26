macro(SetupMatplotlib)
# ------------------------------ Matplotlib ------------------------------

    find_package(Matplotlib)
    if (MATPLOTLIB_FOUND)
        message(STATUS "-- matplotlib-${MATPLOTLIB_VERSION} has been found.")
    else(MATPLOTLIB_FOUND)
        message("=====================================================\n"
                "matplotlib not found, Plot module won't be available.\n"
                "=====================================================\n")
    endif(MATPLOTLIB_FOUND)

endmacro(SetupMatplotlib)
