PYSIDE_WRAP_RC(Spreadsheet_QRC_SRCS Gui/Resources/Spreadsheet.qrc)

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
    DESTINATION
        Mod/Spreadsheet
)
