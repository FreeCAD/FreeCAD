
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(_TEMPLATE_Gui_LIBS
    _TEMPLATE_
    FreeCADGui
)

qt4_add_resources(_TEMPLATE__QRC_SRCS Resources/_TEMPLATE_.qrc)

SET(_TEMPLATE_Gui_SRCS
    ${_TEMPLATE__QRC_SRCS}
    App_TEMPLATE_Gui.cpp
    App_TEMPLATE_GuiPy.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(_TEMPLATE_Gui SHARED ${_TEMPLATE_Gui_SRCS})
target_link_libraries(_TEMPLATE_Gui ${_TEMPLATE_Gui_LIBS})

fc_target_copy_resource(_TEMPLATE_Gui 
    ${CMAKE_SOURCE_DIR}/src/Tools/_TEMPLATE_
    ${CMAKE_BINARY_DIR}/Mod/_TEMPLATE_
    InitGui.py)

SET_BIN_DIR(_TEMPLATE_Gui _TEMPLATE_Gui /Mod/_TEMPLATE_)
SET_PYTHON_PREFIX_SUFFIX(_TEMPLATE_Gui)
    
install(TARGETS _TEMPLATE_Gui DESTINATION ${CMAKE_INSTALL_LIBDIR})
