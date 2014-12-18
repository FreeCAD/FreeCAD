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
    ${COIN3D_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${SMESH_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})



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
    Hypothesis.h
    TaskObjectName.h
    TaskCreateNodeSet.h
    TaskDlgCreateNodeSet.h
    TaskFemConstraint.h
    TaskFemConstraintBearing.h
    TaskFemConstraintFixed.h  
    TaskFemConstraintForce.h  
    TaskFemConstraintGear.h   
    TaskFemConstraintPulley.h
    TaskTetParameter.h
    TaskAnalysisInfo.h
    TaskDriver.h
    TaskDlgAnalysis.h
    TaskDlgMeshShapeNetgen.h
)
fc_wrap_cpp(FemGui_MOC_SRCS ${FemGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${FemGui_MOC_SRCS})

set(FemGui_UIC_SRCS
    Hypothesis.ui
    TaskCreateNodeSet.ui
    TaskObjectName.ui
    TaskFemConstraint.ui
    TaskFemConstraintBearing.ui
    TaskFemConstraintFixed.ui
    TaskFemConstraintForce.ui
    TaskTetParameter.ui
    TaskAnalysisInfo.ui
    TaskDriver.ui
)
qt4_wrap_ui(FemGui_UIC_HDRS ${FemGui_UIC_SRCS})

SET(FemGui_DLG_SRCS
    ${FemGui_UIC_HDRS}
    Hypothesis.ui
    Hypothesis.cpp
    Hypothesis.h
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
    TaskFemConstraintGear.cpp
    TaskFemConstraintGear.h
    TaskFemConstraintPulley.cpp
    TaskFemConstraintPulley.h
)
SOURCE_GROUP("Constraint-Dialogs" FILES ${FemGui_DLG_SRCS})

qt4_add_resources(FemResource_SRCS Resources/Fem.qrc)

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
    ViewProviderFemConstraintGear.cpp
    ViewProviderFemConstraintGear.h
    ViewProviderFemConstraintPulley.cpp
    ViewProviderFemConstraintPulley.h
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
    ActiveAnalysisObserver.cpp
    ActiveAnalysisObserver.h
    Command.cpp
    Resources/Fem.qrc
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)
SOURCE_GROUP("Module" FILES ${FemGui_SRCS_Module})

SET(FemGui_SRCS
    ${Python_SRCS}
    ${FemGui_DLG_SRCS}
    ${FemResource_SRCS}
    ${FemGui_SRCS_ViewProvider}
    ${FemGui_SRCS_TaskDlg}
    ${FemGui_SRCS_TaskBoxes}
    ${FemGui_SRCS_Module}
)


add_library(FemGui SHARED ${FemGui_SRCS})
target_link_libraries(FemGui ${FemGui_LIBS})


fc_target_copy_resource(FemGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Fem
    ${CMAKE_BINARY_DIR}/Mod/Fem
    InitGui.py
    MechanicalMaterial.ui
    MechanicalAnalysis.ui
    )

SET_BIN_DIR(FemGui FemGui /Mod/Fem)
SET_PYTHON_PREFIX_SUFFIX(FemGui)

INSTALL(TARGETS FemGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
