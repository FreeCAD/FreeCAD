# SPDX-License-Identifier: LGPL-2.1-or-later

IF (BUILD_GUI)
    PYSIDE_WRAP_RC(
        RELATIVE Help_QRC_SRCS_REL
        ABSOLUTE Help_QRC_SRCS
        RESOURCES Resources/Help.qrc
    )
ENDIF (BUILD_GUI)

SET(Help_SRCS
    InitGui.py
    default.css
    Help.py
    dlgPreferencesHelp.ui
)

SOURCE_GROUP("" FILES ${Help_SRCS})

ADD_CUSTOM_TARGET(Help ALL DEPENDS ${Help_QRC_SRCS} SOURCES ${Help_SRCS})

fc_copy_sources(Help "${CMAKE_BINARY_DIR}/Mod/Help" ${Help_SRCS})

IF (BUILD_GUI)
    fc_target_copy_resource(Help
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_BINARY_DIR}/Mod/Help"
        ${Help_QRC_SRCS_REL})
ENDIF (BUILD_GUI)

INSTALL(FILES ${Help_SRCS} ${Help_QRC_SRCS} DESTINATION Mod/Help)
