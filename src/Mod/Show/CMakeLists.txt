set (
    Show_SRCS
    __init__.py
    Containers.py
    DepGraphTools.py
    SceneDetail.py
    mTempoVis.py
    TVObserver.py
    TVStack.py
    ShowUtils.py)

set (
    Show_SceneDetails_SRCS
    SceneDetails/__init__.py
    SceneDetails/Camera.py
    SceneDetails/ClipPlane.py
    SceneDetails/ObjectClipPlane.py
    SceneDetails/Pickability.py
    SceneDetails/VProperty.py
    SceneDetails/Workbench.py)

set (Show_All_SRCS ${Show_SRCS} ${Show_SceneDetails_SRCS})

source_group ("" FILES ${Show_SRCS})

add_custom_target (Show ALL SOURCES ${Show_All_SRCS})

fc_copy_sources (Show "${CMAKE_BINARY_DIR}/Mod/Show" ${Show_All_SRCS})

install (FILES ${Show_SRCS} DESTINATION Mod/Show)
install (FILES ${Show_SceneDetails_SRCS} DESTINATION Mod/Show/SceneDetails)
