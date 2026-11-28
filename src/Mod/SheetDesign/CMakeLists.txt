# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

disable_occt8_deprecation_warnings()

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(SheetDesign_Scripts
    Init.py
    SheetDesignBoxStyles.py
    SheetDesignDrawing.py
    SheetDesignExport.py
    SheetDesignFold.py
    TestSheetDesignApp.py
    TestSheetDesignBoxStyles.py
)

if(BUILD_GUI)
    list (APPEND SheetDesign_Scripts
        InitGui.py
        SheetDesignCommands.py
        SheetDesignTaskPanels.py
        SheetDesignViewProviders.py
        TestSheetDesignGui.py
    )
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
