
SET(Show_SRCS
    __init__.py
    Containers.py
    DepGraphTools.py
    SceneDetail.py
    TempoVis.py
    TVObserver.py
    TVStack.py
    ShowUtils.py
    SceneDetails/__init__.py
    SceneDetails/Camera.py
    SceneDetails/ClipPlane.py
    SceneDetails/ObjectClipPlane.py
    SceneDetails/Pickability.py
    SceneDetails/VProperty.py
    SceneDetails/Workbench.py
)

SOURCE_GROUP("" FILES ${Show_SRCS})

ADD_CUSTOM_TARGET(Show ALL
    SOURCES ${Show_SRCS}
)

fc_copy_sources(Show "${CMAKE_BINARY_DIR}/Mod/Show" ${Show_SRCS})

INSTALL(
    FILES
        ${Show_SRCS}
    DESTINATION Mod/Show
)
