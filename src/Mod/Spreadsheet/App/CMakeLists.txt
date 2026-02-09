# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

if(WIN32)
    add_definitions(-DFCAppSpreadsheet)
endif(WIN32)

set(Spreadsheet_LIBS
    FreeCADApp
)

set(Spreadsheet_SRCS
    Cell.cpp
    Cell.h
    DisplayUnit.h
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

add_library(Spreadsheet SHARED ${Spreadsheet_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(Spreadsheet PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    Spreadsheet
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(Spreadsheet ${Spreadsheet_LIBS})

if (MSVC)
    target_compile_options(Spreadsheet PRIVATE /wd4251 /wd4275)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Spreadsheet)
endif()


SET_BIN_DIR(Spreadsheet Spreadsheet /Mod/Spreadsheet)
SET_PYTHON_PREFIX_SUFFIX(Spreadsheet)

INSTALL(TARGETS Spreadsheet DESTINATION ${CMAKE_INSTALL_LIBDIR})
