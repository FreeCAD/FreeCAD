# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
# *   Copyright (c) 2021 Tobias Vaara <t@vaara.se>                          *
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

# to run the example use:
"""
from femexamples.ccx_buckling_lateraltorsionalbuckling import setup
setup()

"""

import FreeCAD
import Fem
import ObjectsFem


mesh_name = "Mesh"  # needs to be Mesh to work with unit tests

# Steel H-profile example HEA300, in millimeter.
b = 290  # Width of flanges
t = 14  # Thickness of flanges
h = 300  # Total height of beam
d = 8  # Thickness of web
l = 8000  # Length of beam

# Distributed load on top flange(Newton).
force_load = 200000.0 #Equivalent to 25kN/m line load.

class calc_ltp_h_beam():  # Only for welded I-beams that are symmetrical in both axis.
    def __init__(self, web, flanges, height, width, length):
        self.web = web
        self.flanges = flanges
        self.height = height
        self.width = width
        self.length = length
        self.E = 210000  # N/mm2
        self.G = 80769  # N/mm2
        self.I_z = self.calc_I_z()
        self.I_t = self.calc_I_t()
        self.I_w = self.calc_I_w()

    def calc_I_z(self):
        i_z = ((self.flanges * (self.width ** 3)) / 12) * 2 + (
                    ((self.height - 2 * self.flanges) * (self.web ** 3)) / 12)

        return i_z

    def calc_I_t(self):
        i_t = (self.width * self.flanges ** 3) / 3 + (self.width * self.flanges ** 3) / 3 + (
                    (self.height - self.flanges) * self.web ** 3) / 3

        return i_t

    def calc_I_w(self):
        alpha = 0.5  # for symmetrical cross-sections
        i_w = (((self.height - self.flanges) ** 2) * (self.width ** 3) * self.flanges * alpha) / 12

        return i_w

    def M_cr(self):  # only values for this specific example

        C_1 = 1.13  # Coefficient depending on the shape of the moment diagram
        C_2 = 0.45  # Coefficient depending on the point of load application relative to the shear centre
        C_3 = 0.525  # Coefficient depending on the symmetry of the cross-section around the weak axis
        k_z = 1  # k_z and k_w are effective length factors that takes rotational and warping into account.
        k_w = 1  # they are usually set to 1.0.
        z_s = 0  # Vertical coordinate for the shear center
        z_a = self.height / 2  # Vertical coordinate for the point of load application
        z_g = z_a - z_s
        z_j = 0  # Only used if cross-section is assymetric about its major axis.
        Z = C_2 * z_g - C_3 * z_j

        # Critical ltp moment 3 formula factor
        Mcr_1 = ((C_1 * 3.1415 ** 2) * (self.E * self.I_z)) / ((k_z * self.length) ** 2)
        Mcr_2 = ((k_z / k_w) ** 2) * (self.I_w / self.I_z)
        Mcr_3 = (((k_z * self.length) ** 2) * self.G * self.I_t) / ((3.1415 ** 2) * self.E * self.I_z) + Z ** 2
        Mcr_tot = ((Mcr_2 + Mcr_3) ** 0.5) - Z
        Mcr = Mcr_1 * Mcr_tot

        return Mcr

    def Moment(self):

        lineload = force_load/self.length
        m_ = (lineload*self.length**2)/8
        return m_



def addbox(docxx, height, width, length, x, y, z, box_name):
    box_obj = docxx.addObject('Part::Box', box_name)
    box_obj.Height = height
    box_obj.Width = width
    box_obj.Length = length
    box_obj.Placement = FreeCAD.Placement(FreeCAD.Vector(x, y, z), FreeCAD.Rotation(0, 0, 0))


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "Linear Buckling Analysis Static",
            "meshtype": "solid",
            "meshelement": "Tet10",
            "constraints": ["force", "displacement"],
            "solvers": ["calculix"],
            "material": "solid",
            "equation": "mechanical"
            }
    return info


def setup_base(doc=None, solvertype="ccxtools"):

    # setup box base model
    if doc is None:
        doc = init_doc()

    addbox(doc, t, b, l, 0, -b / 2, 0, 'Bottom flange')
    addbox(doc, h - 2 * t, d, l, 0, -d / 2, t, 'Web')
    addbox(doc, t, b, l, 0, -b / 2, h - t, 'Top flange')

    shape = []
    for i in doc.Objects:

        if i.isDerivedFrom("Part::Feature"):
            shape.append(i)

    doc.addObject("Part::MultiFuse", "beam").Shapes = shape

    doc.recompute()

    geom_obj = doc.beam

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # material
    material_object = analysis.addObject(ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial"))[0]
    mat = material_object.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7900 kg/m^3"
    material_object.Material = mat

    # mesh
    from .meshes.mesh_linear_buckling import create_nodes, create_elements

    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, mesh_name))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Part = doc.beam
    #femmesh_obj.SecondOrderLinear = False
    #femmesh_obj.CharacteristicLengthMin = "8.0 mm"

    #document
    document_txt = doc.addObject("App::TextDocument", "Comparison, manual calculation")
    beam = calc_ltp_h_beam(d,t,h,b,l)


    document_txt.Text = "Manual calculation of the elastic critical moment with the 3-factor formula\n\n"

    document_txt.Text += "CROSS-SECTION DATA\n"
    document_txt.Text += "d = " + str(beam.web) + " mm  Thickness Web\n"
    document_txt.Text += "t = " + str(beam.flanges) + " mm  Thickness Flange\n"
    document_txt.Text += "h = " + str(beam.height) + " mm  Height of beam\n"
    document_txt.Text += "w = " + str(beam.width) + " mm  Width of beam\n"
    document_txt.Text += "l = " + str(beam.length) + " mm  Length of beam\n"
    document_txt.Text += "Iz = " + str(beam.I_z) + " mm4  Moment of inertia, z-axis\n"
    document_txt.Text += "It = " + str(beam.I_t) + " mm4  Torsion constant\n"
    document_txt.Text += "Iw = " + str(beam.I_w) + " mm6  Warping constant\n"
    document_txt.Text += "\n\n"

    document_txt.Text += "LOAD DATA\n"
    document_txt.Text += "Q = " + str(force_load) + " N Total load on top flange\n"
    document_txt.Text += "q = " + str(force_load/beam.length) + " N/mm Line load on top flange\n"
    document_txt.Text += "\n\n"
    document_txt.Text += "RESULT\n"
    document_txt.Text += "M = " + str(beam.Moment()) + " Nmm Calculated moment\n"
    document_txt.Text += "M_cr = " + str(beam.M_cr()) + " Nmm Calculated Elastic Critical moment from the 3-Factor formula\n"
    document_txt.Text += "\n"
    document_txt.Text += "Buckling factor :" + str(beam.M_cr()/beam.Moment())
    doc.recompute()
    return doc


def setup(doc=None, solvertype="ccxtools"):
    #setup

    doc = setup_base(doc, solvertype)

    analysis = doc.Analysis

    # solver
    if solvertype == "calculix":
        solver_object = analysis.addObject(
            ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
        )[0]
    elif solvertype == "ccxtools":
        solver_object = analysis.addObject(
            ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        )[0]
        solver_object.WorkingDir = u""

    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_object.SplitInputWriter = False
        solver_object.AnalysisType = "buckling"
        solver_object.BucklingFactors = 1
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsControlParameterTimeUse = False

    ## displacement constraint
    displacement_constraint = ObjectsFem.makeConstraintDisplacement(doc, "FemConstraintDisplacement")
    displacement_constraint.References = [(doc.beam, ("Face12", "Face4"))]
    displacement_constraint.zFix = True
    displacement_constraint.zFree = False
    displacement_constraint.yFix = True
    displacement_constraint.yFree = False
    analysis.addObject(displacement_constraint)

    displacement_constraint2 = ObjectsFem.makeConstraintDisplacement(doc, "FemConstraintDisplacement2")
    displacement_constraint2.References = [(doc.beam, ("Edge6"))]

    displacement_constraint2.xFix = True
    displacement_constraint2.xFree = False
    analysis.addObject(displacement_constraint2)

    ## force_constraint
    force_constraint = ObjectsFem.makeConstraintForce(doc, "FemConstraintForce")
    force_constraint.References = [(doc.beam, "Face15")]
    force_constraint.Force = force_load
    force_constraint.Reversed = True
    analysis.addObject(force_constraint)

    doc.recompute()

    return doc