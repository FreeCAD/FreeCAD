# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(SheetMetalCore_LIBS
  FreeCADApp
  Part
)

SET(SheetMetalCore_SRCS
  AppSheetMetalCore.cpp
  PreCompiled.h
  Features.cpp
  Features.h
  SheetModel.cpp
  SheetModel.h
)

add_library(SheetMetalCore SHARED ${SheetMetalCore_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(SheetMetalCore PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    SheetMetalCore
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_directories(SheetMetalCore PUBLIC ${OCC_LIBRARY_DIR})
target_link_libraries(SheetMetalCore ${SheetMetalCore_LIBS})
target_compile_warn_error(SheetMetalCore)

SET_BIN_DIR(SheetMetalCore)
SET_PYTHON_PREFIX_SUFFIX(SheetMetalCore)

install(TARGETS SheetMetalCore DESTINATION ${CMAKE_INSTALL_LIBDIR})
