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

set(StartGui_UIC_SRCS
    DlgStartPreferences.ui
    DlgStartPreferencesAdvanced.ui
)

set (Start_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Start_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Start_TR_QRC} ${QM_SRCS})
qt_add_resources(Start_QRC_SRCS Resources/Start.qrc ${Start_TR_QRC})

SET(StartGui_SRCS
    ${Start_QRC_SRCS}
    ${StartGui_UIC_SRCS}
    DlgStartPreferences.ui
    DlgStartPreferencesAdvanced.ui
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

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${StartGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(StartGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(StartGui SHARED ${StartGui_SRCS} ${StartGuiIcon_SVG})
target_link_libraries(StartGui ${StartGui_LIBS})

SET_BIN_DIR(StartGui StartGui /Mod/Start)
SET_PYTHON_PREFIX_SUFFIX(StartGui)

fc_copy_sources(StartGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Start" ${StartGuiIcon_SVG})

INSTALL(TARGETS StartGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${StartGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Start/Resources/icons")
