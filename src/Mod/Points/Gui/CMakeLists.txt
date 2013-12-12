include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${SOQT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
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
qt4_wrap_ui(Dialogs_UIC_HDRS ${Dialog_UIC_SRCS})
SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    DlgPointsRead.ui
    DlgPointsReadImp.cpp
    DlgPointsReadImp.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

qt4_add_resources(Resource_SRCS Resources/Points.qrc)
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

if(MSVC)
    set_target_properties(PointsGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(PointsGui PROPERTIES DEBUG_OUTPUT_NAME "PointsGui_d")
    set_target_properties(PointsGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Points)
    set_target_properties(PointsGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(PointsGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(PointsGui PROPERTIES DEBUG_OUTPUT_NAME "PointsGui_d")
    set_target_properties(PointsGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Points)
    set_target_properties(PointsGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(PointsGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Points)
    set_target_properties(PointsGui PROPERTIES PREFIX "")
    set_target_properties(PointsGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS PointsGui DESTINATION lib)
