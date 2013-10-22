include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)

set(MeshGui_LIBS
    ${Boost_LIBRARIES}
    ${OPENGL_glu_LIBRARY}
    Mesh
    FreeCADGui
)

set(Mesh_MOC_HDRS
    DlgEvaluateMeshImp.h
    DlgRegularSolidImp.h
    DlgSettingsMeshView.h
    DlgSmoothing.h
    MeshEditor.h
    PropertyEditorMesh.h
    RemoveComponents.h
    Selection.h
)
fc_wrap_cpp(Mesh_MOC_SRCS ${Mesh_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${Mesh_MOC_SRCS})

set(Dialogs_UIC_SRCS
    DlgEvaluateMesh.ui
    DlgRegularSolid.ui
    DlgSettingsMeshView.ui
    DlgSmoothing.ui
    RemoveComponents.ui
    Segmentation.ui
    Selection.ui
)
qt4_wrap_ui(Dialogs_UIC_HDRS ${Dialogs_UIC_SRCS})
SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    DlgEvaluateMesh.ui
    DlgEvaluateMeshImp.cpp
    DlgEvaluateMeshImp.h
    DlgRegularSolid.ui
    DlgRegularSolidImp.cpp
    DlgRegularSolidImp.h
    DlgSettingsMeshView.ui
    DlgSettingsMeshView.cpp
    DlgSettingsMeshView.h
    DlgSmoothing.ui
    DlgSmoothing.cpp
    DlgSmoothing.h
    RemoveComponents.ui
    RemoveComponents.cpp
    RemoveComponents.h
    Segmentation.ui
    Segmentation.cpp
    Segmentation.h
    Selection.ui
    Selection.cpp
    Selection.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

SET(Inventor_SRCS
    SoFCIndexedFaceSet.cpp
    SoFCIndexedFaceSet.h
    SoFCMeshObject.cpp
    SoFCMeshObject.h
    SoPolygon.cpp
    SoPolygon.h
)
SOURCE_GROUP("Inventor" FILES ${Inventor_SRCS})

qt4_add_resources(resource_SRCS Resources/Mesh.qrc)
SET(resource_SRCS
    ${resource_SRCS}
    Resources/Mesh.qrc
)
SOURCE_GROUP("resource" FILES ${resource_SRCS})

SET(ViewProvider_SRCS
    ViewProvider.cpp
    ViewProvider.h
    ViewProviderPython.cpp
    ViewProviderPython.h
    ViewProviderCurvature.cpp
    ViewProviderCurvature.h
    ViewProviderDefects.cpp
    ViewProviderDefects.h
    ViewProviderMeshFaceSet.cpp
    ViewProviderMeshFaceSet.h
    ViewProviderTransform.cpp
    ViewProviderTransform.h
    ViewProviderTransformDemolding.cpp
    ViewProviderTransformDemolding.h
)
SOURCE_GROUP("ViewProvider" FILES ${ViewProvider_SRCS})

SET(MeshGui_SRCS
    ${Dialogs_SRCS}
    ${Inventor_SRCS}
    ${resource_SRCS}
    ${ViewProvider_SRCS}
    AppMeshGui.cpp
    Command.cpp
    Doxygen.cpp
    PreCompiled.cpp
    PreCompiled.h
    MeshEditor.cpp
    MeshEditor.h
    MeshSelection.cpp
    MeshSelection.h
    PropertyEditorMesh.cpp
    PropertyEditorMesh.h
    Workbench.cpp
    Workbench.h
)

add_library(MeshGui SHARED ${MeshGui_SRCS})
target_link_libraries(MeshGui ${MeshGui_LIBS})


fc_target_copy_resource(MeshGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Mesh
    ${CMAKE_BINARY_DIR}/Mod/Mesh
    InitGui.py)

if(MSVC)
    set_target_properties(MeshGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(MeshGui PROPERTIES DEBUG_OUTPUT_NAME "MeshGui_d")
    set_target_properties(MeshGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Mesh)
    set_target_properties(MeshGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(MeshGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(MeshGui PROPERTIES DEBUG_OUTPUT_NAME "MeshGui_d")
    set_target_properties(MeshGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Mesh)
    set_target_properties(MeshGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(MeshGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Mesh)
    set_target_properties(MeshGui PROPERTIES PREFIX "")
    set_target_properties(MeshGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS MeshGui DESTINATION lib)
