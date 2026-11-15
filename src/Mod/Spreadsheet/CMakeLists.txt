# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Spreadsheet_Scripts
    Init.py
    TestSpreadsheetApp.py
    importXLSX.py
)

set(Spreadsheet_TestScripts
    SpreadsheetTests/__init__.py
    SpreadsheetTests/TestSpreadsheet.py
    SpreadsheetTests/TestImportXLSX.py
)

if(BUILD_GUI)
    list (APPEND Spreadsheet_Scripts InitGui.py TestSpreadsheetGui.py)
    list(APPEND Spreadsheet_TestScripts SpreadsheetTests/TestSpreadsheetGui.py)
endif(BUILD_GUI)

add_custom_target(SpreadsheetScripts ALL
    SOURCES ${Spreadsheet_Scripts}
            ${Spreadsheet_TestScripts}
)

fc_target_copy_resource(SpreadsheetScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Spreadsheet
    ${Spreadsheet_Scripts}
    ${Spreadsheet_TestScripts}
)

install(
    FILES
        ${Spreadsheet_Scripts}
    DESTINATION
        Mod/Spreadsheet
)

install(
    FILES
        ${Spreadsheet_TestScripts}
    DESTINATION
        Mod/Spreadsheet/SpreadsheetTests
)
