include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
)

set(StartGui_LIBS
    Start
    FreeCADGui
)

set(StartGui_UIC_SRCS
    DlgStartPreferences.ui
)

if(BUILD_QT5)
    qt5_add_resources(Start_QRC_SRCS Resources/Start.qrc)
    qt5_wrap_ui(StartGui_UIC_HDRS ${StartGui_UIC_SRCS})
else()
    qt4_add_resources(Start_QRC_SRCS Resources/Start.qrc)
    qt4_wrap_ui(StartGui_UIC_HDRS ${StartGui_UIC_SRCS})
endif()

SET(StartGui_SRCS
    ${Start_QRC_SRCS}
    ${StartGui_UIC_SRCS}
    DlgStartPreferences.ui
    DlgStartPreferencesImp.cpp
    DlgStartPreferencesImp.h
    AppStartGui.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

SET(StartGuiIcon_SVG
    Resources/icons/StartWorkbench.svg
)

add_library(StartGui SHARED ${StartGui_SRCS} ${StartGuiIcon_SVG})
target_link_libraries(StartGui ${StartGui_LIBS})

SET_BIN_DIR(StartGui StartGui /Mod/Start)
SET_PYTHON_PREFIX_SUFFIX(StartGui)

fc_copy_sources(StartGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Start" ${StartGuiIcon_SVG})

INSTALL(TARGETS StartGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${StartGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Start/Resources/icons")
