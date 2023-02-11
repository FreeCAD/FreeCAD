if(WIN32)
    add_definitions(-DFCAppSpreadsheet)
endif(WIN32)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)

set(Spreadsheet_LIBS
    FreeCADApp
)

set(Spreadsheet_SRCS
    Cell.cpp
    Cell.h
    DisplayUnit.h
    PreCompiled.cpp
    PreCompiled.h
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
    Sheet.cpp
    Sheet.h
    SheetPy.xml
    SheetPyImp.cpp
    SheetObserver.cpp
    SheetObserver.h
    Utils.cpp
    Utils.h
    AppSpreadsheet.cpp
)

generate_from_xml(SheetPy)
generate_from_xml(PropertySheetPy)
generate_from_xml(PropertyColumnWidthsPy)
generate_from_xml(PropertyRowHeightsPy)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Spreadsheet_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Spreadsheet PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(Spreadsheet SHARED ${Spreadsheet_SRCS})
target_link_libraries(Spreadsheet ${Spreadsheet_LIBS})


SET_BIN_DIR(Spreadsheet Spreadsheet /Mod/Spreadsheet)
SET_PYTHON_PREFIX_SUFFIX(Spreadsheet)

INSTALL(TARGETS Spreadsheet DESTINATION ${CMAKE_INSTALL_LIBDIR})
