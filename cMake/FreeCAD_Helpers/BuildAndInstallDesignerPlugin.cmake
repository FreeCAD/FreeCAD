# SPDX-FileNotice: Part of the FreeCAD project.

macro(BuildAndInstallDesignerPlugin)
    # =================================================================
    # ============= Build and install the designer plugin =============
    # =================================================================

    if(BUILD_DESIGNER_PLUGIN)
        add_subdirectory(src/Tools/plugins/widget)
    endif(BUILD_DESIGNER_PLUGIN)
endmacro(BuildAndInstallDesignerPlugin)
