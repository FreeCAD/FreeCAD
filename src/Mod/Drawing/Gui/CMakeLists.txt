if(MSVC)
    add_definitions(-DHAVE_ATANH -DHAVE_ASINH -DHAVE_ACOSH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${QT_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)
link_directories(${OCC_LIBRARY_DIR})

set(DrawingGui_LIBS
    Drawing
    FreeCADGui
)


set(DrawingGui_MOC_HDRS
    DrawingView.h
    TaskOrthoViews.h
    TaskDialog.h
)




fc_wrap_cpp(DrawingGui_MOC_SRCS ${DrawingGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${DrawingGui_MOC_SRCS})

qt4_add_resources(DrawingGui_SRCS Resources/Drawing.qrc)

set(DrawingGui_UIC_SRCS
    TaskOrthoViews.ui
)

qt4_wrap_ui(DrawingGui_UIC_HDRS ${DrawingGui_UIC_SRCS})

SET(DrawingGui_SRCS
    ${DrawingGui_SRCS}
    AppDrawingGui.cpp
    AppDrawingGuiPy.cpp
    Command.cpp
    Resources/Drawing.qrc
    PreCompiled.cpp
    PreCompiled.h
    TaskDialog.cpp
    TaskDialog.h
    TaskOrthoViews.ui
    TaskOrthoViews.cpp
    TaskOrthoViews.h
    Workbench.cpp
    Workbench.h
)
SET(DrawingGuiView_SRCS
    DrawingView.cpp
    DrawingView.h
)
SET(DrawingGuiViewProvider_SRCS
    ViewProviderPage.cpp
    ViewProviderPage.h
    ViewProviderView.cpp
    ViewProviderView.h
)

SOURCE_GROUP("Mod" FILES ${DrawingGui_SRCS})
SOURCE_GROUP("SVG-View" FILES ${DrawingGuiView_SRCS})
SOURCE_GROUP("ViewProvider" FILES ${DrawingGuiViewProvider_SRCS})

SET(DrawingGuiTaskDlgs_SRCS
    TaskOrthoViews.ui
)
SOURCE_GROUP("TaskDialogs" FILES ${DrawingGuiTaskDlgs_SRCS})

if(MSVC)
    #add_definitions(-D_PreComp_)
    #GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${DrawingGui_SRCS} ${DrawingGuiView_SRCS} ${DrawingGuiViewProvider_SRCS})
    #ADD_MSVC_PRECOMPILED_HEADER(DrawingGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(MSVC)

add_library(DrawingGui SHARED ${DrawingGui_SRCS} ${DrawingGuiView_SRCS} ${DrawingGuiViewProvider_SRCS})
target_link_libraries(DrawingGui ${DrawingGui_LIBS})


fc_target_copy_resource(DrawingGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Drawing
    ${CMAKE_BINARY_DIR}/Mod/Drawing
    InitGui.py)

SET_BIN_DIR(DrawingGui DrawingGui /Mod/Drawing)
SET_PYTHON_PREFIX_SUFFIX(DrawingGui)

INSTALL(TARGETS DrawingGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

SET(DrawingGuiIcon_SVG
    Resources/icons/DrawingWorkbench.svg
)

fc_copy_sources(DrawingGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Drawing" ${DrawingGuiIcon_SVG})

INSTALL(FILES ${DrawingGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Drawing/Resources/icons")
