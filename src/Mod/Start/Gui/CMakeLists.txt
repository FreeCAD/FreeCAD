include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

set(StartGui_LIBS
    Start
    FreeCADGui
)

if(BUILD_QT5)
    qt5_add_resources(Start_QRC_SRCS Resources/Start.qrc)
else()
    qt4_add_resources(Start_QRC_SRCS Resources/Start.qrc)
endif()

SET(StartGui_SRCS
    ${Start_QRC_SRCS}
    AppStartGui.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(StartGui SHARED ${StartGui_SRCS})
target_link_libraries(StartGui ${StartGui_LIBS})

fc_target_copy_resource(StartGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Start
    ${CMAKE_BINARY_DIR}/Mod/Start
    InitGui.py)

fc_target_copy_resource(StartGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Start/StartPage
    ${CMAKE_BINARY_DIR}/Mod/Start/StartPage
    ${StartPage_Scripts})

fc_target_copy_resource(StartGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Start/StartPage
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Start/StartPage
    ${StartPage_Resources})

SET_BIN_DIR(StartGui StartGui /Mod/Start)
SET_PYTHON_PREFIX_SUFFIX(StartGui)

INSTALL(TARGETS StartGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

SET(StartGuiIcon_SVG
    Resources/icons/StartWorkbench.svg
)

fc_copy_sources(StartGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Start" ${StartGuiIcon_SVG})

INSTALL(FILES ${StartGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Start/Resources/icons")
