
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Spreadsheet_Scripts
    Init.py
    importXLSX.py
)

if(BUILD_GUI)
    list (APPEND Spreadsheet_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(SpreadsheetScripts ALL
    SOURCES ${Spreadsheet_Scripts}
)

fc_target_copy_resource(SpreadsheetScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Spreadsheet
    ${Spreadsheet_Scripts}
)

install(
    FILES
        ${Spreadsheet_Scripts}
    DESTINATION
        Mod/Spreadsheet
)
