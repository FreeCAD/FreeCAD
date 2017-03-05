
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
        MakeBottle.py
        TestPartApp.py
        TestPartGui.py
        JoinFeatures.py
    DESTINATION
        Mod/Part
)

INSTALL(
    FILES
        AttachmentEditor/__init__.py
        AttachmentEditor/Commands.py
        AttachmentEditor/FrozenClass.py
        AttachmentEditor/TaskAttachmentEditor.py
        AttachmentEditor/TaskAttachmentEditor.ui
    DESTINATION
        Mod/Part/AttachmentEditor
       )

INSTALL(
    FILES
        BOPTools/__init__.py
        BOPTools/GeneralFuseResult.py
        BOPTools/JoinAPI.py
        BOPTools/JoinFeatures.py
        BOPTools/ShapeMerge.py
        BOPTools/SplitAPI.py
        BOPTools/SplitFeatures.py
        BOPTools/Utils.py
    DESTINATION
        Mod/Part/BOPTools
       )

INSTALL(
    FILES
        CompoundTools/__init__.py
        CompoundTools/_CommandCompoundFilter.py
        CompoundTools/CompoundFilter.py
    DESTINATION
        Mod/Part/CompoundTools
       )
