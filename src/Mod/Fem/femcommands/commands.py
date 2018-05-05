# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************


import FreeCAD
import FreeCADGui
from .manager import CommandManager
from PySide import QtCore


class _CommandFemAnalysis(CommandManager):
    "The FEM_Analysis command definition"
    def __init__(self):
        super(_CommandFemAnalysis, self).__init__()
        self.resources = {'Pixmap': 'fem-analysis',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_Analysis", "Analysis container"),
                          'Accel': "N, A",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_Analysis", "Creates an analysis container with standard solver CalculiX")}
        self.is_active = 'with_document'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeAnalysis(FreeCAD.ActiveDocument, 'Analysis')")
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.ActiveObject)")
        # create a CalculiX ccx tools solver for any new analysis, to be on the save side fo rnew users
        FreeCADGui.doCommand("ObjectsFem.makeSolverCalculixCcxTools(FreeCAD.ActiveDocument)")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(FreeCAD.ActiveDocument.ActiveObject)")


class _CommandFemConstraintBodyHeatSource(CommandManager):
    "The FEM_ConstraintBodyHeatSource command definition"
    def __init__(self):
        super(_CommandFemConstraintBodyHeatSource, self).__init__()
        self.resources = {
            'Pixmap': 'fem-constraint-heatflux',  # the heatflux icon is used
            'MenuText': QtCore.QT_TRANSLATE_NOOP(
                "FEM_ConstraintBodyHeatSource",
                "Constraint body heat source"),
            'ToolTip': QtCore.QT_TRANSLATE_NOOP(
                "FEM_ConstraintBodyHeatSource",
                "Creates a FEM constraint body heat source")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemConstraintBodyHeatSource")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeConstraintBodyHeatSource(FreeCAD.ActiveDocument))")


class _CommandFemConstraintElectrostaticPotential(CommandManager):
    "The FEM_ConstraintElectrostaticPotential command definition"
    def __init__(self):
        super(_CommandFemConstraintElectrostaticPotential, self).__init__()
        self.resources = {
            'Pixmap': 'fem-constraint-electrostatic-potential',
            'MenuText': QtCore.QT_TRANSLATE_NOOP(
                "FEM_ConstraintElectrostaticPotential",
                "Constraint Potential"),
            'ToolTip': QtCore.QT_TRANSLATE_NOOP(
                "FEM_ConstraintElectrostaticPotential",
                "Creates a FEM constraint electrostatic potential")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemConstraintElectrostaticPotential")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeConstraintElectrostaticPotential(FreeCAD.ActiveDocument))")


class _CommandFemConstraintFlowVelocity(CommandManager):
    "The FEM_ConstraintFlowVelocity command definition"
    def __init__(self):
        super(_CommandFemConstraintFlowVelocity, self).__init__()
        self.resources = {
            'Pixmap': 'fem-constraint-flow-velocity',
            'MenuText': QtCore.QT_TRANSLATE_NOOP(
                "FEM_ConstraintFlowVelocity",
                "Constraint Flow Velocity"),
            'ToolTip': QtCore.QT_TRANSLATE_NOOP(
                "FEM_ConstraintFlowVelocity",
                "Creates a FEM constraint flow velocity")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemConstraintFlowVelocity")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeConstraintFlowVelocity(FreeCAD.ActiveDocument))")


class _CommandFemConstraintInitialFlowVelocity(CommandManager):
    "The FEM_ConstraintInitialFlowVelocity command definition"
    def __init__(self):
        super(_CommandFemConstraintInitialFlowVelocity, self).__init__()
        self.resources = {
            'Pixmap': 'fem-constraint-initial-flow-velocity',
            'MenuText': QtCore.QT_TRANSLATE_NOOP(
                "FEM_ConstraintInitialFlowVelocity",
                "Constraint Initial Flow Velocity"),
            'ToolTip': QtCore.QT_TRANSLATE_NOOP(
                "FEM_ConstraintInitialFlowVelocity",
                "Creates a FEM constraint initial flow velocity")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemConstraintInitialFlowVelocity")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeConstraintInitialFlowVelocity(FreeCAD.ActiveDocument))")


class _CommandFemConstraintSelfWeight(CommandManager):
    "The FEM_ConstraintSelfWeight command definition"
    def __init__(self):
        super(_CommandFemConstraintSelfWeight, self).__init__()
        self.resources = {'Pixmap': 'fem-constraint-selfweight',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_ConstraintSelfWeight", "Constraint self weight"),
                          'Accel': "C, W",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_ConstraintSelfWeight", "Creates a FEM constraint self weight")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemConstraintSelfWeight")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeConstraintSelfWeight(FreeCAD.ActiveDocument))")


class _CommandFemElementFluid1D(CommandManager):
    "The FEM_ElementFluid1D command definition"
    def __init__(self):
        super(_CommandFemElementFluid1D, self).__init__()
        self.resources = {'Pixmap': 'fem-fluid-section',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_ElementFluid1D", "Fluid section for 1D flow"),
                          'Accel': "C, B",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_ElementFluid1D", "Creates a FEM fluid section for 1D flow")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemElementFluid1D")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeElementFluid1D(FreeCAD.ActiveDocument))")


class _CommandFemElementGeometry1D(CommandManager):
    "The Fem_ElementGeometry1D command definition"
    def __init__(self):
        super(_CommandFemElementGeometry1D, self).__init__()
        self.resources = {'Pixmap': 'fem-beam-section',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_ElementGeometry1D", "Beam cross section"),
                          'Accel': "C, B",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_ElementGeometry1D", "Creates a FEM beam cross section")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemElementGeometry1D")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeElementGeometry1D(FreeCAD.ActiveDocument))")


class _CommandFemElementGeometry2D(CommandManager):
    "The FEM_ElementGeometry2D command definition"
    def __init__(self):
        super(_CommandFemElementGeometry2D, self).__init__()
        self.resources = {'Pixmap': 'fem-shell-thickness',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_ElementGeometry2D", "Shell plate thickness"),
                          'Accel': "C, S",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_ElementGeometry2D", "Creates a FEM shell plate thickness")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemElementGeometry2D")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeElementGeometry2D(FreeCAD.ActiveDocument))")


class _CommandFemElementRotation1D(CommandManager):
    "The Fem_ElementRotation1D command definition"
    def __init__(self):
        super(_CommandFemElementRotation1D, self).__init__()
        self.resources = {'Pixmap': 'fem-beam-rotation',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_ElementRotation1D", "Beam rotation"),
                          'Accel': "C, R",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_ElementRotation1D", "Creates a FEM beam rotation")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemElementRotation1D")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeElementRotation1D(FreeCAD.ActiveDocument))")


class _CommandFemEquationElectrostatic(CommandManager):
    "The FEM_EquationElectrostatic command definition"
    def __init__(self):
        super(_CommandFemEquationElectrostatic, self).__init__()
        self.resources = {'Pixmap': 'fem-equation-electrostatic',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_EquationElectrostatic", "Electrostatic equation"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_EquationElectrostatic", "Creates a FEM equation for electrostatic")}
        self.is_active = 'with_solver_elmer'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemEquationElasticity")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeEquationElectrostatic(FreeCAD.ActiveDocument, FreeCAD.ActiveDocument." + self.selobj.Name + ")")
        FreeCADGui.Selection.clearSelection()


class _CommandFemEquationElasticity(CommandManager):
    "The FEM_EquationElasticity command definition"
    def __init__(self):
        super(_CommandFemEquationElasticity, self).__init__()
        self.resources = {'Pixmap': 'fem-equation-elasticity',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_EquationElasticity", "Elasticity equation"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_EquationElasticity", "Creates a FEM equation for elasticity")}
        self.is_active = 'with_solver_elmer'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemEquationElasticity")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeEquationElasticity(FreeCAD.ActiveDocument, FreeCAD.ActiveDocument." + self.selobj.Name + ")")
        FreeCADGui.Selection.clearSelection()


class _CommandFemEquationFlow(CommandManager):
    "The FEM_EquationFlow command definition"
    def __init__(self):
        super(_CommandFemEquationFlow, self).__init__()
        self.resources = {'Pixmap': 'fem-equation-flow',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_EquationFlow", "Flow equation"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_EquationFlow", "Creates a FEM equation for flow")}
        self.is_active = 'with_solver_elmer'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemEquationFlow")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeEquationFlow(FreeCAD.ActiveDocument, FreeCAD.ActiveDocument." + self.selobj.Name + ")")
        FreeCADGui.Selection.clearSelection()


class _CommandFemEquationFluxsolver(CommandManager):
    "The FEM_EquationFluxsolver command definition"
    def __init__(self):
        super(_CommandFemEquationFluxsolver, self).__init__()
        self.resources = {'Pixmap': 'fem-equation-fluxsolver',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_EquationFluxsolver", "Fluxsolver equation"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_EquationFluxsolver", "Creates a FEM equation for fluxsolver")}
        self.is_active = 'with_solver_elmer'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemEquationFluxsolver")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeEquationFluxsolver(FreeCAD.ActiveDocument, FreeCAD.ActiveDocument." + self.selobj.Name + ")")
        FreeCADGui.Selection.clearSelection()


class _CommandFemEquationHeat(CommandManager):
    "The FEM_EquationHeat command definition"
    def __init__(self):
        super(_CommandFemEquationHeat, self).__init__()
        self.resources = {'Pixmap': 'fem-equation-heat',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_EquationHeat", "Fluxsolver heat"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_EquationHeat", "Creates a FEM equation for heat")}
        self.is_active = 'with_solver_elmer'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemEquationHeat")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeEquationHeat(FreeCAD.ActiveDocument, FreeCAD.ActiveDocument." + self.selobj.Name + ")")
        FreeCADGui.Selection.clearSelection()


class _CommandFemMaterialFluid(CommandManager):
    "The FEM_MaterialFluid command definition"
    def __init__(self):
        super(_CommandFemMaterialFluid, self).__init__()
        self.resources = {'Pixmap': 'fem-material-fluid',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MaterialFluid", "FEM material for fluid"),
                          'Accel': "M, M",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MaterialFluid", "Creates a FEM material for fluid")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Fluid Material")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeMaterialFluid(FreeCAD.ActiveDocument, 'FluidMaterial'))")
        FreeCADGui.doCommand("FreeCADGui.ActiveDocument.setEdit(FreeCAD.ActiveDocument.ActiveObject.Name)")


class _CommandFemMaterialMechanicalNonlinear(CommandManager):
    "The FEM_MaterialMechanicalNonlinear command definition"
    def __init__(self):
        super(_CommandFemMaterialMechanicalNonlinear, self).__init__()
        self.resources = {'Pixmap': 'fem-material-nonlinear',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MaterialMechanicalNonlinear", "Nonlinear mechanical material"),
                          'Accel': "C, W",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MaterialMechanicalNonlinear", "Creates a nonlinear mechanical material")}
        self.is_active = 'with_material_solid_which_has_no_nonlinear_material'

    def Activated(self):
        string_lin_mat_obj = "FreeCAD.ActiveDocument.getObject('" + self.selobj.Name + "')"
        command_to_run = "FemGui.getActiveAnalysis().addObject(ObjectsFem.makeMaterialMechanicalNonlinear(FreeCAD.ActiveDocument, " + string_lin_mat_obj + "))"
        FreeCAD.ActiveDocument.openTransaction("Create FemMaterialMechanicalNonlinear")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand(command_to_run)
        # set some property of the solver to nonlinear (only if one solver is available and if this solver is a CalculiX solver):
        # nonlinear material
        # nonlinear geometry --> its is triggered anyway https://forum.freecadweb.org/viewtopic.php?f=18&t=23101&p=180489#p180489
        solver_object = None
        for m in self.active_analysis.Group:
            if m.isDerivedFrom('Fem::FemSolverObjectPython'):
                if not solver_object:
                    solver_object = m
                else:
                    # we do not change attributes if we have more than one solver, since we do not know which one to take
                    solver_object = None
                    break
        # check new frame work solver and old frame work solver
        if solver_object and (hasattr(solver_object, "SolverType") and solver_object.SolverType == 'FemSolverCalculix' or (hasattr(solver_object, "Proxy") and solver_object.Proxy.Type == 'Fem::FemSolverObjectCalculix')):
            solver_object.MaterialNonlinearity = "nonlinear"
            solver_object.GeometricalNonlinearity = "nonlinear"
        FreeCADGui.doCommand("App.ActiveDocument.recompute()")
        FreeCADGui.Selection.clearSelection()


class _CommandFemMaterialSolid(CommandManager):
    "The FEM_MaterialSolid command definition"
    def __init__(self):
        super(_CommandFemMaterialSolid, self).__init__()
        self.resources = {'Pixmap': 'fem-material',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MaterialSolid", "FEM material for solid"),
                          'Accel': "M, M",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MaterialSolid", "Creates a FEM material for solid")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Solid Material")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeMaterialSolid(FreeCAD.ActiveDocument, 'SolidMaterial'))")
        FreeCADGui.doCommand("FreeCADGui.ActiveDocument.setEdit(FreeCAD.ActiveDocument.ActiveObject.Name)")


class _CommandFemMesh2Mesh(CommandManager):
    "The FEM_FemMesh2Mesh command definition"
    def __init__(self):
        super(_CommandFemMesh2Mesh, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-to-mesh',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_FEMMesh2Mesh", "FEM mesh to mesh"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_FEMMesh2Mesh", "Convert the surface of a FEM mesh to a mesh")}
        self.is_active = 'with_femmesh_andor_res'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Mesh from FEMMesh")
        if self.selobj and not self.selobj2:  # no result object selected
            FreeCADGui.addModule("femmesh.femmesh2mesh")
            FreeCADGui.doCommand("out_mesh = femmesh.femmesh2mesh.femmesh_2_mesh(App.ActiveDocument." + self.selobj.Name + ".FemMesh)")
            FreeCADGui.addModule("Mesh")
            FreeCADGui.doCommand("Mesh.show(Mesh.Mesh(out_mesh))")
            FreeCADGui.doCommand("App.ActiveDocument." + self.selobj.Name + ".ViewObject.hide()")
        if self.selobj and self.selobj2:
            femmesh = self.selobj
            res = self.selobj2
            FreeCADGui.addModule("femmesh.femmesh2mesh")
            FreeCADGui.doCommand("out_mesh = femmesh.femmesh2mesh.femmesh_2_mesh(App.ActiveDocument." + femmesh.Name + ".FemMesh, App.ActiveDocument." + res.Name + ")")
            FreeCADGui.addModule("Mesh")
            FreeCADGui.doCommand("Mesh.show(Mesh.Mesh(out_mesh))")
            FreeCADGui.doCommand("App.ActiveDocument." + femmesh.Name + ".ViewObject.hide()")
        FreeCADGui.Selection.clearSelection()


class _CommandFemMeshBoundaryLayer(CommandManager):
    "The FEM_MeshBoundaryLayer command definition"
    def __init__(self):
        super(_CommandFemMeshBoundaryLayer, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-boundary-layer',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshBoundaryLayer", "FEM mesh boundary layer"),
                          'Accel': "M, B",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshBoundaryLayer", "Creates a FEM mesh boundary layer")}
        self.is_active = 'with_gmsh_femmesh'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemMeshBoundaryLayer")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeMeshBoundaryLayer(FreeCAD.ActiveDocument, FreeCAD.ActiveDocument." + self.selobj.Name + ")")
        FreeCADGui.Selection.clearSelection()


class _CommandFemMeshClear(CommandManager):
    "The FEM_MeshClear command definition"
    def __init__(self):
        super(_CommandFemMeshClear, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-clear-mesh',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshClear", "Clear FEM mesh"),
                          # 'Accel': "Z, Z",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshClear", "Clear the Mesh of a FEM mesh object")}
        self.is_active = 'with_femmesh'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Clear FEM mesh")
        FreeCADGui.addModule("Fem")
        FreeCADGui.doCommand("App.ActiveDocument." + self.selobj.Name + ".FemMesh = Fem.FemMesh()")
        FreeCADGui.doCommand("App.ActiveDocument.recompute()")
        FreeCADGui.Selection.clearSelection()


class _CommandFemMeshGmshFromShape(CommandManager):
    "The FEM_MeshGmshFromShape command definition"
    def __init__(self):
        super(_CommandFemMeshGmshFromShape, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-gmsh-from-shape',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshGmshFromShape", "FEM mesh from shape by Gmsh"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshGmshFromShape", "Create a FEM mesh from a shape by Gmsh mesher")}
        self.is_active = 'with_part_feature'

    def Activated(self):
        # a mesh could be made with and without an analysis,
        # we're going to check not for an analysis in command manager module
        FreeCAD.ActiveDocument.openTransaction("Create FEM mesh by Gmsh")
        mesh_obj_name = 'FEMMeshGmsh'
        # mesh_obj_name = self.selobj.Name + "_Mesh"  # if requested by some people add Preference for this
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeMeshGmsh(FreeCAD.ActiveDocument, '" + mesh_obj_name + "')")
        FreeCADGui.doCommand("FreeCAD.ActiveDocument.ActiveObject.Part = FreeCAD.ActiveDocument." + self.selobj.Name)
        import FemGui
        if FemGui.getActiveAnalysis():
            FreeCADGui.addModule("FemGui")
            FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(FreeCAD.ActiveDocument.ActiveObject)")
        FreeCADGui.doCommand("Gui.ActiveDocument.setEdit(FreeCAD.ActiveDocument.ActiveObject.Name)")
        FreeCADGui.Selection.clearSelection()


class _CommandFemMeshGroup(CommandManager):
    "The FEM_MeshGroup command definition"
    def __init__(self):
        super(_CommandFemMeshGroup, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-from-shape',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshGroup", "FEM mesh group"),
                          'Accel': "M, G",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshGroup", "Creates a FEM mesh group")}
        self.is_active = 'with_gmsh_femmesh'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemMeshGroup")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeMeshGroup(FreeCAD.ActiveDocument, FreeCAD.ActiveDocument." + self.selobj.Name + ")")
        FreeCADGui.Selection.clearSelection()


class _CommandFemMeshNetgenFromShape(CommandManager):
    "The FEM_MeshNetgenFromShape command definition"
    def __init__(self):
        super(_CommandFemMeshNetgenFromShape, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-netgen-from-shape',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshFromShape", "FEM mesh from shape by Netgen"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshFromShape", "Create a FEM volume mesh from a solid or face shape by Netgen internal mesher")}
        self.is_active = 'with_part_feature'

    def Activated(self):
        # a mesh could be made with and without an analysis,
        # we're going to check not for an analysis in command manager module
        FreeCAD.ActiveDocument.openTransaction("Create FEM mesh Netgen")
        mesh_obj_name = 'FEMMeshNetgen'
        # mesh_obj_name = sel[0].Name + "_Mesh"  # if requested by some people add Preference for this
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeMeshNetgen(FreeCAD.ActiveDocument, '" + mesh_obj_name + "')")
        FreeCADGui.doCommand("FreeCAD.ActiveDocument.ActiveObject.Shape = FreeCAD.ActiveDocument." + self.selobj.Name)
        import FemGui
        if FemGui.getActiveAnalysis():
            FreeCADGui.addModule("FemGui")
            FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(App.ActiveDocument.ActiveObject)")
        FreeCADGui.doCommand("Gui.ActiveDocument.setEdit(App.ActiveDocument.ActiveObject.Name)")
        FreeCADGui.Selection.clearSelection()


class _CommandFemMeshPrintInfo(CommandManager):
    "The FEM_MeshPrintInfo command definition"
    def __init__(self):
        super(_CommandFemMeshPrintInfo, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-print-info',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshPrintInfo", "Print FEM mesh info"),
                          # 'Accel': "Z, Z",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshPrintInfo", "Print FEM mesh info")}
        self.is_active = 'with_femmesh'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Print FEM mesh info")
        FreeCADGui.doCommand("print(App.ActiveDocument." + self.selobj.Name + ".FemMesh)")
        FreeCADGui.addModule("PySide")
        FreeCADGui.doCommand("mesh_info = str(App.ActiveDocument." + self.selobj.Name + ".FemMesh)")
        FreeCADGui.doCommand("PySide.QtGui.QMessageBox.information(None, 'FEM Mesh Info', mesh_info)")
        FreeCADGui.Selection.clearSelection()


class _CommandFemMeshRegion(CommandManager):
    "The FEM_MeshRegion command definition"
    def __init__(self):
        super(_CommandFemMeshRegion, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-region',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshRegion", "FEM mesh region"),
                          'Accel': "M, R",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshRegion", "Creates a FEM mesh region")}
        self.is_active = 'with_gmsh_femmesh'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemMeshRegion")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeMeshRegion(FreeCAD.ActiveDocument, FreeCAD.ActiveDocument." + self.selobj.Name + ")")
        FreeCADGui.Selection.clearSelection()


class _CommandFemResultShow(CommandManager):
    "The FEM_ResultShow command definition"
    def __init__(self):
        super(_CommandFemResultShow, self).__init__()
        self.resources = {'Pixmap': 'fem-result',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_ResultShow", "Show result"),
                          'Accel': "S, R",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_ResultShow", "Shows and visualizes selected result data")}
        self.is_active = 'with_selresult'

    def Activated(self):
        self.selobj.ViewObject.startEditing()


class _CommandFemResultsPurge(CommandManager):
    "The FEM_ResultsPurge command definition"
    def __init__(self):
        super(_CommandFemResultsPurge, self).__init__()
        self.resources = {'Pixmap': 'fem-purge-results',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_ResultsPurge", "Purge results"),
                          'Accel': "S, S",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_ResultsPurge", "Purges all results from active analysis")}
        self.is_active = 'with_results'

    def Activated(self):
        import femresult.resulttools as resulttools
        resulttools.purge_results(self.active_analysis)


class _CommandFemSolverCalculixCxxtools(CommandManager):
    "The FEM_SolverCalculix ccx tools command definition"
    def __init__(self):
        super(_CommandFemSolverCalculixCxxtools, self).__init__()
        self.resources = {'Pixmap': 'fem-solver',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_SolverCalculix", "Solver CalculiX Standard"),
                          'Accel': "S, X",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_SolverCalculix", "Creates a standard FEM solver CalculiX with ccx tools")}
        self.is_active = 'with_analysis'

    def Activated(self):
        has_nonlinear_material_obj = False
        for m in self.active_analysis.Group:
            if hasattr(m, "Proxy") and m.Proxy.Type == "Fem::MaterialMechanicalNonlinear":
                has_nonlinear_material_obj = True
        FreeCAD.ActiveDocument.openTransaction("Create SolverCalculix")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.addModule("FemGui")
        if has_nonlinear_material_obj:
            FreeCADGui.doCommand("solver = ObjectsFem.makeSolverCalculixCcxTools(FreeCAD.ActiveDocument)")
            FreeCADGui.doCommand("solver.GeometricalNonlinearity = 'nonlinear'")
            FreeCADGui.doCommand("solver.MaterialNonlinearity = 'nonlinear'")
            FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(solver)")
        else:
            FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeSolverCalculixCcxTools(FreeCAD.ActiveDocument))")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _CommandFemSolverCalculiX(CommandManager):
    "The FEM_SolverCalculix command definition"
    def __init__(self):
        super(_CommandFemSolverCalculiX, self).__init__()
        self.resources = {'Pixmap': 'fem-solver',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_SolverCalculiX", "Solver CalculiX (experimental)"),
                          'Accel': "S, C",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_SolverCalculiX", "Creates a FEM solver CalculiX (experimental)")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create CalculiX solver object")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeSolverCalculix(FreeCAD.ActiveDocument))")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _CommandFemSolverControl(CommandManager):
    "The FEM_SolverControl command definition"
    def __init__(self):
        super(_CommandFemSolverControl, self).__init__()
        self.resources = {'Pixmap': 'fem-control-solver',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_SolverControl", "Solver job control"),
                          'Accel': "S, C",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_SolverControl", "Changes solver attributes and runs the calculations for the selected solver")}
        self.is_active = 'with_solver'

    def Activated(self):
        FreeCADGui.ActiveDocument.setEdit(self.selobj, 0)


class _CommandFemSolverElmer(CommandManager):
    "The FEM_SolverElmer command definition"
    def __init__(self):
        super(_CommandFemSolverElmer, self).__init__()
        self.resources = {'Pixmap': 'fem-elmer',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_SolverElmer", "Solver Elmer"),
                          'Accel': "S, E",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_SolverElmer", "Creates a FEM solver Elmer")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Elmer solver object")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeSolverElmer(FreeCAD.ActiveDocument))")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _CommandFemSolverRun(CommandManager):
    "The FEM_SolverRun command definition"
    def __init__(self):
        super(_CommandFemSolverRun, self).__init__()
        self.resources = {'Pixmap': 'fem-run-solver',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_SolverRun", "Run solver calculations"),
                          'Accel': "R, C",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_SolverRun", "Runs the calculations for the selected solver")}
        self.is_active = 'with_solver'

    def Activated(self):
        import femsolver.run
        from PySide import QtGui

        def load_results(ret_code):
            if ret_code == 0:
                self.fea.load_results()
            else:
                print("CalculiX failed ccx finished with error {}".format(ret_code))

        self.solver = self.selobj
        if self.solver.Proxy.Type == 'Fem::FemSolverCalculixCcxTools':
            print('CalxuliX ccx tools solver!')
            from femtools import ccxtools
            self.fea = ccxtools.FemToolsCcx(None, self.solver)
            self.fea.reset_mesh_purge_results_checked()
            message = self.fea.check_prerequisites()
            if message:
                QtGui.QMessageBox.critical(None, "Missing prerequisite", message)
                return
            self.fea.finished.connect(load_results)
            QtCore.QThreadPool.globalInstance().start(self.fea)
        else:
            print('Frame work solver!')
            try:
                machine = femsolver.run.getMachine(self.solver)
            except femsolver.run.MustSaveError:
                QtGui.QMessageBox.critical(
                    FreeCADGui.getMainWindow(),
                    "Can't start Solver",
                    "Please save the file before executing the solver. "
                    "This must be done because the location of the working "
                    "directory is set to \"Beside .fcstd File\".")
                return
            except femsolver.run.DirectoryDoesNotExist:
                QtGui.QMessageBox.critical(
                    FreeCADGui.getMainWindow(),
                    "Can't start Solver",
                    "Selected working directory doesn't exist.")
                return
            if not machine.running:
                machine.reset()
                machine.target = femsolver.run.RESULTS
                machine.start()
                machine.join()  # wait for the machine to finish.
        FreeCADGui.Selection.clearSelection()


class _CommandFemSolverZ88(CommandManager):
    "The FEM_SolverZ88 command definition"
    def __init__(self):
        super(_CommandFemSolverZ88, self).__init__()
        self.resources = {'Pixmap': 'fem-solver',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_SolverZ88", "Solver Z88"),
                          'Accel': "S, Z",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_SolverZ88", "Creates a FEM solver Z88")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create SolverZ88")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(ObjectsFem.makeSolverZ88(FreeCAD.ActiveDocument))")


# the sting in add command will be the page name on FreeCAD wiki
FreeCADGui.addCommand('FEM_Analysis', _CommandFemAnalysis())
FreeCADGui.addCommand('FEM_ConstraintBodyHeatSource', _CommandFemConstraintBodyHeatSource())
FreeCADGui.addCommand('FEM_ConstraintElectrostaticPotential', _CommandFemConstraintElectrostaticPotential())
FreeCADGui.addCommand('FEM_ConstraintFlowVelocity', _CommandFemConstraintFlowVelocity())
FreeCADGui.addCommand('FEM_ConstraintInitialFlowVelocity', _CommandFemConstraintInitialFlowVelocity())
FreeCADGui.addCommand('FEM_ConstraintSelfWeight', _CommandFemConstraintSelfWeight())
FreeCADGui.addCommand('FEM_ElementFluid1D', _CommandFemElementFluid1D())
FreeCADGui.addCommand('FEM_ElementGeometry1D', _CommandFemElementGeometry1D())
FreeCADGui.addCommand('FEM_ElementGeometry2D', _CommandFemElementGeometry2D())
FreeCADGui.addCommand('FEM_ElementRotation1D', _CommandFemElementRotation1D())
FreeCADGui.addCommand('FEM_EquationElectrostatic', _CommandFemEquationElectrostatic())
FreeCADGui.addCommand('FEM_EquationElasticity', _CommandFemEquationElasticity())
FreeCADGui.addCommand('FEM_EquationFlow', _CommandFemEquationFlow())
FreeCADGui.addCommand('FEM_EquationFluxsolver', _CommandFemEquationFluxsolver())
FreeCADGui.addCommand('FEM_EquationHeat', _CommandFemEquationHeat())
FreeCADGui.addCommand('FEM_MaterialFluid', _CommandFemMaterialFluid())
FreeCADGui.addCommand('FEM_MaterialMechanicalNonlinear', _CommandFemMaterialMechanicalNonlinear())
FreeCADGui.addCommand('FEM_MaterialSolid', _CommandFemMaterialSolid())
FreeCADGui.addCommand('FEM_FEMMesh2Mesh', _CommandFemMesh2Mesh())
FreeCADGui.addCommand('FEM_MeshBoundaryLayer', _CommandFemMeshBoundaryLayer())
FreeCADGui.addCommand('FEM_MeshClear', _CommandFemMeshClear())
FreeCADGui.addCommand('FEM_MeshGmshFromShape', _CommandFemMeshGmshFromShape())
FreeCADGui.addCommand('FEM_MeshGroup', _CommandFemMeshGroup())
FreeCADGui.addCommand('FEM_MeshNetgenFromShape', _CommandFemMeshNetgenFromShape())
FreeCADGui.addCommand('FEM_MeshPrintInfo', _CommandFemMeshPrintInfo())
FreeCADGui.addCommand('FEM_MeshRegion', _CommandFemMeshRegion())
FreeCADGui.addCommand('FEM_ResultShow', _CommandFemResultShow())
FreeCADGui.addCommand('FEM_ResultsPurge', _CommandFemResultsPurge())
FreeCADGui.addCommand('FEM_SolverCalculixCxxtools', _CommandFemSolverCalculixCxxtools())
FreeCADGui.addCommand('FEM_SolverCalculiX', _CommandFemSolverCalculiX())
FreeCADGui.addCommand('FEM_SolverControl', _CommandFemSolverControl())
FreeCADGui.addCommand('FEM_SolverElmer', _CommandFemSolverElmer())
FreeCADGui.addCommand('FEM_SolverRun', _CommandFemSolverRun())
FreeCADGui.addCommand('FEM_SolverZ88', _CommandFemSolverZ88())
