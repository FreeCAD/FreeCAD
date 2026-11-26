# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

disable_occt8_deprecation_warnings()

add_subdirectory(App)

set(SheetDesign_Scripts
    Init.py
    SheetDesignDxf.py
    TestSheetDesignApp.py
)

if(BUILD_GUI)
    list (APPEND SheetDesign_Scripts InitGui.py SheetDesignCommands.py)
endif(BUILD_GUI)

set(SheetDesign_Icons
    Resources/icons/SheetDesignWorkbench.svg
    Resources/icons/SheetDesign_Base.svg
    Resources/icons/SheetDesign_Cut.svg
    Resources/icons/SheetDesign_CutFlat.svg
    Resources/icons/SheetDesign_ExportDxf.svg
    Resources/icons/SheetDesign_Flange.svg
    Resources/icons/SheetDesign_FlatPattern.svg
    Resources/icons/SheetDesign_Refold.svg
    Resources/icons/SheetDesign_Rule.svg
    Resources/icons/SheetDesign_Unfold.svg
)

add_custom_target(SheetDesignScripts ALL
    SOURCES ${SheetDesign_Scripts} ${SheetDesign_Icons}
)

fc_target_copy_resource(SheetDesignScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/SheetDesign
    ${SheetDesign_Scripts}
    ${SheetDesign_Icons}
)

install(
    FILES
        ${SheetDesign_Scripts}
    DESTINATION
        Mod/SheetDesign
)

install(
    FILES
        ${SheetDesign_Icons}
    DESTINATION
        Mod/SheetDesign/Resources/icons
)
