include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}/Inventor/annex
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)

if(MSVC)
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/3rdParty/OpenGL/api
    )
endif(MSVC)

set(MeshGui_LIBS
    ${Boost_LIBRARIES}
    ${OPENGL_glu_LIBRARY}
    Mesh
    FreeCADGui
)

generate_from_xml(ViewProviderMeshPy)

SET(MeshGui_XML_SRCS
    ViewProviderMeshPy.xml
)
SOURCE_GROUP("XML" FILES ${MeshGui_XML_SRCS})

set(Dialogs_UIC_SRCS
    DlgEvaluateMesh.ui
    DlgEvaluateSettings.ui
    DlgRegularSolid.ui
    DlgSettingsMeshView.ui
    DlgSettingsImportExport.ui
    DlgDecimating.ui
    DlgSmoothing.ui
    RemoveComponents.ui
    RemeshGmsh.ui
    Segmentation.ui
    SegmentationBestFit.ui
    Selection.ui
)

SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    DlgEvaluateMesh.ui
    DlgEvaluateMeshImp.cpp
    DlgEvaluateMeshImp.h
    DlgEvaluateSettings.ui
    DlgEvaluateSettings.cpp
    DlgEvaluateSettings.h
    DlgRegularSolid.ui
    DlgRegularSolidImp.cpp
    DlgRegularSolidImp.h
    DlgSettingsMeshView.ui
    DlgSettingsMeshView.cpp
    DlgSettingsMeshView.h
    DlgSettingsImportExport.ui
    DlgSettingsImportExportImp.cpp
    DlgSettingsImportExportImp.h
    DlgDecimating.ui
    DlgDecimating.cpp
    DlgDecimating.h
    DlgSmoothing.ui
    DlgSmoothing.cpp
    DlgSmoothing.h
    RemoveComponents.ui
    RemoveComponents.cpp
    RemoveComponents.h
    RemeshGmsh.ui
    RemeshGmsh.cpp
    RemeshGmsh.h
    Segmentation.ui
    Segmentation.cpp
    Segmentation.h
    SegmentationBestFit.ui
    SegmentationBestFit.cpp
    SegmentationBestFit.h
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

set (Mesh_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Mesh_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Mesh_TR_QRC} ${QM_SRCS})
qt_add_resources(resource_SRCS Resources/Mesh.qrc ${Mesh_TR_QRC})

SET(resource_SRCS
    ${QM_SRCS}
    ${resource_SRCS}
    ${Mesh_TR_QRC}
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
    ViewProviderMeshPyImp.cpp
    ViewProviderMeshFaceSet.cpp
    ViewProviderMeshFaceSet.h
    ViewProviderTransform.cpp
    ViewProviderTransform.h
    ViewProviderTransformDemolding.cpp
    ViewProviderTransformDemolding.h
)
SOURCE_GROUP("ViewProvider" FILES ${ViewProvider_SRCS})

SET(MeshGui_SRCS
    ${MeshGui_XML_SRCS}
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
    ThumbnailExtension.cpp
    ThumbnailExtension.h
    Workbench.cpp
    Workbench.h
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${MeshGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(MeshGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

SET(MeshGuiIcon_SVG
    Resources/icons/MeshWorkbench.svg
)

# Suppress this warning for the SbMatrix class
if (EIGEN3_NO_DEPRECATED_COPY)
    set_source_files_properties(
        SoFCIndexedFaceSet.cpp
        ViewProvider.cpp
        PROPERTIES COMPILE_FLAGS ${EIGEN3_NO_DEPRECATED_COPY})
endif ()

add_library(MeshGui SHARED ${MeshGui_SRCS} ${MeshGuiIcon_SVG})
target_link_libraries(MeshGui ${MeshGui_LIBS})


SET_BIN_DIR(MeshGui MeshGui /Mod/Mesh)
SET_PYTHON_PREFIX_SUFFIX(MeshGui)

fc_copy_sources(MeshGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Mesh" ${MeshGuiIcon_SVG})

INSTALL(TARGETS MeshGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${MeshGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Mesh/Resources/icons")
