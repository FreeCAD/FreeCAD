# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(SheetDesign_LIBS
  FreeCADApp
  Part
  PartDesign
)

SET(SheetDesign_SRCS
  AppSheetDesign.cpp
  PreCompiled.h
  Features.cpp
  Features.h
  FoldSolver.cpp
  FoldSolver.h
  SheetModel.cpp
  SheetModel.h
)

add_library(SheetDesign SHARED ${SheetDesign_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(SheetDesign PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    SheetDesign
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(SheetDesign SYSTEM PRIVATE ${EIGEN3_INCLUDE_DIR})

target_link_directories(SheetDesign PUBLIC ${OCC_LIBRARY_DIR})
target_link_libraries(SheetDesign ${SheetDesign_LIBS})
target_compile_warn_error(SheetDesign)

SET_BIN_DIR(SheetDesign)
SET_PYTHON_PREFIX_SUFFIX(SheetDesign)

install(TARGETS SheetDesign DESTINATION ${CMAKE_INSTALL_LIBDIR})
