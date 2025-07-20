include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(SurfaceGui_LIBS
    Surface
    PartGui
    FreeCADGui
)

qt_add_resources(SurfaceGui_QRC_SRCS Resources/Surface.qrc)

SET(SurfaceGui_UIC_SRCS
    TaskFilling.ui
    TaskFillingEdge.ui
    TaskFillingVertex.ui
    TaskGeomFillSurface.ui
    TaskSections.ui
    Blending/TaskBlendCurve.ui
)

SET(BlendingGui_SRCS
    Blending/TaskBlendCurve.cpp
    Blending/TaskBlendCurve.h
    Blending/ViewProviderBlendCurve.cpp
    Blending/ViewProviderBlendCurve.h
)

SET(SurfaceGui_SRCS
    ${SurfaceGui_QRC_SRCS}
    ${SurfaceGui_UIC_HDRS}
    ${BlendingGui_SRCS}
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

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${SurfaceGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(SurfaceGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

link_directories(${OCC_LIBRARY_DIR})
add_library(SurfaceGui SHARED ${SurfaceGui_SRCS} ${SurfaceGuiIcon_SVG})
target_link_libraries(SurfaceGui ${SurfaceGui_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(SurfaceGui)
endif()

SET_BIN_DIR(SurfaceGui SurfaceGui /Mod/Surface)
SET_PYTHON_PREFIX_SUFFIX(SurfaceGui)

fc_copy_sources(SurfaceGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Surface" ${SurfaceGuiIcon_SVG})

install(TARGETS SurfaceGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${SurfaceGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Surface/Resources/icons")
