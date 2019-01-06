
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Part_Scripts
    Init.py
    JoinFeatures.py
    MakeBottle.py
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

set(BOPTools_Scripts
    BOPTools/__init__.py
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

add_custom_target(PartScripts ALL SOURCES
    ${Part_Scripts}
    ${AttachmentEditor_Scripts}
    ${BOPTools_Scripts}
    ${CompoundTools_Scripts}
)

fc_target_copy_resource(PartScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Part
    ${Part_Scripts}
    ${AttachmentEditor_Scripts}
    ${BOPTools_Scripts}
    ${CompoundTools_Scripts}
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
