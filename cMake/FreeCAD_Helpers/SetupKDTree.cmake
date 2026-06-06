macro(SetupKDTree)

    if(FREECAD_USE_EXTERNAL_KDTREE)
        find_path(KDTREE_INCLUDE_DIRS kdtree.hpp ${CMAKE_INCLUDE_PATH} PATH_SUFFIXES kdtree++)
        if(NOT KDTREE_INCLUDE_DIRS)
            message(FATAL_ERROR "FREECAD_USE_EXTERNAL_KDTREE was enabled but kdtree.hpp was not found")
        endif()
    else(FREECAD_USE_EXTERNAL_KDTREE)
        set(KDTREE_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/3rdParty/libkdtree)
    endif(FREECAD_USE_EXTERNAL_KDTREE)

endmacro(SetupKDTree)
