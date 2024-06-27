macro(FreeCADLibpackChecks)
    # ==============================================================================

    if(FREECAD_LIBPACK_USE)

        # checking for a unique file in LibPack location to make sure the right version of the LibPack is there
        find_file(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER MANIFEST.db PATHS ${FREECAD_LIBPACK_DIR} NO_DEFAULT_PATH NO_CACHE)
        find_file(FREECAD_LIBPACK_CHECKFILE_VERSION FREECAD_LIBPACK_VERSION PATHS ${FREECAD_LIBPACK_DIR} NO_DEFAULT_PATH NO_CACHE)

        # don't show them in the GUI
        set(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER "${FREECAD_LIBPACK_CHECKFILE_CLBUNDLER}" CACHE INTERNAL "Find libpack")
        set(FREECAD_LIBPACK_CHECKFILE_VERSION "${FREECAD_LIBPACK_CHECKFILE_VERSION}" CACHE INTERNAL "Find libpack v3+")

        if (FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
            set(FREECAD_LIBPACK_VERSION "CLbundler" CACHE STRING "Displays if the libpack has been found" FORCE)
            message("Found LibPack v2 (CLBundler) via ${FREECAD_LIBPACK_CHECKFILE_CLBUNDLER}")
            include(cMake/UseLibPackCLbundler.cmake)
        elseif (FREECAD_LIBPACK_CHECKFILE_VERSION)
            file(READ ${FREECAD_LIBPACK_CHECKFILE_VERSION} FREECAD_LIBPACK_VERSION)
            set(FREECAD_LIBPACK_VERSION "${FREECAD_LIBPACK_VERSION}" CACHE STRING "Version of the LibPack, if found" FORCE)
            message("Found LibPack " ${FREECAD_LIBPACK_VERSION})
            include(cMake/UseLibPack3.cmake)
        else(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
            set(FREECAD_LIBPACK_VERSION "NOTFOUND" CACHE STRING "Displays if the libpack has been found" FORCE)
            message(SEND_ERROR  "Could not find LibPack in specified location:" ${FREECAD_LIBPACK_DIR})
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
