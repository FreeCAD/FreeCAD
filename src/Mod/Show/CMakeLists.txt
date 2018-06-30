
SET(Show_SRCS
    __init__.py
    DepGraphTools.py
    FrozenClass.py
    TempoVis.py
    Containers.py
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