# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__  = "FreeCAD FEM command definitions"
__author__ = "Bernd Hahnebach"
__url__    = "https://www.freecadweb.org"

## @package commands
#  \ingroup FEM
#  \brief FreeCAD FEM command definitions

import FreeCAD
import FreeCADGui

from .manager import CommandManager
from femtools.femutils import is_of_type


# Python command definitions
# for C++ command definitions see src/Mod/Fem/Command.cpp
# TODO, may be even more generic class creation
# with type() and identifier instead of class for
# the commands which add new document objects.
# see https://www.python-course.eu/python3_classes_and_type.php


class _Analysis(CommandManager):
    "The FEM_Analysis command definition"

    def __init__(self):
        super(_Analysis, self).__init__()
        self.menuetext = "Analysis container"
        self.accel = "S, A"
        self.tooltip = "Creates an analysis container with standard solver CalculiX"
        self.is_active = "with_document"

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeAnalysis(FreeCAD.ActiveDocument, 'Analysis')")
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.ActiveObject)")
        # create a CalculiX ccx tools solver for any new analysis
        # to be on the safe side for new users
        FreeCADGui.doCommand("ObjectsFem.makeSolverCalculixCcxTools(FreeCAD.ActiveDocument)")
        FreeCADGui.doCommand(
            "FemGui.getActiveAnalysis().addObject(FreeCAD.ActiveDocument.ActiveObject)"
        )
        FreeCAD.ActiveDocument.recompute()


class _ClippingPlaneAdd(CommandManager):
    "The FEM_ClippingPlaneAdd command definition"

    def __init__(self):
        super(_ClippingPlaneAdd, self).__init__()
        self.menuetext = "Clipping plane on face"
        self.tooltip = "Add a clipping plane on a selected face"
        self.is_active = "with_document"

    def Activated(self):
        from pivy import coin
        from femtools.femutils import getBoundBoxOfAllDocumentShapes
        from femtools.femutils import getSelectedFace

        overalboundbox = getBoundBoxOfAllDocumentShapes(FreeCAD.ActiveDocument)
        # print(overalboundbox)
        min_bb_length = (min(set([
            overalboundbox.XLength,
            overalboundbox.YLength,
            overalboundbox.ZLength
        ])))
        dbox = min_bb_length * 0.2

        aFace = getSelectedFace(FreeCADGui.Selection.getSelectionEx())
        if aFace:
            f_CoM = aFace.CenterOfMass
            f_uvCoM = aFace.Surface.parameter(f_CoM)  # u,v at CoM for normalAt calculation
            f_normal = aFace.normalAt(f_uvCoM[0], f_uvCoM[1])
        else:
            f_CoM = FreeCAD.Vector(0, 0, 0)
            f_normal = FreeCAD.Vector(0, 0, 1)

        coin_normal_vector = coin.SbVec3f(-f_normal.x, -f_normal.y, -f_normal.z)
        coin_bound_box = coin.SbBox3f(
            f_CoM.x - dbox, f_CoM.y - dbox,
            f_CoM.z - dbox * 0.15,
            f_CoM.x + dbox,
            f_CoM.y + dbox,
            f_CoM.z + dbox * 0.15
        )
        clip_plane = coin.SoClipPlaneManip()
        clip_plane.setValue(coin_bound_box, coin_normal_vector, 1)
        FreeCADGui.ActiveDocument.ActiveView.getSceneGraph().insertChild(clip_plane, 1)


class _ClippingPlaneRemoveAll(CommandManager):
    "The FEM_ClippingPlaneemoveAll command definition"

    def __init__(self):
        super(_ClippingPlaneRemoveAll, self).__init__()
        self.menuetext = "Remove all clipping planes"
        self.tooltip = "Remove all clipping planes"
        self.is_active = "with_document"

    def Activated(self):
        line1 = "for node in list(sg.getChildren()):\n"
        line2 = "    if isinstance(node, coin.SoClipPlane):\n"
        line3 = "        sg.removeChild(node)"
        FreeCADGui.doCommand("from pivy import coin")
        FreeCADGui.doCommand("sg = Gui.ActiveDocument.ActiveView.getSceneGraph()")
        FreeCADGui.doCommand("nodes = sg.getChildren()")
        FreeCADGui.doCommand(line1 + line2 + line3)


class _ConstantVacuumPermittivity(CommandManager):
    "The FEM_ConstantVacuumPermittivity command definition"

    def __init__(self):
        super(_ConstantVacuumPermittivity, self).__init__()
        self.pixmap = "fem-solver-analysis-thermomechanical.svg"
        self.menuetext = "Constant vacuum permittivity"
        self.tooltip = "Creates a FEM constant vacuum permittivity to overwrite standard value"
        self.is_active = "with_document"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_noset_edit"


class _ConstraintBodyHeatSource(CommandManager):
    "The FEM_ConstraintBodyHeatSource command definition"

    def __init__(self):
        super(_ConstraintBodyHeatSource, self).__init__()
        self.pixmap = "FEM_ConstraintHeatflux"  # the heatflux icon is used
        self.menuetext = "Constraint body heat source"
        self.tooltip = "Creates a FEM constraint body heat source"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_noset_edit"


class _ConstraintElectrostaticPotential(CommandManager):
    "The FEM_ConstraintElectrostaticPotential command definition"

    def __init__(self):
        super(_ConstraintElectrostaticPotential, self).__init__()
        self.menuetext = "Constraint electrostatic potential"
        self.tooltip = "Creates a FEM constraint electrostatic potential"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _ConstraintFlowVelocity(CommandManager):
    "The FEM_ConstraintFlowVelocity command definition"

    def __init__(self):
        super(_ConstraintFlowVelocity, self).__init__()
        self.menuetext = "Constraint flow velocity"
        self.tooltip = "Creates a FEM constraint flow velocity"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _ConstraintInitialFlowVelocity(CommandManager):
    "The FEM_ConstraintInitialFlowVelocity command definition"

    def __init__(self):
        super(_ConstraintInitialFlowVelocity, self).__init__()
        self.menuetext = "Constraint initial flow velocity"
        self.tooltip = "Creates a FEM constraint initial flow velocity"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _ConstraintSectionPrint(CommandManager):
    "The FEM_ConstraintSectionPrint command definition"

    def __init__(self):
        super(_ConstraintSectionPrint, self).__init__()
        self.menuetext = "Constraint sectionprint"
        self.tooltip = "Creates a FEM constraint sectionprint"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _ConstraintSelfWeight(CommandManager):
    "The FEM_ConstraintSelfWeight command definition"

    def __init__(self):
        super(_ConstraintSelfWeight, self).__init__()
        self.menuetext = "Constraint self weight"
        self.tooltip = "Creates a FEM constraint self weight"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_noset_edit"


class _ConstraintTie(CommandManager):
    "The FEM_ConstraintTie command definition"

    def __init__(self):
        super(_ConstraintTie, self).__init__()
        self.menuetext = "Constraint tie"
        self.tooltip = "Creates a FEM constraint tie"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _ElementFluid1D(CommandManager):
    "The FEM_ElementFluid1D command definition"

    def __init__(self):
        super(_ElementFluid1D, self).__init__()
        self.menuetext = "Fluid section for 1D flow"
        self.tooltip = "Creates a FEM fluid section for 1D flow"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _ElementGeometry1D(CommandManager):
    "The Fem_ElementGeometry1D command definition"

    def __init__(self):
        super(_ElementGeometry1D, self).__init__()
        self.menuetext = "Beam cross section"
        self.tooltip = "Creates a FEM beam cross section"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _ElementGeometry2D(CommandManager):
    "The FEM_ElementGeometry2D command definition"

    def __init__(self):
        super(_ElementGeometry2D, self).__init__()
        self.menuetext = "Shell plate thickness"
        self.tooltip = "Creates a FEM shell plate thickness"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _ElementRotation1D(CommandManager):
    "The Fem_ElementRotation1D command definition"

    def __init__(self):
        super(_ElementRotation1D, self).__init__()
        self.menuetext = "Beam rotation"
        self.tooltip = "Creates a FEM beam rotation"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_noset_edit"


class _EquationElectrostatic(CommandManager):
    "The FEM_EquationElectrostatic command definition"

    def __init__(self):
        super(_EquationElectrostatic, self).__init__()
        self.menuetext = "Electrostatic equation"
        self.tooltip = "Creates a FEM equation for electrostatic"
        self.is_active = "with_solver_elmer"
        self.do_activated = "add_obj_on_gui_selobj_noset_edit"


class _EquationElasticity(CommandManager):
    "The FEM_EquationElasticity command definition"

    def __init__(self):
        super(_EquationElasticity, self).__init__()
        self.menuetext = "Elasticity equation"
        self.tooltip = "Creates a FEM equation for elasticity"
        self.is_active = "with_solver_elmer"
        self.do_activated = "add_obj_on_gui_selobj_noset_edit"


class _EquationFlow(CommandManager):
    "The FEM_EquationFlow command definition"

    def __init__(self):
        super(_EquationFlow, self).__init__()
        self.menuetext = "Flow equation"
        self.tooltip = "Creates a FEM equation for flow"
        self.is_active = "with_solver_elmer"
        self.do_activated = "add_obj_on_gui_selobj_noset_edit"


class _EquationFlux(CommandManager):
    "The FEM_EquationFlux command definition"

    def __init__(self):
        super(_EquationFlux, self).__init__()
        self.menuetext = "Flux equation"
        self.tooltip = "Creates a FEM equation for flux"
        self.is_active = "with_solver_elmer"
        self.do_activated = "add_obj_on_gui_selobj_noset_edit"


class _EquationElectricforce(CommandManager):
    "The FEM_EquationElectricforce command definition"

    def __init__(self):
        super(_EquationElectricforce, self).__init__()
        self.menuetext = "Electricforce equation"
        self.tooltip = "Creates a FEM equation for electric forces"
        self.is_active = "with_solver_elmer"
        self.do_activated = "add_obj_on_gui_selobj_noset_edit"


class _EquationHeat(CommandManager):
    "The FEM_EquationHeat command definition"

    def __init__(self):
        super(_EquationHeat, self).__init__()
        self.menuetext = "Heat equation"
        self.tooltip = "Creates a FEM equation for heat"
        self.is_active = "with_solver_elmer"
        self.do_activated = "add_obj_on_gui_selobj_noset_edit"


class _Examples(CommandManager):
    "The FEM_Examples command definition"

    def __init__(self):
        super(_Examples, self).__init__()
        self.pixmap = "preferences-fem"
        self.menuetext = "Open FEM examples"
        self.tooltip = "Open FEM examples"
        self.is_active = "always"

    def Activated(self):
        FreeCADGui.addModule("femexamples.examplesgui")
        FreeCADGui.doCommand("femexamples.examplesgui.show_examplegui()")


class _MaterialEditor(CommandManager):
    "The FEM_MaterialEditor command definition"

    def __init__(self):
        super(_MaterialEditor, self).__init__()
        self.pixmap = "Arch_Material_Group"
        self.menuetext = "Material editor"
        self.tooltip = "Opens the FreeCAD material editor"
        self.is_active = "always"

    def Activated(self):
        FreeCADGui.addModule("MaterialEditor")
        FreeCADGui.doCommand("MaterialEditor.openEditor()")


class _MaterialFluid(CommandManager):
    "The FEM_MaterialFluid command definition"

    def __init__(self):
        super(_MaterialFluid, self).__init__()
        self.menuetext = "Material for fluid"
        self.tooltip = "Creates a FEM material for fluid"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _MaterialMechanicalNonlinear(CommandManager):
    "The FEM_MaterialMechanicalNonlinear command definition"

    def __init__(self):
        super(_MaterialMechanicalNonlinear, self).__init__()
        self.menuetext = "Nonlinear mechanical material"
        self.tooltip = "Creates a nonlinear mechanical material"
        self.is_active = "with_material_solid"

    def Activated(self):
        # test if there is a nonlinear material which has the selected material as base material
        for o in self.selobj.Document.Objects:
            if (
                is_of_type(o, "Fem::MaterialMechanicalNonlinear")
                and o.LinearBaseMaterial == self.selobj
            ):
                FreeCAD.Console.PrintError(
                    "Nonlinear material {} is based on the selected material {}. "
                    "Only one nonlinear object allowed for each material.\n"
                    .format(o.Name, self.selobj.Name)
                )
                return

        # add a nonlinear material
        string_lin_mat_obj = "FreeCAD.ActiveDocument.getObject('" + self.selobj.Name + "')"
        command_to_run = (
            "FemGui.getActiveAnalysis().addObject(ObjectsFem."
            "makeMaterialMechanicalNonlinear(FreeCAD.ActiveDocument, {}))"
            .format(string_lin_mat_obj)
        )
        FreeCAD.ActiveDocument.openTransaction("Create FemMaterialMechanicalNonlinear")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand(command_to_run)
        # set some property of the solver to nonlinear
        # (only if one solver is available and if this solver is a CalculiX solver):
        # nonlinear material
        # nonlinear geometry --> it is triggered anyway
        # https://forum.freecadweb.org/viewtopic.php?f=18&t=23101&p=180489#p180489
        solver_object = None
        for m in self.active_analysis.Group:
            if m.isDerivedFrom("Fem::FemSolverObjectPython"):
                if not solver_object:
                    solver_object = m
                else:
                    # we do not change attributes if we have more than one solver
                    # since we do not know which one to take
                    solver_object = None
                    break
        # set solver attribute for nonlinearity for ccxtools
        # CalculiX solver or new frame work CalculiX solver
        if solver_object and (
            is_of_type(solver_object, "Fem::SolverCcxTools")
            or is_of_type(solver_object, "Fem::SolverCalculix")
        ):
            FreeCAD.Console.PrintMessage(
                "Set MaterialNonlinearity and GeometricalNonlinearity to nonlinear for {}\n"
                .format(solver_object.Label)
            )
            solver_object.MaterialNonlinearity = "nonlinear"
            solver_object.GeometricalNonlinearity = "nonlinear"
        FreeCADGui.Selection.clearSelection()
        FreeCAD.ActiveDocument.recompute()


class _MaterialReinforced(CommandManager):
    "The FEM_MaterialReinforced command definition"

    def __init__(self):
        super(_MaterialReinforced, self).__init__()
        self.menuetext = "Reinforced material (concrete)"
        self.tooltip = "Creates a material for reinforced matrix material such as concrete"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _MaterialSolid(CommandManager):
    "The FEM_MaterialSolid command definition"

    def __init__(self):
        super(_MaterialSolid, self).__init__()
        self.menuetext = "Material for solid"
        self.accel = "M, S"
        self.tooltip = "Creates a FEM material for solid"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_set_edit"


class _FEMMesh2Mesh(CommandManager):
    "The FEM_FEMMesh2Mesh command definition"

    def __init__(self):
        super(_FEMMesh2Mesh, self).__init__()
        self.menuetext = "FEM mesh to mesh"
        self.tooltip = "Convert the surface of a FEM mesh to a mesh"
        self.is_active = "with_femmesh_andor_res"

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Mesh from FEMMesh")
        if self.selobj and not self.selobj2:  # no result object selected
            FreeCADGui.addModule("femmesh.femmesh2mesh")
            FreeCADGui.doCommand(
                "out_mesh = femmesh.femmesh2mesh.femmesh_2_mesh("
                "FreeCAD.ActiveDocument.{}.FemMesh)"
                .format(self.selobj.Name)
            )
            FreeCADGui.addModule("Mesh")
            FreeCADGui.doCommand("Mesh.show(Mesh.Mesh(out_mesh))")
            FreeCADGui.doCommand(
                "FreeCAD.ActiveDocument." + self.selobj.Name + ".ViewObject.hide()"
            )
        if self.selobj and self.selobj2:
            femmesh = self.selobj
            res = self.selobj2
            FreeCADGui.addModule("femmesh.femmesh2mesh")
            FreeCADGui.doCommand(
                "out_mesh = femmesh.femmesh2mesh.femmesh_2_mesh("
                "FreeCAD.ActiveDocument.{}.FemMesh, FreeCAD.ActiveDocument.{})"
                .format(femmesh.Name, res.Name)
            )
            FreeCADGui.addModule("Mesh")
            FreeCADGui.doCommand("Mesh.show(Mesh.Mesh(out_mesh))")
            FreeCADGui.doCommand(
                "FreeCAD.ActiveDocument." + femmesh.Name + ".ViewObject.hide()"
            )
        FreeCADGui.Selection.clearSelection()
        FreeCAD.ActiveDocument.recompute()


class _MeshBoundaryLayer(CommandManager):
    "The FEM_MeshBoundaryLayer command definition"

    def __init__(self):
        super(_MeshBoundaryLayer, self).__init__()
        self.menuetext = "FEM mesh boundary layer"
        self.tooltip = "Creates a FEM mesh boundary layer"
        self.is_active = "with_gmsh_femmesh"
        self.do_activated = "add_obj_on_gui_selobj_set_edit"


class _MeshClear(CommandManager):
    "The FEM_MeshClear command definition"

    def __init__(self):
        super(_MeshClear, self).__init__()
        self.menuetext = "Clear FEM mesh"
        self.tooltip = "Clear the Mesh of a FEM mesh object"
        self.is_active = "with_femmesh"

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Clear FEM mesh")
        FreeCADGui.addModule("Fem")
        FreeCADGui.doCommand(
            "FreeCAD.ActiveDocument." + self.selobj.Name + ".FemMesh = Fem.FemMesh()"
        )
        FreeCADGui.Selection.clearSelection()
        FreeCAD.ActiveDocument.recompute()


class _MeshDisplayInfo(CommandManager):
    "The FEM_MeshDisplayInfo command definition"

    def __init__(self):
        super(_MeshDisplayInfo, self).__init__()
        self.menuetext = "Display FEM mesh info"
        self.tooltip = "Display FEM mesh info"
        self.is_active = "with_femmesh"

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Display FEM mesh info")
        FreeCADGui.doCommand("print(FreeCAD.ActiveDocument." + self.selobj.Name + ".FemMesh)")
        FreeCADGui.addModule("PySide")
        FreeCADGui.doCommand(
            "mesh_info = str(FreeCAD.ActiveDocument." + self.selobj.Name + ".FemMesh)"
        )
        FreeCADGui.doCommand(
            "PySide.QtGui.QMessageBox.information(None, 'FEM Mesh Info', mesh_info)"
        )
        FreeCADGui.Selection.clearSelection()
        FreeCAD.ActiveDocument.recompute()


class _MeshGmshFromShape(CommandManager):
    "The FEM_MeshGmshFromShape command definition"

    def __init__(self):
        super(_MeshGmshFromShape, self).__init__()
        self.menuetext = "FEM mesh from shape by Gmsh"
        self.tooltip = "Create a FEM mesh from a shape by Gmsh mesher"
        self.is_active = "with_part_feature"

    def Activated(self):
        # a mesh could be made with and without an analysis,
        # we're going to check not for an analysis in command manager module
        FreeCAD.ActiveDocument.openTransaction("Create FEM mesh by Gmsh")
        mesh_obj_name = "FEMMeshGmsh"
        # if requested by some people add Preference for this
        # mesh_obj_name = self.selobj.Name + "_Mesh"
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand(
            "ObjectsFem.makeMeshGmsh(FreeCAD.ActiveDocument, '" + mesh_obj_name + "')"
        )
        FreeCADGui.doCommand(
            "FreeCAD.ActiveDocument.ActiveObject.Part = FreeCAD.ActiveDocument.{}"
            .format(self.selobj.Name)
        )
        # Gmsh mesh object could be added without an active analysis
        # but if there is an active analysis move it in there
        import FemGui
        if FemGui.getActiveAnalysis():
            FreeCADGui.addModule("FemGui")
            FreeCADGui.doCommand(
                "FemGui.getActiveAnalysis().addObject(FreeCAD.ActiveDocument.ActiveObject)"
            )
        FreeCADGui.doCommand(
            "FreeCADGui.ActiveDocument.setEdit(FreeCAD.ActiveDocument.ActiveObject.Name)"
        )
        FreeCADGui.Selection.clearSelection()
        FreeCAD.ActiveDocument.recompute()


class _MeshGroup(CommandManager):
    "The FEM_MeshGroup command definition"

    def __init__(self):
        super(_MeshGroup, self).__init__()
        self.menuetext = "FEM mesh group"
        self.tooltip = "Creates a FEM mesh group"
        self.is_active = "with_gmsh_femmesh"
        self.do_activated = "add_obj_on_gui_selobj_set_edit"


class _MeshNetgenFromShape(CommandManager):
    "The FEM_MeshNetgenFromShape command definition"

    def __init__(self):
        super(_MeshNetgenFromShape, self).__init__()
        self.menuetext = "FEM mesh from shape by Netgen"
        self.tooltip = "Create a FEM mesh from a solid or face shape by Netgen internal mesher"
        self.is_active = "with_part_feature"

    def Activated(self):
        # a mesh could be made with and without an analysis,
        # we're going to check not for an analysis in command manager module
        FreeCAD.ActiveDocument.openTransaction("Create FEM mesh Netgen")
        mesh_obj_name = "FEMMeshNetgen"
        # if requested by some people add Preference for this
        # mesh_obj_name = sel[0].Name + "_Mesh"
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand(
            "ObjectsFem.makeMeshNetgen(FreeCAD.ActiveDocument, '" + mesh_obj_name + "')"
        )
        FreeCADGui.doCommand(
            "FreeCAD.ActiveDocument.ActiveObject.Shape = FreeCAD.ActiveDocument.{}"
            .format(self.selobj.Name)
        )
        # Netgen mesh object could be added without an active analysis
        # but if there is an active analysis move it in there
        import FemGui
        if FemGui.getActiveAnalysis():
            FreeCADGui.addModule("FemGui")
            FreeCADGui.doCommand(
                "FemGui.getActiveAnalysis().addObject(FreeCAD.ActiveDocument.ActiveObject)"
            )
        FreeCADGui.doCommand(
            "FreeCADGui.ActiveDocument.setEdit(FreeCAD.ActiveDocument.ActiveObject.Name)"
        )
        FreeCADGui.Selection.clearSelection()
        # a recompute immediately starts meshing when task panel is opened, this is not intended


class _MeshRegion(CommandManager):
    "The FEM_MeshRegion command definition"

    def __init__(self):
        super(_MeshRegion, self).__init__()
        self.menuetext = "FEM mesh region"
        self.tooltip = "Creates a FEM mesh region"
        self.is_active = "with_gmsh_femmesh"
        self.do_activated = "add_obj_on_gui_selobj_set_edit"


class _ResultShow(CommandManager):
    "The FEM_ResultShow command definition"

    def __init__(self):
        super(_ResultShow, self).__init__()
        self.menuetext = "Show result"
        self.accel = "R, S"
        self.tooltip = "Shows and visualizes selected result data"
        self.is_active = "with_selresult"

    def Activated(self):
        self.selobj.ViewObject.Document.setEdit(self.selobj.ViewObject, 0)


class _ResultsPurge(CommandManager):
    "The FEM_ResultsPurge command definition"

    def __init__(self):
        super(_ResultsPurge, self).__init__()
        self.menuetext = "Purge results"
        self.accel = "R, P"
        self.tooltip = "Purges all results from active analysis"
        self.is_active = "with_results"

    def Activated(self):
        import femresult.resulttools as resulttools
        resulttools.purge_results(self.active_analysis)


class _SolverCxxtools(CommandManager):
    "The FEM_SolverCalculix ccx tools command definition"

    def __init__(self):
        super(_SolverCxxtools, self).__init__()
        self.pixmap = "FEM_SolverStandard"
        self.menuetext = "Solver CalculiX Standard"
        self.accel = "S, X"
        self.tooltip = "Creates a standard FEM solver CalculiX with ccx tools"
        self.is_active = "with_analysis"

    def Activated(self):
        has_nonlinear_material_obj = False
        for m in self.active_analysis.Group:
            if is_of_type(m, "Fem::MaterialMechanicalNonlinear"):
                has_nonlinear_material_obj = True
        FreeCAD.ActiveDocument.openTransaction("Create SolverCalculix")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.addModule("FemGui")
        if has_nonlinear_material_obj:
            FreeCADGui.doCommand(
                "solver = ObjectsFem.makeSolverCalculixCcxTools(FreeCAD.ActiveDocument)"
            )
            FreeCADGui.doCommand("solver.GeometricalNonlinearity = 'nonlinear'")
            FreeCADGui.doCommand("solver.MaterialNonlinearity = 'nonlinear'")
            FreeCADGui.doCommand("FemGui.getActiveAnalysis().addObject(solver)")
        else:
            FreeCADGui.doCommand(
                "FemGui.getActiveAnalysis().addObject(ObjectsFem."
                "makeSolverCalculixCcxTools(FreeCAD.ActiveDocument))"
            )
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _SolverCalculix(CommandManager):
    "The FEM_SolverCalculix command definition"

    def __init__(self):
        super(_SolverCalculix, self).__init__()
        self.pixmap = "FEM_SolverStandard"
        self.menuetext = "Solver CalculiX (experimental)"
        self.accel = "S, C"
        self.tooltip = "Creates a FEM solver CalculiX (experimental)"
        self.is_active = "with_analysis"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_noset_edit"


class _SolverControl(CommandManager):
    "The FEM_SolverControl command definition"

    def __init__(self):
        super(_SolverControl, self).__init__()
        self.menuetext = "Solver job control"
        self.accel = "S, T"
        self.tooltip = "Changes solver attributes and runs the calculations for the selected solver"
        self.is_active = "with_solver"

    def Activated(self):
        FreeCADGui.ActiveDocument.setEdit(self.selobj, 0)


class _SolverElmer(CommandManager):
    "The FEM_SolverElmer command definition"

    def __init__(self):
        super(_SolverElmer, self).__init__()
        self.menuetext = "Solver Elmer"
        self.accel = "S, E"
        self.tooltip = "Creates a FEM solver Elmer"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_noset_edit"


class _SolverRun(CommandManager):
    "The FEM_SolverRun command definition"

    def __init__(self):
        super(_SolverRun, self).__init__()
        self.menuetext = "Run solver calculations"
        self.accel = "S, R"
        self.tooltip = "Runs the calculations for the selected solver"
        self.is_active = "with_solver"

    def Activated(self):
        from femsolver.run import run_fem_solver
        run_fem_solver(self.selobj)
        FreeCADGui.Selection.clearSelection()
        FreeCAD.ActiveDocument.recompute()


class _SolverZ88(CommandManager):
    "The FEM_SolverZ88 command definition"

    def __init__(self):
        super(_SolverZ88, self).__init__()
        self.pixmap = "FEM_SolverZ88.svg"
        self.menuetext = "Solver Z88"
        self.accel = "S, Z"
        self.tooltip = "Creates a FEM solver Z88"
        self.is_active = "with_analysis"
        self.do_activated = "add_obj_on_gui_noset_edit"


# the string in add command will be the page name on FreeCAD wiki
FreeCADGui.addCommand(
    "FEM_Analysis",
    _Analysis()
)
FreeCADGui.addCommand(
    "FEM_ClippingPlaneAdd",
    _ClippingPlaneAdd()
)
FreeCADGui.addCommand(
    "FEM_ClippingPlaneRemoveAll",
    _ClippingPlaneRemoveAll()
)
FreeCADGui.addCommand(
    "FEM_ConstantVacuumPermittivity",
    _ConstantVacuumPermittivity()
)
FreeCADGui.addCommand(
    "FEM_ConstraintBodyHeatSource",
    _ConstraintBodyHeatSource()
)
FreeCADGui.addCommand(
    "FEM_ConstraintElectrostaticPotential",
    _ConstraintElectrostaticPotential()
)
FreeCADGui.addCommand(
    "FEM_ConstraintFlowVelocity",
    _ConstraintFlowVelocity()
)
FreeCADGui.addCommand(
    "FEM_ConstraintInitialFlowVelocity",
    _ConstraintInitialFlowVelocity()
)
FreeCADGui.addCommand(
    "FEM_ConstraintSectionPrint",
    _ConstraintSectionPrint()
)
FreeCADGui.addCommand(
    "FEM_ConstraintSelfWeight",
    _ConstraintSelfWeight()
)
FreeCADGui.addCommand(
    "FEM_ConstraintTie",
    _ConstraintTie()
)
FreeCADGui.addCommand(
    "FEM_ElementFluid1D",
    _ElementFluid1D()
)
FreeCADGui.addCommand(
    "FEM_ElementGeometry1D",
    _ElementGeometry1D()
)
FreeCADGui.addCommand(
    "FEM_ElementGeometry2D",
    _ElementGeometry2D()
)
FreeCADGui.addCommand(
    "FEM_ElementRotation1D",
    _ElementRotation1D()
)
FreeCADGui.addCommand(
    "FEM_EquationElectrostatic",
    _EquationElectrostatic()
)
FreeCADGui.addCommand(
    "FEM_EquationElasticity",
    _EquationElasticity()
)
FreeCADGui.addCommand(
    "FEM_EquationFlow",
    _EquationFlow()
)
FreeCADGui.addCommand(
    "FEM_EquationFlux",
    _EquationFlux()
)
FreeCADGui.addCommand(
    "FEM_EquationElectricforce",
    _EquationElectricforce()
)
FreeCADGui.addCommand(
    "FEM_EquationHeat",
    _EquationHeat()
)
FreeCADGui.addCommand(
    "FEM_Examples",
    _Examples()
)
FreeCADGui.addCommand(
    "FEM_MaterialEditor",
    _MaterialEditor()
)
FreeCADGui.addCommand(
    "FEM_MaterialFluid",
    _MaterialFluid()
)
FreeCADGui.addCommand(
    "FEM_MaterialMechanicalNonlinear",
    _MaterialMechanicalNonlinear()
)
FreeCADGui.addCommand(
    "FEM_MaterialReinforced",
    _MaterialReinforced()
)
FreeCADGui.addCommand(
    "FEM_MaterialSolid",
    _MaterialSolid()
)
FreeCADGui.addCommand(
    "FEM_FEMMesh2Mesh",
    _FEMMesh2Mesh()
)
FreeCADGui.addCommand(
    "FEM_MeshBoundaryLayer",
    _MeshBoundaryLayer()
)
FreeCADGui.addCommand(
    "FEM_MeshClear",
    _MeshClear()
)
FreeCADGui.addCommand(
    "FEM_MeshDisplayInfo",
    _MeshDisplayInfo()
)
FreeCADGui.addCommand(
    "FEM_MeshGmshFromShape",
    _MeshGmshFromShape()
)
FreeCADGui.addCommand(
    "FEM_MeshGroup",
    _MeshGroup()
)
FreeCADGui.addCommand(
    "FEM_MeshNetgenFromShape",
    _MeshNetgenFromShape()
)
FreeCADGui.addCommand(
    "FEM_MeshRegion",
    _MeshRegion()
)
FreeCADGui.addCommand(
    "FEM_ResultShow",
    _ResultShow()
)
FreeCADGui.addCommand(
    "FEM_ResultsPurge",
    _ResultsPurge()
)
FreeCADGui.addCommand(
    "FEM_SolverCalculixCxxtools",
    _SolverCxxtools()
)
FreeCADGui.addCommand(
    "FEM_SolverCalculiX",
    _SolverCalculix()
)
FreeCADGui.addCommand(
    "FEM_SolverControl",
    _SolverControl()
)
FreeCADGui.addCommand(
    "FEM_SolverElmer",
    _SolverElmer()
)
FreeCADGui.addCommand(
    "FEM_SolverRun",
    _SolverRun()
)
FreeCADGui.addCommand(
    "FEM_SolverZ88",
    _SolverZ88()
)
