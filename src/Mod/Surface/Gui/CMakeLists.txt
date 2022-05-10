if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
)

set(SurfaceGui_LIBS
    Surface
    PartGui
    FreeCADGui
)

if (BUILD_QT5)
    qt5_add_resources(SurfaceGui_QRC_SRCS Resources/Surface.qrc)
else()
    qt4_add_resources(SurfaceGui_QRC_SRCS Resources/Surface.qrc)
endif()

SET(SurfaceGui_UIC_SRCS
    TaskFilling.ui
    TaskFillingEdge.ui
    TaskFillingVertex.ui
    TaskGeomFillSurface.ui
    TaskSections.ui
)

if (BUILD_QT5)
    qt5_wrap_ui(SurfaceGui_UIC_HDRS ${SurfaceGui_UIC_SRCS})
else()
    qt4_wrap_ui(SurfaceGui_UIC_HDRS ${SurfaceGui_UIC_SRCS})
endif()

SET(SurfaceGui_SRCS
    ${SurfaceGui_QRC_SRCS}
    ${SurfaceGui_UIC_HDRS}
    TaskFilling.cpp
    TaskFilling.h
    TaskFillingEdge.cpp
    TaskFillingEdge.h
    TaskFillingVertex.cpp
    TaskFillingVertex.h
    TaskGeomFillSurface.cpp
    TaskGeomFillSurface.h
    TaskSections.cpp
    TaskSections.h
    AppSurfaceGui.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    ViewProviderExtend.cpp
    ViewProviderExtend.h
    Workbench.cpp
    Workbench.h
#    ViewProviderCut.cpp
#    ViewProviderCut.h
)

SET(SurfaceGuiIcon_SVG
    Resources/icons/Surface_Workbench.svg
)

link_directories(${OCC_LIBRARY_DIR})
add_library(SurfaceGui SHARED ${SurfaceGui_SRCS} ${SurfaceGuiIcon_SVG})
target_link_libraries(SurfaceGui ${SurfaceGui_LIBS})

SET_BIN_DIR(SurfaceGui SurfaceGui /Mod/Surface)
SET_PYTHON_PREFIX_SUFFIX(SurfaceGui)

fc_copy_sources(SurfaceGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Surface" ${SurfaceGuiIcon_SVG})

install(TARGETS SurfaceGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${SurfaceGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Surface/Resources/icons")

