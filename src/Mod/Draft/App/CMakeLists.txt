# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)


set(DraftUtils_LIBS
    Part
    FreeCADApp
)

SET(DraftUtils_SRCS
    AppDraftUtils.cpp
    AppDraftUtilsPy.cpp
    PreCompiled.h
)

add_library(DraftUtils SHARED ${DraftUtils_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(DraftUtils PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    DraftUtils
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(DraftUtils ${DraftUtils_LIBS})

if (MSVC)
    target_compile_options(DraftUtils PRIVATE /wd4275)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(DraftUtils)
endif()


SET_BIN_DIR(DraftUtils DraftUtils /Mod/Draft)
SET_PYTHON_PREFIX_SUFFIX(DraftUtils)

INSTALL(TARGETS DraftUtils DESTINATION ${CMAKE_INSTALL_LIBDIR})
