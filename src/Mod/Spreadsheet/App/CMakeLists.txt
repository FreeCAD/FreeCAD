if(WIN32)
    add_definitions(-DFCAppSpreadsheet)
endif(WIN32)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
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
    PropertySheet.pyi
    PropertySheetPyImp.cpp
    PropertyColumnWidths.cpp
    PropertyColumnWidths.h
    PropertyColumnWidths.pyi
    PropertyColumnWidthsPyImp.cpp
    PropertyRowHeights.cpp
    PropertyRowHeights.h
    PropertyRowHeights.pyi
    PropertyRowHeightsPyImp.cpp
    Sheet.cpp
    Sheet.h
    Sheet.pyi
    SheetPyImp.cpp
    SheetObserver.cpp
    SheetObserver.h
    Utils.cpp
    Utils.h
    AppSpreadsheet.cpp
)

generate_from_py(Sheet)
generate_from_py(PropertySheet)
generate_from_py(PropertyColumnWidths)
generate_from_py(PropertyRowHeights)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Spreadsheet_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Spreadsheet PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(Spreadsheet SHARED ${Spreadsheet_SRCS})
target_link_libraries(Spreadsheet ${Spreadsheet_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Spreadsheet)
endif()


SET_BIN_DIR(Spreadsheet Spreadsheet /Mod/Spreadsheet)
SET_PYTHON_PREFIX_SUFFIX(Spreadsheet)

INSTALL(TARGETS Spreadsheet DESTINATION ${CMAKE_INSTALL_LIBDIR})
