if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

if(FREECAD_BUILD_FEM_NETGEN)
    add_definitions(-DHAVE_NETGEN)
endif(FREECAD_BUILD_FEM_NETGEN)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(MeshPartGui_LIBS
    MeshPart
    FreeCADGui
)

set(MeshPartGui_MOC_HDRS
    Tessellation.h
)
fc_wrap_cpp(MeshPartGui_MOC_SRCS ${MeshPartGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${MeshPartGui_MOC_SRCS})

qt4_add_resources(MeshPartGui_QRC_SRCS Resources/MeshPart.qrc)

set(MeshPartGui_UIC_SRCS
    Tessellation.ui
)
qt4_wrap_ui(MeshPartGui_UIC_HDRS ${MeshPartGui_UIC_SRCS})

SET(MeshPartGui_SRCS
    ${MeshPartGui_QRC_SRCS}
    ${MeshPartGui_UIC_HDRS}
    AppMeshPartGui.cpp
    AppMeshPartGuiPy.cpp
    Command.cpp
    Resources/MeshPart.qrc
    PreCompiled.cpp
    PreCompiled.h
    Tessellation.ui
    Tessellation.cpp
    Tessellation.h
    Workbench.cpp
    Workbench.h
)

add_library(MeshPartGui SHARED ${MeshPartGui_SRCS})
target_link_libraries(MeshPartGui ${MeshPartGui_LIBS})


fc_target_copy_resource(MeshPartGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/MeshPart
    ${CMAKE_BINARY_DIR}/Mod/MeshPart
    InitGui.py)

if(MSVC)
    set_target_properties(MeshPartGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(MeshPartGui PROPERTIES DEBUG_OUTPUT_NAME "MeshPartGui_d")
    set_target_properties(MeshPartGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/MeshPart)
    set_target_properties(MeshPartGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(MeshPartGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(MeshPartGui PROPERTIES DEBUG_OUTPUT_NAME "MeshPartGui_d")
    set_target_properties(MeshPartGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/MeshPart)
    set_target_properties(MeshPartGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(MeshPartGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/MeshPart)
    set_target_properties(MeshPartGui PROPERTIES PREFIX "")
    set_target_properties(MeshPartGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS MeshPartGui DESTINATION lib)
