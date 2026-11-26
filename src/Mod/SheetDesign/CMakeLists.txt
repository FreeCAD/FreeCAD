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

add_custom_target(SheetDesignScripts ALL
    SOURCES ${SheetDesign_Scripts}
)

fc_target_copy_resource(SheetDesignScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/SheetDesign
    ${SheetDesign_Scripts}
)

install(
    FILES
        ${SheetDesign_Scripts}
    DESTINATION
        Mod/SheetDesign
)
