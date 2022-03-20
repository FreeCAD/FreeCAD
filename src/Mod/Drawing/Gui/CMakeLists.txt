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
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(DrawingGui_LIBS
    Drawing
    FreeCADGui
)

set(DrawingGui_UIC_SRCS
    TaskOrthoViews.ui
)

if(BUILD_QT5)
    qt5_add_resources(DrawingGui_SRCS Resources/Drawing.qrc)
    qt5_wrap_ui(DrawingGui_UIC_HDRS ${DrawingGui_UIC_SRCS})
else()
    qt4_add_resources(DrawingGui_SRCS Resources/Drawing.qrc)
    qt4_wrap_ui(DrawingGui_UIC_HDRS ${DrawingGui_UIC_SRCS})
endif()

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

SET(DrawingGuiIcon_SVG
    Resources/icons/DrawingWorkbench.svg
)

add_library(DrawingGui SHARED ${DrawingGui_SRCS} ${DrawingGuiView_SRCS}
                              ${DrawingGuiViewProvider_SRCS} ${DrawingGuiIcon_SVG})
target_link_libraries(DrawingGui ${DrawingGui_LIBS})


SET_BIN_DIR(DrawingGui DrawingGui /Mod/Drawing)
SET_PYTHON_PREFIX_SUFFIX(DrawingGui)

fc_copy_sources(DrawingGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Drawing" ${DrawingGuiIcon_SVG})

INSTALL(TARGETS DrawingGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${DrawingGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Drawing/Resources/icons")
