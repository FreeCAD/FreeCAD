# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif()

set(MbDFEM_Scripts
    FreeCADMbDAnimation.py
    FreeCADMbDAnimationPanel.py
    FreeCADMbDBackend.py
    FreeCADMbDExporter.py
    FreeCADMbDMassMarkerPanel.py
    FreeCADMbDResults.py
    FreeCADMbDSimulationPanel.py
    Init.py
    TestMbDFEMApp.py
)

set(MbDFEM_Examples
    Examples/CreateMbDFEMModel.py
)

if(BUILD_GUI)
    list(APPEND MbDFEM_Scripts InitGui.py TestMbDFEMGui.py)
endif()

add_custom_target(MbDFEMScripts ALL
    SOURCES ${MbDFEM_Scripts} ${MbDFEM_Examples}
)

fc_target_copy_resource_flat(MbDFEMScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/MbDFEM
    ${MbDFEM_Scripts}
)

fc_target_copy_resource(MbDFEMScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/MbDFEM
    ${MbDFEM_Examples}
)

add_custom_command(
    TARGET MbDFEMScripts
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/Mod/MbDFEM/__pycache__"
    COMMENT "Removing stale MbDFEM Python bytecode cache"
)

install(
    FILES ${MbDFEM_Scripts}
    DESTINATION Mod/MbDFEM
)

install(
    FILES ${MbDFEM_Examples}
    DESTINATION Mod/MbDFEM/Examples
)
