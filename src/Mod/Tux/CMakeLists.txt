# SPDX-License-Identifier: LGPL-2.1-or-later

IF (BUILD_GUI)
    PYSIDE_WRAP_RC(
        RELATIVE Tux_QRC_SRCS_REL
        ABSOLUTE Tux_QRC_SRCS
        RESOURCES Resources/Tux.qrc
    )
ENDIF (BUILD_GUI)

SET(Tux_SRCS
    InitGui.py
    NavigationIndicatorGui.py
    PersistentToolbars.py
    PersistentToolbarsGui.py
)
SOURCE_GROUP("" FILES ${Tux_SRCS})

ADD_CUSTOM_TARGET(Tux ALL
    DEPENDS ${Tux_QRC_SRCS}
    SOURCES ${Tux_SRCS}
)

fc_copy_sources(Tux "${CMAKE_BINARY_DIR}/Mod/Tux" ${Tux_SRCS})

IF (BUILD_GUI)
    fc_target_copy_resource(Tux
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_BINARY_DIR}/Mod/Tux"
        ${Tux_QRC_SRCS_REL})
ENDIF (BUILD_GUI)

INSTALL(
    FILES
        ${Tux_SRCS}
        ${Tux_QRC_SRCS}
    DESTINATION
        Mod/Tux
)
