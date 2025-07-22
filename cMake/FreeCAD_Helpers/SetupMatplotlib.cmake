macro(SetupMatplotlib)
# ------------------------------ Matplotlib ------------------------------

    find_package(Matplotlib)
    if (MATPLOTLIB_FOUND)
        message(STATUS "Found Matplotlib: ${MATPLOTLIB_PATH_DIRS} (found version \"${MATPLOTLIB_VERSION}\")")
    else(MATPLOTLIB_FOUND)
        message("=====================================================\n"
                "Matplotlib not found, Plot module won't be available.\n"
                "=====================================================\n")
    endif(MATPLOTLIB_FOUND)

endmacro(SetupMatplotlib)
