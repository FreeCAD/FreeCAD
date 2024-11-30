IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Help_QRC_SRCS Resources/Help.qrc)
ENDIF (BUILD_GUI)

SET(Help_SRCS
    InitGui.py
    default.css
    Help.py
    dlgPreferencesHelp.ui
)

SOURCE_GROUP("" FILES ${Help_SRCS})

ADD_CUSTOM_TARGET(Help ALL SOURCES ${Help_SRCS} ${Help_QRC_SRCS})

fc_copy_sources(Help "${CMAKE_BINARY_DIR}/Mod/Help" ${Help_SRCS})

IF (BUILD_GUI)
    fc_target_copy_resource(Help
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/Help
        Help_rc.py)
ENDIF (BUILD_GUI)

INSTALL(FILES ${Help_SRCS} ${Help_QRC_SRCS} DESTINATION Mod/Help)
