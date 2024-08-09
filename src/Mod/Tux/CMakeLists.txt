if (BUILD_GUI)
    pyside_wrap_rc (Tux_QRC_SRCS Resources/Tux.qrc)
endif (BUILD_GUI)

set (Tux_SRCS InitGui.py NavigationIndicatorGui.py PersistentToolbars.py PersistentToolbarsGui.py)
source_group ("" FILES ${Tux_SRCS})

add_custom_target (Tux ALL SOURCES ${Tux_SRCS} ${Tux_QRC_SRCS})

fc_copy_sources (Tux "${CMAKE_BINARY_DIR}/Mod/Tux" ${Tux_SRCS})

if (BUILD_GUI)
    fc_target_copy_resource (Tux ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR}/Mod/Tux Tux_rc.py)
endif (BUILD_GUI)

install (FILES ${Tux_SRCS} ${Tux_QRC_SRCS} DESTINATION Mod/Tux)
