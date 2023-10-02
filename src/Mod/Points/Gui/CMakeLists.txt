include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)

set(PointsGui_LIBS
    Points
    FreeCADGui
)

set(Dialog_UIC_SRCS
    DlgPointsRead.ui
)

SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    DlgPointsRead.ui
    DlgPointsReadImp.cpp
    DlgPointsReadImp.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

set (Points_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Points_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Points_TR_QRC} ${QM_SRCS})
qt_add_resources(Resource_SRCS Resources/Points.qrc ${Points_TR_QRC})

SET(Resource_SRCS
    ${Resource_SRCS}
    Resources/Points.qrc
)
SOURCE_GROUP("Resource" FILES ${Resource_SRCS})

SET(PointsGui_SRCS
    ${Dialogs_SRCS}
    ${Resource_SRCS}
    AppPointsGui.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    ViewProvider.cpp
    ViewProvider.h
    Workbench.cpp
    Workbench.h
)

set(PointsGui_Scripts
    ../InitGui.py
)

SET(PointsGuiIcon_SVG
    Resources/icons/PointsWorkbench.svg
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${PointsGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(PointsGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(PointsGui SHARED ${PointsGui_SRCS} ${PointsGui_Scripts} ${PointsGuiIcon_SVG})
target_link_libraries(PointsGui ${PointsGui_LIBS})

fc_target_copy_resource_flat(PointsGui
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Points
    ${PointsGui_Scripts})

SET_BIN_DIR(PointsGui PointsGui /Mod/Points)
SET_PYTHON_PREFIX_SUFFIX(PointsGui)

fc_copy_sources(PointsGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Points" ${PointsGuiIcon_SVG})

INSTALL(TARGETS PointsGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

INSTALL(FILES ${PointsGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Points/Resources/icons")
