# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

disable_occt8_deprecation_warnings()

add_subdirectory(App)

set(SheetMetalCore_Scripts
    Init.py
    SheetMetalExport.py
    TestSheetMetalApp.py
)

if(BUILD_GUI)
    list (APPEND SheetMetalCore_Scripts InitGui.py SheetMetalCommands.py)
endif(BUILD_GUI)

add_custom_target(SheetMetalCoreScripts ALL
    SOURCES ${SheetMetalCore_Scripts}
)

fc_target_copy_resource(SheetMetalCoreScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/SheetMetalCore
    ${SheetMetalCore_Scripts}
)

install(
    FILES
        ${SheetMetalCore_Scripts}
    DESTINATION
        Mod/SheetMetalCore
)
