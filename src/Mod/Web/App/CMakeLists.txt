# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(Web_LIBS
    FreeCADApp
    ${QtNetwork_LIBRARIES}
)

SET(Web_SRCS
    AppWeb.cpp
    PreCompiled.h
    Server.cpp
    Server.h
)

add_library(Web SHARED ${Web_SRCS})

target_include_directories(
    Web
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(
    Web
    SYSTEM
    PUBLIC
    ${QtNetwork_INCLUDE_DIRS}
)
target_link_libraries(Web ${Web_LIBS})

if(FREECAD_USE_PCH)
    target_precompile_headers(Web PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Web)
endif()

SET_BIN_DIR(Web Web /Mod/Web)
SET_PYTHON_PREFIX_SUFFIX(Web)

install(TARGETS Web DESTINATION ${CMAKE_INSTALL_LIBDIR})
