# SPDX-FileNotice: Part of the FreeCAD project.

macro(SetupFreetype)
#--------------------FreeType-----------------------

    if(FREECAD_USE_FREETYPE)
        find_package(Freetype)
        if(NOT FREETYPE_FOUND)
            message("===============================================================\n"
                    "FreeType2 not found. Part module will lack of makeWireString().\n"
                    "===============================================================\n")
        else()
            # find_package(harfbuzz CONFIG) fails on windows
            # lets do it the complicated way instead.
            find_path(HARFBUZZ_INCLUDE_DIR hb.h PATH_SUFFIXES harfbuzz)
            find_library(HARFBUZZ_LIBRARY NAMES harfbuzz)
            if(HARFBUZZ_INCLUDE_DIR AND HARFBUZZ_LIBRARY)
                if(NOT TARGET harfbuzz::harfbuzz)
                    add_library(harfbuzz::harfbuzz UNKNOWN IMPORTED)
                    set_target_properties(harfbuzz::harfbuzz PROPERTIES
                        IMPORTED_LOCATION "${HARFBUZZ_LIBRARY}"
                        INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIR}"
                    )
                endif()
            else()
                message(FATAL_ERROR "HarfBuzz not found")
            endif()
        endif(NOT FREETYPE_FOUND)
    endif(FREECAD_USE_FREETYPE)

endmacro(SetupFreetype)
