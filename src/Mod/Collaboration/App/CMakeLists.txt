# SPDX-License-Identifier: LGPL-2.1-or-later

set(Collaboration_LIBS
  FreeCADApp
  Part
)

SET(Module_SRCS
  AppCollaboration.cpp
  AppCollaborationPy.cpp
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})

SET(Collaboration_SRCS
  Topic.cpp
  Topic.h
  ${Module_SRCS}
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})

add_library(Collaboration SHARED ${Collaboration_SRCS})
target_include_directories(
  Collaboration
  PRIVATE
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(Collaboration ${Collaboration_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Collaboration)
endif()

if(FREECAD_USE_PCH)
  target_precompile_headers(Collaboration PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
  )
endif(FREECAD_USE_PCH)

SET_BIN_DIR(Collaboration CollaborationApp /Mod/Collaboration)
SET_PYTHON_PREFIX_SUFFIX(Collaboration)

INSTALL(TARGETS Collaboration DESTINATION ${CMAKE_INSTALL_LIBDIR})
