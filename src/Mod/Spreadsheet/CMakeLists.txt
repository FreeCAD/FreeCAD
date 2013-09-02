
SET(Spreadsheet_SRCS
    Init.py
    InitGui.py
    Spreadsheet.py
    Spreadsheet_rc.py
)
SOURCE_GROUP("" FILES ${Spreadsheet_SRCS})

SET(all_files ${Spreadsheet_SRCS})

ADD_CUSTOM_TARGET(Spreadsheet ALL
    SOURCES ${all_files}
)

fc_copy_sources(Spreadsheet "${CMAKE_BINARY_DIR}/Mod/Spreadsheet" ${all_files})

INSTALL(
    FILES ${Spreadsheet_SRCS}
    DESTINATION Mod/Spreadsheet
)
