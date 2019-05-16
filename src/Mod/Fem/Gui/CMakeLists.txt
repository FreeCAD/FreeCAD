if(MSVC)
    add_definitions(-DFCGuiFem -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)


if(BUILD_FEM_NETGEN)
    add_definitions(-DFCWithNetgen)
endif(BUILD_FEM_NETGEN)


include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${SMESH_INCLUDE_DIR}
    ${VTK_INCLUDE_DIRS}
)

link_directories(${OCC_LIBRARY_DIR})
link_directories(${SMESH_LIB_PATH})


set(FemGui_LIBS
    Fem
    FreeCADGui
)


generate_from_xml(ViewProviderFemMeshPy)


SET(Python_SRCS
    ViewProviderFemMeshPy.xml
    ViewProviderFemMeshPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})


set(FemGui_MOC_HDRS
    DlgSettingsFemCcxImp.h
    DlgSettingsFemElmerImp.h
    DlgSettingsFemExportAbaqusImp.h
    DlgSettingsFemGeneralImp.h
    DlgSettingsFemGmshImp.h
    DlgSettingsFemInOutVtkImp.h
    DlgSettingsFemMaterialImp.h
    DlgSettingsFemZ88Imp.h
    PropertyFemMeshItem.h
    TaskObjectName.h
    TaskCreateNodeSet.h
    TaskDlgCreateNodeSet.h
    TaskFemConstraint.h
    TaskFemConstraintBearing.h
    TaskFemConstraintFixed.h
    TaskFemConstraintForce.h
    TaskFemConstraintFluidBoundary.h
    TaskFemConstraintPressure.h
    TaskFemConstraintGear.h
    TaskFemConstraintPulley.h
    TaskFemConstraintDisplacement.h
    TaskFemConstraintTemperature.h
    TaskFemConstraintHeatflux.h
    TaskFemConstraintInitialTemperature.h
    TaskFemConstraintPlaneRotation.h
    TaskFemConstraintContact.h
    TaskFemConstraintTransform.h
    TaskTetParameter.h
    TaskAnalysisInfo.h
    TaskDriver.h
    TaskDlgAnalysis.h
    TaskDlgMeshShapeNetgen.h
)
if(BUILD_FEM_VTK)
    set(FemGui_MOC_HDRS
        ${FemGui_MOC_HDRS}
        TaskPostBoxes.h
        ViewProviderFemPostFunction.h
    )
endif(BUILD_FEM_VTK)
fc_wrap_cpp(FemGui_MOC_SRCS ${FemGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${FemGui_MOC_SRCS})


set(FemGui_UIC_SRCS
    DlgSettingsFemCcx.ui
    DlgSettingsFemElmer.ui
    DlgSettingsFemExportAbaqus.ui
    DlgSettingsFemGeneral.ui
    DlgSettingsFemGmsh.ui
    DlgSettingsFemInOutVtk.ui
    DlgSettingsFemMaterial.ui
    DlgSettingsFemZ88.ui
    TaskCreateNodeSet.ui
    TaskObjectName.ui
    TaskFemConstraint.ui
    TaskFemConstraintBearing.ui
    TaskFemConstraintFixed.ui
    TaskFemConstraintForce.ui
    TaskFemConstraintFluidBoundary.ui
    TaskFemConstraintPressure.ui
    TaskFemConstraintDisplacement.ui
    TaskFemConstraintTemperature.ui
    TaskFemConstraintHeatflux.ui
    TaskFemConstraintInitialTemperature.ui
    TaskFemConstraintPlaneRotation.ui
    TaskFemConstraintContact.ui
    TaskFemConstraintTransform.ui
    TaskTetParameter.ui
    TaskAnalysisInfo.ui
    TaskDriver.ui
)
if(BUILD_FEM_VTK)
    set(FemGui_UIC_SRCS
        ${FemGui_UIC_SRCS}
        TaskPostDisplay.ui
        TaskPostClip.ui
        TaskPostDataAlongLine.ui
        TaskPostDataAtPoint.ui
        TaskPostScalarClip.ui
        TaskPostWarpVector.ui
        TaskPostCut.ui
        PlaneWidget.ui
        SphereWidget.ui
    )
endif(BUILD_FEM_VTK)


if(BUILD_QT5)
    qt5_wrap_ui(FemGui_UIC_HDRS ${FemGui_UIC_SRCS})
else()
    qt4_wrap_ui(FemGui_UIC_HDRS ${FemGui_UIC_SRCS})
endif()


SET(FemGui_DLG_SRCS
    ${FemGui_UIC_HDRS}
    DlgSettingsFemCcx.ui
    DlgSettingsFemCcxImp.cpp
    DlgSettingsFemCcxImp.h
    DlgSettingsFemElmer.ui
    DlgSettingsFemElmerImp.cpp
    DlgSettingsFemElmerImp.h
    DlgSettingsFemExportAbaqus.ui
    DlgSettingsFemExportAbaqusImp.cpp
    DlgSettingsFemExportAbaqusImp.h
    DlgSettingsFemGeneral.ui
    DlgSettingsFemGeneralImp.cpp
    DlgSettingsFemGeneralImp.h
    DlgSettingsFemGmsh.ui
    DlgSettingsFemGmshImp.cpp
    DlgSettingsFemGmshImp.h
    DlgSettingsFemInOutVtk.ui
    DlgSettingsFemInOutVtkImp.cpp
    DlgSettingsFemInOutVtkImp.h
    DlgSettingsFemMaterial.ui
    DlgSettingsFemMaterialImp.cpp
    DlgSettingsFemMaterialImp.h
    DlgSettingsFemZ88.ui
    DlgSettingsFemZ88Imp.cpp
    DlgSettingsFemZ88Imp.h
    TaskFemConstraint.ui
    TaskFemConstraint.cpp
    TaskFemConstraint.h
    TaskFemConstraintBearing.ui
    TaskFemConstraintBearing.cpp
    TaskFemConstraintBearing.h
    TaskFemConstraintFixed.ui
    TaskFemConstraintFixed.cpp
    TaskFemConstraintFixed.h
    TaskFemConstraintForce.ui
    TaskFemConstraintForce.cpp
    TaskFemConstraintForce.h
    TaskFemConstraintFluidBoundary.ui
    TaskFemConstraintFluidBoundary.cpp
    TaskFemConstraintFluidBoundary.h
    TaskFemConstraintPressure.ui
    TaskFemConstraintPressure.cpp
    TaskFemConstraintPressure.h
    TaskFemConstraintGear.cpp
    TaskFemConstraintGear.h
    TaskFemConstraintPulley.cpp
    TaskFemConstraintPulley.h
    TaskFemConstraintDisplacement.ui
    TaskFemConstraintDisplacement.cpp
    TaskFemConstraintDisplacement.h
    TaskFemConstraintTemperature.ui
    TaskFemConstraintTemperature.cpp
    TaskFemConstraintTemperature.h
    TaskFemConstraintHeatflux.ui
    TaskFemConstraintHeatflux.cpp
    TaskFemConstraintHeatflux.h
    TaskFemConstraintInitialTemperature.ui
    TaskFemConstraintInitialTemperature.cpp
    TaskFemConstraintInitialTemperature.h
    TaskFemConstraintPlaneRotation.ui
    TaskFemConstraintPlaneRotation.cpp
    TaskFemConstraintPlaneRotation.h
    TaskFemConstraintContact.ui
    TaskFemConstraintContact.cpp
    TaskFemConstraintContact.h
    TaskFemConstraintTransform.ui
    TaskFemConstraintTransform.cpp
    TaskFemConstraintTransform.h
)
SOURCE_GROUP("Constraint-Dialogs" FILES ${FemGui_DLG_SRCS})


if(BUILD_QT5)
    qt5_add_resources(FemResource_SRCS Resources/Fem.qrc)
else()
    qt4_add_resources(FemResource_SRCS Resources/Fem.qrc)
endif()
SOURCE_GROUP("Resources" FILES ${FemResource_SRCS})


SET(FemGui_SRCS_ViewProvider
    ViewProviderFemMesh.cpp
    ViewProviderFemMesh.h
    ViewProviderFemMeshShape.cpp
    ViewProviderFemMeshShape.h
    ViewProviderFemMeshShapeNetgen.cpp
    ViewProviderFemMeshShapeNetgen.h
    ViewProviderAnalysis.cpp
    ViewProviderAnalysis.h
    ViewProviderSolver.cpp
    ViewProviderSolver.h
    ViewProviderSetNodes.cpp
    ViewProviderSetNodes.h
    ViewProviderSetElements.cpp
    ViewProviderSetElements.h
    ViewProviderSetFaces.cpp
    ViewProviderSetFaces.h
    ViewProviderSetGeometry.cpp
    ViewProviderSetGeometry.h
    FemSelectionGate.cpp
    FemSelectionGate.h
    ViewProviderFemConstraint.cpp
    ViewProviderFemConstraint.h
    ViewProviderFemConstraintBearing.cpp
    ViewProviderFemConstraintBearing.h
    ViewProviderFemConstraintFixed.cpp
    ViewProviderFemConstraintFixed.h
    ViewProviderFemConstraintForce.cpp
    ViewProviderFemConstraintForce.h
    ViewProviderFemConstraintFluidBoundary.cpp
    ViewProviderFemConstraintFluidBoundary.h
    ViewProviderFemConstraintPressure.cpp
    ViewProviderFemConstraintPressure.h
    ViewProviderFemConstraintGear.cpp
    ViewProviderFemConstraintGear.h
    ViewProviderFemConstraintPulley.cpp
    ViewProviderFemConstraintPulley.h
    ViewProviderFemConstraintDisplacement.cpp
    ViewProviderFemConstraintDisplacement.h
    ViewProviderFemConstraintTemperature.cpp
    ViewProviderFemConstraintTemperature.h
    ViewProviderFemConstraintHeatflux.cpp
    ViewProviderFemConstraintHeatflux.h
    ViewProviderFemConstraintInitialTemperature.cpp
    ViewProviderFemConstraintInitialTemperature.h
    ViewProviderFemConstraintPlaneRotation.cpp
    ViewProviderFemConstraintPlaneRotation.h
    ViewProviderFemConstraintContact.cpp
    ViewProviderFemConstraintContact.h
    ViewProviderFemConstraintTransform.cpp
    ViewProviderFemConstraintTransform.h
    ViewProviderResult.cpp
    ViewProviderResult.h
)
SOURCE_GROUP("ViewProvider" FILES ${FemGui_SRCS_ViewProvider})


SET(FemGui_SRCS_TaskBoxes
    TaskObjectName.ui
    TaskObjectName.cpp
    TaskObjectName.h
    TaskCreateNodeSet.ui
    TaskCreateNodeSet.cpp
    TaskCreateNodeSet.h
    TaskDriver.ui
    TaskDriver.cpp
    TaskDriver.h
    TaskAnalysisInfo.ui
    TaskAnalysisInfo.cpp
    TaskAnalysisInfo.h
    TaskTetParameter.ui
    TaskTetParameter.cpp
    TaskTetParameter.h
)
if(BUILD_FEM_VTK)
    SET(FemGui_SRCS_TaskBoxes
        ${FemGui_SRCS_TaskBoxes}
        PlaneWidget.ui
        SphereWidget.ui
        TaskPostClip.ui
        TaskPostDataAlongLine.ui
        TaskPostDataAtPoint.ui
        TaskPostScalarClip.ui
        TaskPostDisplay.ui
        TaskPostWarpVector.ui
        TaskPostCut.ui
        TaskPostBoxes.h
        TaskPostBoxes.cpp
    )
endif(BUILD_FEM_VTK)
SOURCE_GROUP("Task_Boxes" FILES ${FemGui_SRCS_TaskBoxes})


SET(FemGui_SRCS_TaskDlg
    TaskDlgCreateNodeSet.h
    TaskDlgCreateNodeSet.cpp
    TaskDlgMeshShapeNetgen.h
    TaskDlgMeshShapeNetgen.cpp
    TaskDlgAnalysis.h
    TaskDlgAnalysis.cpp
)
SOURCE_GROUP("Task_Dialogs" FILES ${FemGui_SRCS_TaskDlg})


SET(FemGui_SRCS_Module
    AppFemGui.cpp
    AppFemGuiPy.cpp
    AbaqusHighlighter.cpp
    AbaqusHighlighter.h
    ActiveAnalysisObserver.cpp
    ActiveAnalysisObserver.h
    Command.cpp
    Resources/Fem.qrc
    PreCompiled.cpp
    PreCompiled.h
    PropertyFemMeshItem.cpp
    PropertyFemMeshItem.h
    Workbench.cpp
    Workbench.h
)
SOURCE_GROUP("Module" FILES ${FemGui_SRCS_Module})


if(BUILD_FEM_VTK)
    SET(FemGui_SRCS_Post
        ViewProviderFemPostObject.h
        ViewProviderFemPostObject.cpp
        ViewProviderFemPostPipeline.h
        ViewProviderFemPostPipeline.cpp
        ViewProviderFemPostFunction.h
        ViewProviderFemPostFunction.cpp
        ViewProviderFemPostFilter.h
        ViewProviderFemPostFilter.cpp
        )
    SOURCE_GROUP("PostObjects" FILES ${FemGui_SRCS_Post})
endif(BUILD_FEM_VTK)


SET(FemGui_SRCS
    ${Python_SRCS}
    ${FemGui_DLG_SRCS}
    ${FemResource_SRCS}
    ${FemGui_SRCS_ViewProvider}
    ${FemGui_SRCS_TaskDlg}
    ${FemGui_SRCS_TaskBoxes}
    ${FemGui_SRCS_Module}
    ${FemGui_SRCS_Post}
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${FemGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(FemGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

SET(FemGuiIcon_SVG
    Resources/icons/FemWorkbench.svg
)

add_library(FemGui SHARED ${FemGui_SRCS} ${FemGuiIcon_SVG})
target_link_libraries(FemGui ${FemGui_LIBS} ${VTK_LIBRARIES})


fc_copy_sources(FemGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Fem" ${FemGuiIcon_SVG})


INSTALL(FILES ${FemGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Fem/Resources/icons")


# Python modules ui files, they are copied as they are, thus the need not to be added to Fem.qrc
# see https://forum.freecadweb.org/viewtopic.php?f=10&t=25833
SET(FemGuiPythonUI_SRCS
    Resources/ui/ElectrostaticPotential.ui
    Resources/ui/ElementFluid1D.ui
    Resources/ui/ElementGeometry1D.ui
    Resources/ui/ElementGeometry2D.ui
    Resources/ui/ElementRotation1D.ui
    Resources/ui/FlowVelocity.ui
    Resources/ui/InitialFlowVelocity.ui
    Resources/ui/Material.ui
    Resources/ui/MeshBoundaryLayer.ui
    Resources/ui/MeshGmsh.ui
    Resources/ui/MeshGroup.ui
    Resources/ui/MeshGroupXDMFExport.ui
    Resources/ui/MeshRegion.ui
    Resources/ui/ResultShow.ui
    Resources/ui/SolverCalculix.ui
)

ADD_CUSTOM_TARGET(FemPythonUi ALL
    SOURCES ${FemGuiPythonUI_SRCS}
)

fc_copy_sources(FemPythonUi "${CMAKE_BINARY_DIR}/Mod/Fem" ${FemGuiPythonUI_SRCS})

INSTALL(FILES ${FemGuiPythonUI_SRCS} DESTINATION Mod/Fem/Resources/ui)



SET_BIN_DIR(FemGui FemGui /Mod/Fem)
SET_PYTHON_PREFIX_SUFFIX(FemGui)


INSTALL(TARGETS FemGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
