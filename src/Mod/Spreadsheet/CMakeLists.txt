PYSIDE_WRAP_RC(Spreadsheet_QRC_SRCS Resources/Spreadsheet.qrc)

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

fc_target_copy_resource(Spreadsheet
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Spreadsheet
    Spreadsheet_rc.py)

INSTALL(
    FILES
        Init.py
        InitGui.py
    DESTINATION
        Mod/Spreadsheet
)
