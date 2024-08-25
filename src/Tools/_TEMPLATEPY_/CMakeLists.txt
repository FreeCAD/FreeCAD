if (BUILD_GUI)
    pyside_wrap_rc (_TEMPLATEPY__QRC_SRCS Resources/_TEMPLATEPY_.qrc)
endif (BUILD_GUI)

set (_TEMPLATEPY__SRCS Init.py InitGui.py _TEMPLATEPY_.py _TEMPLATEPY_Gui.py)
source_group ("" FILES ${_TEMPLATEPY__SRCS})

set (_TEMPLATEPY_GuiIcon_SVG Resources/icons/_TEMPLATEPY_Workbench.svg)

add_custom_target (_TEMPLATEPY_ ALL SOURCES ${_TEMPLATEPY__SRCS} ${_TEMPLATEPY__QRC_SRCS}
                                            ${_TEMPLATEPY_GuiIcon_SVG})

fc_copy_sources (_TEMPLATEPY_ "${CMAKE_BINARY_DIR}/Mod/_TEMPLATEPY_" ${_TEMPLATEPY__SRCS})
fc_copy_sources (_TEMPLATEPY_ "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/_TEMPLATEPY_"
                 ${_TEMPLATEPY_GuiIcon_SVG})

if (BUILD_GUI)
    fc_target_copy_resource (_TEMPLATEPY_ ${CMAKE_CURRENT_BINARY_DIR}
                             ${CMAKE_BINARY_DIR}/Mod/_TEMPLATEPY_ _TEMPLATEPY__rc.py)
endif (BUILD_GUI)

install (FILES ${_TEMPLATEPY__SRCS} ${_TEMPLATEPY__QRC_SRCS} DESTINATION Mod/_TEMPLATEPY_)

install (FILES ${_TEMPLATEPY_GuiIcon_SVG} DESTINATION "Mod/_TEMPLATEPY_/Resources/icons")
