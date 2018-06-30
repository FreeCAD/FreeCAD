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

set(PointsGui_MOC_HDRS
    DlgPointsReadImp.h
)
fc_wrap_cpp(PointsGui_MOC_SRCS ${PointsGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${PointsGui_MOC_SRCS})

set(Dialog_UIC_SRCS
    DlgPointsRead.ui
)

if (BUILD_QT5)
    qt5_wrap_ui(Dialogs_UIC_HDRS ${Dialog_UIC_SRCS})
else()
    qt4_wrap_ui(Dialogs_UIC_HDRS ${Dialog_UIC_SRCS})
endif()

SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    DlgPointsRead.ui
    DlgPointsReadImp.cpp
    DlgPointsReadImp.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

if (BUILD_QT5)
    qt5_add_resources(Resource_SRCS Resources/Points.qrc)
else()
    qt4_add_resources(Resource_SRCS Resources/Points.qrc)
endif()

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

add_library(PointsGui SHARED ${PointsGui_SRCS})
target_link_libraries(PointsGui ${PointsGui_LIBS})


fc_target_copy_resource(PointsGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Points
    ${CMAKE_BINARY_DIR}/Mod/Points
    InitGui.py)

SET_BIN_DIR(PointsGui PointsGui /Mod/Points)
SET_PYTHON_PREFIX_SUFFIX(PointsGui)

INSTALL(TARGETS PointsGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

SET(PointsGuiIcon_SVG
    Resources/icons/PointsWorkbench.svg
)

fc_copy_sources(PointsGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Points" ${PointsGuiIcon_SVG})

INSTALL(FILES ${PointsGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Points/Resources/icons")
