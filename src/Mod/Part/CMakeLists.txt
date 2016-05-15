
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
        AttachmentEditor/DepGraphTools.py
        AttachmentEditor/FrozenClass.py
        AttachmentEditor/TaskAttachmentEditor.py
        AttachmentEditor/TaskAttachmentEditor.ui
        AttachmentEditor/TempoVis.py
    DESTINATION
        Mod/Part/AttachmentEditor
       )
