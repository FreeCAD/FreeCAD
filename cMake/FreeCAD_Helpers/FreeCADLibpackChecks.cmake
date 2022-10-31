macro(FreeCADLibpackChecks)
    # ==============================================================================

    if(FREECAD_LIBPACK_USE)

        # checking for a unique file in LibPack location to make sure the right version of the LibPack is there
        find_file(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER MANIFEST.db ${FREECAD_LIBPACK_DIR})

        # don't show them in the GUI
        set(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER "${FREECAD_LIBPACK_CHECKFILE_CLBUNDLER}" CACHE INTERNAL "Find libpack")

        if (FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
            set(FREECAD_LIBPACK_VERSION "CLbundler" CACHE STRING "Displays if the libpack has been found" FORCE)
            include(cMake/UseLibPackCLbundler.cmake)
        else(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
            set(FREECAD_LIBPACK_VERSION "NOTFOUND" CACHE STRING "Displays if the libpack has been found" FORCE)
            message(SEND_ERROR  "Could not find libpack in specified location:" ${FREECAD_LIBPACK_DIR})
        endif(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
# -------------------------------- PyCXX --------------------------------

        find_package(PyCXX REQUIRED)

# -------------------------------- Swig ----------------------------------

        find_package(SWIG)

        if(NOT SWIG_FOUND)
            message("==================================================\n"
                    "SWIG not found, don't build SWIG binding for pivy.\n"
                    "==================================================\n")
        endif(NOT SWIG_FOUND)

# -------------------------------- Salome SMESH --------------------------

        if(NOT FREECAD_USE_EXTERNAL_SMESH)
            set(SMESH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/3rdParty/salomesmesh/inc)
        endif()

    endif(FREECAD_LIBPACK_USE)

endmacro(FreeCADLibpackChecks)
