macro(CheckInterModuleDependencies)
    # ==============================================================================
    # inter-module dependencies

    # Takes a dependent module followed by a variable-length list of prerequisite modules.  Warns if
    # any of the prerequisite modules are disabled.
    function(REQUIRES_MODS dependent)
        if(${dependent})
            foreach(prerequisite IN LISTS ARGN)
                if(NOT ${prerequisite})
                    message(STATUS "${dependent} requires ${prerequisite} to be ON, but it"
                                   " is \"${${prerequisite}}\"")
                    set(${dependent}
                        OFF
                        PARENT_SCOPE)
                    break()
                endif(NOT ${prerequisite})
            endforeach()
        endif(${dependent})
    endfunction(REQUIRES_MODS)

    requires_mods(BUILD_BIM BUILD_PART BUILD_MESH BUILD_DRAFT)
    requires_mods(BUILD_DRAFT BUILD_SKETCHER)
    requires_mods(BUILD_DRAWING BUILD_PART BUILD_SPREADSHEET)
    requires_mods(BUILD_FEM BUILD_PART)
    requires_mods(BUILD_IDF BUILD_PART)
    requires_mods(BUILD_IMPORT BUILD_PART)
    requires_mods(BUILD_INSPECTION BUILD_MESH BUILD_POINTS BUILD_PART)
    requires_mods(BUILD_JTREADER BUILD_MESH)
    requires_mods(BUILD_MESH_PART BUILD_PART BUILD_MESH BUILD_SMESH)
    requires_mods(BUILD_FLAT_MESH BUILD_MESH_PART)
    requires_mods(BUILD_OPENSCAD BUILD_MESH_PART BUILD_DRAFT)
    requires_mods(BUILD_PART_DESIGN BUILD_SKETCHER)
    # REQUIRES_MODS(BUILD_CAM               BUILD_PART BUILD_MESH BUILD_ROBOT)
    requires_mods(BUILD_CAM BUILD_PART BUILD_MESH)
    requires_mods(BUILD_REVERSEENGINEERING BUILD_PART BUILD_MESH)
    requires_mods(BUILD_ROBOT BUILD_PART)
    requires_mods(BUILD_SANDBOX BUILD_PART BUILD_MESH)
    requires_mods(BUILD_SKETCHER BUILD_PART)
    requires_mods(BUILD_SPREADSHEET BUILD_DRAFT)
    requires_mods(BUILD_TECHDRAW BUILD_PART BUILD_SPREADSHEET)
endmacro(CheckInterModuleDependencies)
