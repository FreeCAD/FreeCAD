macro(CheckInterModuleDependencies)
    # ==============================================================================
    #inter-module dependencies

    # Takes a dependent module followed by a variable-length list of prerequisite
    # modules.  Warns if any of the prerequisite modules are disabled.  Without
    # Qt whole module trees are switched off on purpose, so there the dependent
    # module simply follows its prerequisite.
    function(REQUIRES_MODS dependent)
        if(${dependent})
            foreach(prerequisite IN LISTS ARGN)
                if(NOT ${prerequisite})
                    if(FREECAD_HAS_QT)
                        message(SEND_ERROR "${dependent} requires ${prerequisite} to be ON, but it"
                            " is \"${${prerequisite}}\"")
                    else()
                        message(STATUS "Disabling ${dependent}: it requires ${prerequisite}")
                    endif()
                    set(${dependent} OFF PARENT_SCOPE)
                    set(MODULE_DISABLED ON PARENT_SCOPE)
                    break()
                endif(NOT ${prerequisite})
            endforeach()
        endif(${dependent})
    endfunction(REQUIRES_MODS)

    # A module turned off here may be the prerequisite of a module checked
    # earlier in the list, so repeat until the set of modules is stable.
    set(MODULE_DISABLED ON)
    while(MODULE_DISABLED)
        set(MODULE_DISABLED OFF)

        REQUIRES_MODS(BUILD_ASSEMBLY           BUILD_PART BUILD_PART_DESIGN BUILD_SPREADSHEET)
        REQUIRES_MODS(BUILD_BIM                BUILD_PART BUILD_MESH BUILD_MESH_PART BUILD_DRAFT)
        REQUIRES_MODS(BUILD_DRAFT              BUILD_SKETCHER BUILD_TECHDRAW)
        REQUIRES_MODS(BUILD_FEM                BUILD_PART)
        REQUIRES_MODS(BUILD_IMPORT             BUILD_PART BUILD_PART_DESIGN)
        REQUIRES_MODS(BUILD_INSPECTION         BUILD_MESH BUILD_POINTS BUILD_PART)
        REQUIRES_MODS(BUILD_JTREADER           BUILD_MESH)
        REQUIRES_MODS(BUILD_MESH_PART          BUILD_PART BUILD_MESH)
        REQUIRES_MODS(BUILD_FLAT_MESH          BUILD_MESH_PART)
        REQUIRES_MODS(BUILD_OPENSCAD           BUILD_MESH_PART BUILD_DRAFT)
        REQUIRES_MODS(BUILD_MATERIAL_EXTERNAL  BUILD_MATERIAL)
        REQUIRES_MODS(BUILD_MEASURE            BUILD_PART)
        REQUIRES_MODS(BUILD_PART               BUILD_MATERIAL)
        REQUIRES_MODS(BUILD_PART_DESIGN        BUILD_SKETCHER)
        # REQUIRES_MODS(BUILD_CAM              BUILD_PART BUILD_MESH BUILD_ROBOT)
        REQUIRES_MODS(BUILD_CAM                BUILD_PART BUILD_MESH)
        REQUIRES_MODS(BUILD_REVERSEENGINEERING BUILD_PART BUILD_MESH BUILD_POINTS)
        REQUIRES_MODS(BUILD_ROBOT              BUILD_PART)
        REQUIRES_MODS(BUILD_SKETCHER           BUILD_PART)
        REQUIRES_MODS(BUILD_SPREADSHEET        BUILD_DRAFT)
        REQUIRES_MODS(BUILD_SURFACE            BUILD_PART)
        REQUIRES_MODS(BUILD_TECHDRAW           BUILD_PART BUILD_PART_DESIGN BUILD_SPREADSHEET BUILD_MEASURE BUILD_IMPORT)
    endwhile()
endmacro(CheckInterModuleDependencies)
