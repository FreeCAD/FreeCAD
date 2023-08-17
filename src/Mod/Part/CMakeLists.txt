
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Part_Scripts
    Init.py
    JoinFeatures.py
    MakeBottle.py
    PartEnums.py
    TestPartApp.py
)

if(BUILD_GUI)
    list (APPEND Part_Scripts
          InitGui.py
          TestPartGui.py
    )
endif(BUILD_GUI)

set(AttachmentEditor_Scripts
    AttachmentEditor/__init__.py
    AttachmentEditor/Commands.py
    AttachmentEditor/FrozenClass.py
    AttachmentEditor/TaskAttachmentEditor.py
    AttachmentEditor/TaskAttachmentEditor.ui
)

set(BasicShapes_Scripts
    BasicShapes/__init__.py
    BasicShapes/Shapes.py
    BasicShapes/Utils.py
)

if(BUILD_GUI)
    list (APPEND BasicShapes_Scripts
          BasicShapes/ShapeContent.py
          BasicShapes/CommandShapes.py
          BasicShapes/ViewProviderShapes.py
          BasicShapes/TaskTube.ui
    )
endif(BUILD_GUI)

set(BOPTools_Scripts
    BOPTools/__init__.py
    BOPTools/BOPFeatures.py
    BOPTools/GeneralFuseResult.py
    BOPTools/JoinAPI.py
    BOPTools/JoinFeatures.py
    BOPTools/ShapeMerge.py
    BOPTools/SplitAPI.py
    BOPTools/SplitFeatures.py
    BOPTools/Utils.py
)

set(CompoundTools_Scripts
    CompoundTools/__init__.py
    CompoundTools/_CommandCompoundFilter.py
    CompoundTools/_CommandExplodeCompound.py
    CompoundTools/CompoundFilter.py
    CompoundTools/Explode.py
)

set(Part_tests
    parttests/__init__.py
    parttests/part_test_objects.py
    parttests/regression_tests.py
    parttests/TopoShapeListTest.py
    parttests/ColorPerFaceTest.py
)

add_custom_target(PartScripts ALL SOURCES
    ${Part_Scripts}
    ${AttachmentEditor_Scripts}
    ${BasicShapes_Scripts}
    ${BOPTools_Scripts}
    ${CompoundTools_Scripts}
    ${Part_tests}
)

fc_target_copy_resource(PartScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Part
    ${Part_Scripts}
    ${AttachmentEditor_Scripts}
    ${BasicShapes_Scripts}
    ${BOPTools_Scripts}
    ${CompoundTools_Scripts}
    ${Part_tests}
)

INSTALL(
    FILES
        ${Part_Scripts}
    DESTINATION
        Mod/Part
)

INSTALL(
    FILES
        ${AttachmentEditor_Scripts}
    DESTINATION
        Mod/Part/AttachmentEditor
)

INSTALL(
    FILES
        ${BasicShapes_Scripts}
    DESTINATION
        Mod/Part/BasicShapes
)

INSTALL(
    FILES
        ${BOPTools_Scripts}
    DESTINATION
        Mod/Part/BOPTools
)

INSTALL(
    FILES
        ${CompoundTools_Scripts}
    DESTINATION
        Mod/Part/CompoundTools
)

INSTALL(
    FILES
        ${Part_tests}
    DESTINATION
        Mod/Part/parttests
)
