IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Tux_QRC_SRCS Resources/Tux.qrc)
ENDIF (BUILD_GUI)

SET(Tux_SRCS
    InitGui.py
    NavigationIndicatorGui.py
    PersistentToolbars.py
    PersistentToolbarsGui.py
)
SOURCE_GROUP("" FILES ${Tux_SRCS})

ADD_CUSTOM_TARGET(Tux ALL
    SOURCES ${Tux_SRCS} ${Tux_QRC_SRCS}
)

fc_copy_sources(Tux "${CMAKE_BINARY_DIR}/Mod/Tux" ${Tux_SRCS})

IF (BUILD_GUI)
    fc_target_copy_resource(Tux
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/Tux
        Tux_rc.py)
ENDIF (BUILD_GUI)

INSTALL(
    FILES
        ${Tux_SRCS}
        ${Tux_QRC_SRCS}
    DESTINATION
        Mod/Tux
)
