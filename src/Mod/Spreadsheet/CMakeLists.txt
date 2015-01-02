PYSIDE_WRAP_RC(Spreadsheet_QRC_SRCS Resources/Spreadsheet.qrc)

SET(Spreadsheet_SRCS
    Init.py
    InitGui.py
    Spreadsheet.py
)
SOURCE_GROUP("" FILES ${Spreadsheet_SRCS})

SET(all_files ${Spreadsheet_SRCS})

ADD_CUSTOM_TARGET(Spreadsheet ALL
    SOURCES ${all_files} ${Spreadsheet_QRC_SRCS}
)

fc_copy_sources(Spreadsheet "${CMAKE_BINARY_DIR}/Mod/Spreadsheet" ${all_files})

fc_target_copy_resource(Spreadsheet
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Spreadsheet
    Spreadsheet_rc.py)

INSTALL(
    FILES ${Spreadsheet_SRCS} ${Spreadsheet_QRC_SRCS}
    DESTINATION Mod/Spreadsheet
)
