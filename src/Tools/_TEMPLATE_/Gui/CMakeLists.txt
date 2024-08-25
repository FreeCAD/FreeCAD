include_directories (
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS})

set (_TEMPLATE_Gui_LIBS _TEMPLATE_ FreeCADGui)

qt_add_resources (_TEMPLATE__QRC_SRCS Resources/_TEMPLATE_.qrc)

set (
    _TEMPLATE_Gui_SRCS
    ${_TEMPLATE__QRC_SRCS}
    App_TEMPLATE_Gui.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h)

add_library (_TEMPLATE_Gui SHARED ${_TEMPLATE_Gui_SRCS})
target_link_libraries (_TEMPLATE_Gui ${_TEMPLATE_Gui_LIBS})

set_bin_dir (_TEMPLATE_Gui _TEMPLATE_Gui /Mod/_TEMPLATE_)
set_python_prefix_suffix (_TEMPLATE_Gui)

install (TARGETS _TEMPLATE_Gui DESTINATION ${CMAKE_INSTALL_LIBDIR})
