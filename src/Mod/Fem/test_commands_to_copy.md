# modules
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_common
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_femimport
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_material
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_object
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_open
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_result
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_solverframework


# classes
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_common.TestFemCommon
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_femimport.TestFemImport
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_femimport.TestObjectExistance
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_material.TestMaterialUnits
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshCommon
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshEleTetra10
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_object.TestObjectCreate
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_object.TestObjectType
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_open.TestObjectOpen
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_result.TestResult
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_solverframework.TestSolverFrameWork


# methods
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_freq_analysis
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_static_analysis
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_force_faceload_hexa20
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_contact_shell_shell
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_contact_solid_solid
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_tie
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_static_material_multiple
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_static_material_nonlinar
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_thermomech_bimetall
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_thermomech_flow1D_analysis
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_ccxtools.TestCcxTools.test_thermomech_spine_analysis
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_common.TestFemCommon.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_common.TestFemCommon.test_adding_refshaps
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_common.TestFemCommon.test_pyimport_all_FEM_modules
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_femimport.TestFemImport.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_femimport.TestFemImport.test_import_fem
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_femimport.TestObjectExistance.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_femimport.TestObjectExistance.test_objects_existance
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_material.TestMaterialUnits.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_material.TestMaterialUnits.test_known_quantity_units
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_material.TestMaterialUnits.test_material_card_quantities
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshCommon.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshCommon.test_mesh_seg2_python
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshCommon.test_mesh_seg3_python
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshCommon.test_unv_save_load
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshCommon.test_writeAbaqus_precision
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshEleTetra10.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_create
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_inp
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_unv
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_vkt
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_yml
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_z88
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_object.TestObjectCreate.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_object.TestObjectCreate.test_femobjects_make
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_object.TestObjectType.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_object.TestObjectType.test_femobjects_type
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_object.TestObjectType.test_femobjects_isoftype
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_object.TestObjectType.test_femobjects_derivedfromfem
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_object.TestObjectType.test_femobjects_derivedfromstd
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_open.TestObjectOpen.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_open.TestObjectOpen.test_femobjects_open_head
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_open.TestObjectOpen.test_femobjects_open_de9b3fb438
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_result.TestResult.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_result.TestResult.test_read_frd_massflow_networkpressure
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_result.TestResult.test_stress_von_mises
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_result.TestResult.test_stress_principal_std
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_result.TestResult.test_stress_principal_reinforced
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_result.TestResult.test_rho
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_result.TestResult.test_disp_abs
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_solverframework.TestSolverFrameWork.test_00print
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_solverframework.TestSolverFrameWork.test_solver_calculix
make -j 4 && ./bin/FreeCADCmd -t femtest.app.test_solverframework.TestSolverFrameWork.test_solver_elmer


# methods in FreeCAD
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_freq_analysis'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_static_analysis'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_force_faceload_hexa20'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_contact_shell_shell'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_contact_solid_solid'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_tie'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_static_material_multiple'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_static_material_nonlinar'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_thermomech_bimetall'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_thermomech_flow1D_analysis'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_ccxtools.TestCcxTools.test_thermomech_spine_analysis'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_common.TestFemCommon.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_common.TestFemCommon.test_adding_refshaps'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_common.TestFemCommon.test_pyimport_all_FEM_modules'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_femimport.TestFemImport.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_femimport.TestFemImport.test_import_fem'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_femimport.TestObjectExistance.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_femimport.TestObjectExistance.test_objects_existance'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_material.TestMaterialUnits.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_material.TestMaterialUnits.test_known_quantity_units'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_material.TestMaterialUnits.test_material_card_quantities'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshCommon.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshCommon.test_mesh_seg2_python'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshCommon.test_mesh_seg3_python'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshCommon.test_unv_save_load'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshCommon.test_writeAbaqus_precision'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshEleTetra10.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_create'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_inp'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_unv'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_vkt'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_yml'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_z88'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_object.TestObjectCreate.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_object.TestObjectCreate.test_femobjects_make'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_object.TestObjectType.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_object.TestObjectType.test_femobjects_type'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_object.TestObjectType.test_femobjects_isoftype'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_object.TestObjectType.test_femobjects_derivedfromfem'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_object.TestObjectType.test_femobjects_derivedfromstd'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_open.TestObjectOpen.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_open.TestObjectOpen.test_femobjects_open_head'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_open.TestObjectOpen.test_femobjects_open_de9b3fb438'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_result.TestResult.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_result.TestResult.test_read_frd_massflow_networkpressure'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_result.TestResult.test_stress_von_mises'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_result.TestResult.test_stress_principal_std'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_result.TestResult.test_stress_principal_reinforced'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_result.TestResult.test_rho'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_result.TestResult.test_disp_abs'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_solverframework.TestSolverFrameWork.test_00print'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_solverframework.TestSolverFrameWork.test_solver_calculix'
))

unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
    'femtest.app.test_solverframework.TestSolverFrameWork.test_solver_elmer'
))
