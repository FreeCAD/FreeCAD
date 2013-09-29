if(WIN32)
    add_definitions(-DFCAppSpreadsheet)
endif(WIN32)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
)

set(Spreadsheet_LIBS
    FreeCADApp
)

set(Spreadsheet_SRCS
    Expression.cpp
    Expression.h
    Cell.cpp
    Cell.h
    DisplayUnit.h
    PropertySheet.cpp
    PropertySheet.h
    PropertySheetPy.xml
    PropertySheetPyImp.cpp
    PropertyColumnWidths.cpp
    PropertyColumnWidths.h
    PropertyColumnWidthsPy.xml
    PropertyColumnWidthsPyImp.cpp
    PropertyRowHeights.cpp
    PropertyRowHeights.h
    PropertyRowHeightsPy.xml
    PropertyRowHeightsPyImp.cpp
    PreCompiled.cpp
    PreCompiled.h
    Sheet.cpp
    Sheet.h
    SheetPy.xml
    SheetPyImp.cpp
    SheetObserver.cpp
    SheetObserver.h
    Utils.cpp
    Utils.h
    Range.h
    Range.cpp
    AppSpreadsheet.cpp
)

generate_from_xml(SheetPy)
generate_from_xml(PropertySheetPy)
generate_from_xml(PropertyColumnWidthsPy)
generate_from_xml(PropertyRowHeightsPy)

add_library(Spreadsheet SHARED ${Spreadsheet_SRCS})
target_link_libraries(Spreadsheet ${Spreadsheet_LIBS})


fc_target_copy_resource(Spreadsheet 
    ${CMAKE_SOURCE_DIR}/src/Mod/Spreadsheet
    ${CMAKE_BINARY_DIR}/Mod/Spreadsheet
    Init.py)

SET_BIN_DIR(Spreadsheet Spreadsheet /Mod/Spreadsheet)
SET_PYTHON_PREFIX_SUFFIX(Spreadsheet)

INSTALL(TARGETS Spreadsheet DESTINATION ${CMAKE_INSTALL_LIBDIR})
