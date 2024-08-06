if(BUILD_GUI)
    pyside_wrap_rc(Help_QRC_SRCS Resources/Help.qrc)
endif(BUILD_GUI)

set(Help_SRCS InitGui.py default.css Help.py dlgPreferencesHelp.ui)

source_group("" FILES ${Help_SRCS})

add_custom_target(Help ALL SOURCES ${Help_SRCS} ${Help_QRC_SRCS})

fc_copy_sources(Help "${CMAKE_BINARY_DIR}/Mod/Help" ${Help_SRCS})

if(BUILD_GUI)
    fc_target_copy_resource(Help ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR}/Mod/Help
                            Help_rc.py)
endif(BUILD_GUI)

install(FILES ${Help_SRCS} ${Help_QRC_SRCS} DESTINATION Mod/Help)
