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
    ${COIN3D_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${SOQT_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

set(SurfaceGui_LIBS
    Surface
    FreeCADGui
)

qt4_add_resources(Surface_QRC_SRCS Resources/Surface.qrc)

SET(SurfaceGui_SRCS
    ${Surface_QRC_SRCS}
    AppSurfaceGui.cpp
    AppSurfaceGuiPy.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
#    ViewProviderCut.cpp
#    ViewProviderCut.h
)

link_directories(${OCC_LIBRARY_DIR})
add_library(SurfaceGui SHARED ${SurfaceGui_SRCS})
target_link_libraries(SurfaceGui ${SurfaceGui_LIBS})

fc_target_copy_resource(SurfaceGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Surface
    ${CMAKE_BINARY_DIR}/Mod/Surface
    InitGui.py)

SET_BIN_DIR(SurfaceGui SurfaceGui /Mod/Surface)
SET_PYTHON_PREFIX_SUFFIX(SurfaceGui)
    
install(TARGETS SurfaceGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
