macro(FreeCADLibpackChecks)
    # ==============================================================================

    if(FREECAD_LIBPACK_USE)

        # checking for a unique file in LibPack location to make sure the right version of the LibPack is there
        find_file(FREECAD_LIBPACK_CHECKFILE6X boost_program_options-vc80-mt-gd.lib ${FREECAD_LIBPACK_DIR}/lib )
        find_file(FREECAD_LIBPACK_CHECKFILE7X boost_program_options-vc90-mt-gd-1_39.lib ${FREECAD_LIBPACK_DIR}/lib )
        find_file(FREECAD_LIBPACK_CHECKFILE8X boost_program_options-vc90-mt-gd-1_48.lib ${FREECAD_LIBPACK_DIR}/lib )
        find_file(FREECAD_LIBPACK_CHECKFILE9X boost_program_options-vc90-mt-gd-1_54.lib ${FREECAD_LIBPACK_DIR}/lib )
        find_file(FREECAD_LIBPACK_CHECKFILE10X boost_program_options-vc110-mt-1_55.lib ${FREECAD_LIBPACK_DIR}/lib )
        find_file(FREECAD_LIBPACK_CHECKCUSTOM boost_program_options-vc90-mt-gd-1_41.lib ${FREECAD_LIBPACK_DIR}/lib )
        find_file(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER MANIFEST.db ${FREECAD_LIBPACK_DIR})

        # don't show them in the GUI
        set(FREECAD_LIBPACK_CHECKFILE6X "${FREECAD_LIBPACK_CHECKFILE6X}" CACHE INTERNAL "Find libpack")
        set(FREECAD_LIBPACK_CHECKFILE7X "${FREECAD_LIBPACK_CHECKFILE7X}" CACHE INTERNAL "Find libpack")
        set(FREECAD_LIBPACK_CHECKFILE8X "${FREECAD_LIBPACK_CHECKFILE8X}" CACHE INTERNAL "Find libpack")
        set(FREECAD_LIBPACK_CHECKFILE9X "${FREECAD_LIBPACK_CHECKFILE9X}" CACHE INTERNAL "Find libpack")
        set(FREECAD_LIBPACK_CHECKFILE10X "${FREECAD_LIBPACK_CHECKFILE10X}" CACHE INTERNAL "Find libpack")
        set(FREECAD_LIBPACK_CHECKCUSTOM "${FREECAD_LIBPACK_CHECKCUSTOM}" CACHE INTERNAL "Find libpack")
        set(FREECAD_LIBPACK_CHECKFILE_CLBUNDLER "${FREECAD_LIBPACK_CHECKFILE_CLBUNDLER}" CACHE INTERNAL "Find libpack")

        if (FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
            set(FREECAD_LIBPACK_VERSION "CLbundler" CACHE STRING "Displays if the libpack has been found" FORCE)
            include(cMake/UseLibPackCLbundler.cmake)
        elseif(FREECAD_LIBPACK_CHECKFILE9X)
            set(FREECAD_LIBPACK_VERSION "9.x" CACHE STRING "Displays if the libpack has been found" FORCE)
            include(cMake/UseLibPack9x.cmake)
            set(SWIG_EXECUTABLE ${FREECAD_LIBPACK_DIR}/tools/swigwin-2.0.11/swig.exe CACHE STRING "Swig" FORCE)
            set(FREECAD_LIBPACK_PYSIDEUIC_REL   "${FREECAD_LIBPACK_DIR}/pyside-tools/Lib/site-packages")
            file(GLOB FREECAD_LIBPACK_PIVY_COIN "${FREECAD_LIBPACK_DIR}/pivy/*.*")
            file(GLOB FREECAD_LIBPACK_SHIBOKEN  "${FREECAD_LIBPACK_DIR}/shiboken-1.2.1/lib/site-packages/*.pyd")
            file(GLOB FREECAD_LIBPACK_PYSIDE    "${FREECAD_LIBPACK_DIR}/pyside/lib/site-packages/PySide/*.py*")
            file(GLOB_RECURSE FREECAD_LIBPACK_PYSIDEUIC RELATIVE "${FREECAD_LIBPACK_PYSIDEUIC_REL}" "${FREECAD_LIBPACK_PYSIDEUIC_REL}/pysideuic/*.py")
            file(GLOB FREECAD_LIBPACK_PYTHON    "${FREECAD_LIBPACK_DIR}/bin/*.py*")
        elseif(FREECAD_LIBPACK_CHECKFILE10X)
            set(FREECAD_LIBPACK_VERSION "10.x" CACHE STRING "Displays if the libpack has been found" FORCE)
            include(cMake/UseLibPack10x.cmake)
            set(SWIG_EXECUTABLE ${FREECAD_LIBPACK_DIR}/tools/swigwin-3.0.2/swig.exe CACHE STRING "Swig" FORCE)
        elseif(FREECAD_LIBPACK_CHECKCUSTOM)
            set(FREECAD_LIBPACK_VERSION "Custom" CACHE STRING "Displays if the libpack has been found" FORCE)
            include(cMake/UseLibPackCustom.cmake)
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
