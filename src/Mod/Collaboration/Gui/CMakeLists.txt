# SPDX-License-Identifier: LGPL-2.1-or-later

set(CollaborationGui_LIBS
  FreeCADGui
  Collaboration
)

PYSIDE_WRAP_RC(Collaboration_QRC_SRCS Resources/Collaboration.qrc)
qt_add_resources(CollaborationResource_SRCS Resources/Collaboration.qrc)

SOURCE_GROUP("Resources" FILES ${CollaborationResource_SRCS})

SET(CollaborationGui_SRCS_Module
    AppCollaborationGui.cpp
    AppCollaborationGuiPy.cpp
    ${Collaboration_QRC_SRCS}
)

SOURCE_GROUP("Module" FILES ${CollaborationGui_SRCS_Module})

SET(CollaborationGui_SRCS
  ${CollaborationResource_SRCS}
  ${CollaborationGui_SRCS_Module}
  ViewProviderTopic.cpp
  ViewProviderTopic.h
  Command.cpp
)

add_library(CollaborationGui SHARED ${CollaborationGui_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(CollaborationGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    CollaborationGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(WIN32)
    target_compile_definitions(CollaborationGui PRIVATE _USE_MATH_DEFINES)
endif(WIN32)

target_link_libraries(CollaborationGui ${CollaborationGui_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(CollaborationGui)
endif()

SET_BIN_DIR(CollaborationGui CollaborationGui /Mod/Collaboration)
SET_PYTHON_PREFIX_SUFFIX(CollaborationGui)

fc_copy_sources(CollaborationGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Collaboration" ${CollaborationGuiIcon_SVG})
fc_target_copy_resource(CollaborationGui
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/Collaboration
        Collaboration_rc.py)

INSTALL(TARGETS CollaborationGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${Collaboration_QRC_SRCS} DESTINATION Mod/Collaboration)
