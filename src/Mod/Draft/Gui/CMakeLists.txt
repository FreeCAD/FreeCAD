# SPDX-License-Identifier: LGPL-2.1-or-later

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

set(DraftGuiExt_LIBS
    FreeCADGui
)

set(DraftGuiExt_SRCS
    AppDraftGui.cpp
    DraftWPPickProvider.h
    DraftWPPickProvider.cpp
)

add_library(DraftGuiExt SHARED ${DraftGuiExt_SRCS})

target_include_directories(
    DraftGuiExt
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(DraftGuiExt ${DraftGuiExt_LIBS})

if(MSVC)
    target_compile_options(DraftGuiExt PRIVATE /wd4275)
endif()

if(FREECAD_WARN_ERROR)
    target_compile_warn_error(DraftGuiExt)
endif()

SET_BIN_DIR(DraftGuiExt DraftGuiExt /Mod/Draft)
SET_PYTHON_PREFIX_SUFFIX(DraftGuiExt)

INSTALL(TARGETS DraftGuiExt DESTINATION ${CMAKE_INSTALL_LIBDIR})
