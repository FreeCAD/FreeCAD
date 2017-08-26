
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

SET(Part_SRCS
    __init__.py
    Init.py
    InitGui.py
    MakeBottle.py
    TestPartApp.py
    TestPartGui.py
    JoinFeatures.py
)

SET(AttachmentEditor_SRCS
    AttachmentEditor/__init__.py
    AttachmentEditor/Commands.py
    AttachmentEditor/FrozenClass.py
    AttachmentEditor/TaskAttachmentEditor.py
    AttachmentEditor/TaskAttachmentEditor.ui
)

SET(BOPTools_SRCS
    BOPTools/__init__.py
    BOPTools/GeneralFuseResult.py
    BOPTools/JoinAPI.py
    BOPTools/JoinFeatures.py
    BOPTools/ShapeMerge.py
    BOPTools/SplitAPI.py
    BOPTools/SplitFeatures.py
    BOPTools/Utils.py
)

set(CompoundTools_SRCS
    CompoundTools/__init__.py
    CompoundTools/_CommandCompoundFilter.py
    CompoundTools/CompoundFilter.py
)

ADD_CUSTOM_TARGET(Part_COPY_FILES ALL
    SOURCES ${Part_SRCS} ${AttachmentEditor_SRCS} ${BOPTools_SRCS} ${CompoundTools_SRCS}
)

fc_copy_sources(Part_COPY_FILES "${CMAKE_BINARY_DIR}/Mod/part" ${Part_SRCS})
fc_copy_sources(Part_COPY_FILES "${CMAKE_BINARY_DIR}/Mod/part" ${AttachmentEditor_SRCS})
fc_copy_sources(Part_COPY_FILES "${CMAKE_BINARY_DIR}/Mod/part" ${BOPTools_SRCS})
fc_copy_sources(Part_COPY_FILES "${CMAKE_BINARY_DIR}/Mod/part" ${CompoundTools_SRCS})

INSTALL(
    FILES
        ${Part_SRCS}
    DESTINATION
        Mod/part
)

INSTALL(
    FILES
        ${AttachmentEditor_SRCS}
    DESTINATION
        Mod/part/AttachmentEditor
       )

INSTALL(
    FILES
        ${BOPTools_SRCS}
    DESTINATION
        Mod/part/BOPTools
       )

INSTALL(
    FILES
        ${CompoundTools_SRCS}
    DESTINATION
        Mod/part/CompoundTools
       )
