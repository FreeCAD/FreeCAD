# SPDX-License-Identifier: LGPL-2.1-or-later

set(Forms_Scripts
    Init.py
    importBlend.py
    Forms/__init__.py
    Forms/additive.py
    Forms/blender_bridge.py
    Forms/blend_export.py
    Forms/blend_import.py
    Forms/blender_create.py
    Forms/blender_extract.py
    Forms/brep.py
    Forms/cage.py
    Forms/box.py
    Forms/edit.py
    Forms/edit_tools.py
    Forms/elementmap.py
    Forms/feedback.py
    Forms/form.py
    Forms/interaction.py
    Forms/imported.py
    Forms/operations.py
    Forms/placement.py
    Forms/pipe.py
    Forms/primitives.py
    Forms/symmetry.py
    Forms/surface.py
    Forms/taskpanels.py
    Forms/tmesh.py
    Forms/toolbar.py
    Forms/topology.py
)

if(BUILD_GUI)
    list(APPEND Forms_Scripts
        InitGui.py
        CommandPrimitives.py
        CommandEdit.py
        CommandTopology.py
        Forms/TaskFormEdit.ui
        Forms/TaskFormFlatten.ui
        Forms/TaskFormInsertEdge.ui
        Forms/TaskFormMatch.ui
        Forms/TaskFormSubdivide.ui
        Forms/TaskFormStraighten.ui
        Forms/TaskFormThicken.ui
        Forms/TaskFormTool.ui
        Forms/TaskFormWeld.ui
    )
    set(Forms_Resources
        Resources/icons/Forms_Box.svg
        Resources/icons/Forms_Cylinder.svg
        Resources/icons/Forms_Sphere.svg
        Resources/icons/Forms_Pipe.svg
        Resources/icons/Forms_Face.svg
        Resources/icons/Forms_Torus.svg
        Resources/icons/Forms_Tube.svg
        Resources/icons/Forms_Bridge.svg
        Resources/icons/Forms_EraseAndFill.svg
        Resources/icons/Forms_Flatten.svg
        Resources/icons/Forms_Match.svg
        Resources/icons/Forms_SetPivot.svg
        Resources/icons/Forms_Crease.svg
        Resources/icons/Forms_Uncrease.svg
        Resources/icons/Forms_Unweld.svg
        Resources/icons/Forms_Weld.svg
        Resources/icons/Forms_Straighten.svg
        Resources/icons/Forms_InsertEdge.svg
        Resources/icons/Forms_InsertPoint.svg
        Resources/icons/Forms_FillHole.svg
        Resources/icons/Forms_Subdivide.svg
        Resources/icons/Forms_Thicken.svg
        Resources/icons/Forms_Pointer_InsertEdge.svg
        Resources/icons/Forms_Pointer_InsertPoint.svg
        Resources/icons/Forms_Pointer_Subdivide.svg
        Resources/icons/Forms_Pointer_SetPivot.svg
        Resources/icons/Forms_Workbench.svg
    )
endif(BUILD_GUI)

set(Forms_TestScripts
    FormsTests/__init__.py
    FormsTests/TestBlendExport.py
    FormsTests/TestBlendImport.py
    FormsTests/TestBRep.py
    FormsTests/TestTopology.py
)

add_custom_target(FormsScripts ALL
    SOURCES ${Forms_Scripts} ${Forms_TestScripts} ${Forms_Resources} README.md
)

fc_copy_sources(FormsScripts
    "${CMAKE_BINARY_DIR}/Mod/Forms"
    ${Forms_Scripts}
    ${Forms_TestScripts}
)

if(BUILD_GUI)
    fc_copy_sources(FormsScripts
        "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Forms"
        ${Forms_Resources}
    )
endif(BUILD_GUI)

install(
    FILES Init.py importBlend.py
    DESTINATION Mod/Forms
)

if(BUILD_GUI)
    install(
        FILES InitGui.py CommandPrimitives.py CommandEdit.py CommandTopology.py
        DESTINATION Mod/Forms
    )
    install(
        FILES ${Forms_Resources}
        DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Forms/Resources/icons"
    )
endif(BUILD_GUI)

install(
    FILES Forms/__init__.py Forms/additive.py Forms/blender_bridge.py Forms/blend_export.py Forms/blend_import.py Forms/blender_create.py Forms/blender_extract.py Forms/brep.py Forms/cage.py Forms/box.py Forms/edit.py Forms/edit_tools.py Forms/elementmap.py Forms/feedback.py Forms/form.py Forms/imported.py Forms/interaction.py Forms/operations.py Forms/placement.py Forms/pipe.py Forms/primitives.py Forms/symmetry.py Forms/surface.py Forms/taskpanels.py Forms/tmesh.py Forms/toolbar.py Forms/topology.py
    DESTINATION Mod/Forms/Forms
)

install(
    FILES Forms/TaskFormEdit.ui Forms/TaskFormFlatten.ui Forms/TaskFormInsertEdge.ui Forms/TaskFormMatch.ui Forms/TaskFormSubdivide.ui Forms/TaskFormStraighten.ui Forms/TaskFormThicken.ui Forms/TaskFormTool.ui Forms/TaskFormWeld.ui
    DESTINATION Mod/Forms/Forms
)

install(
    FILES FormsTests/__init__.py FormsTests/TestBlendExport.py FormsTests/TestBlendImport.py FormsTests/TestBRep.py FormsTests/TestTopology.py
    DESTINATION Mod/Forms/FormsTests
)
