# ***************************************************************************
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver Z88 writer"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import time
from os.path import join

import FreeCAD

from .. import writerbase
from feminout import importZ88Mesh
from femmesh import meshtools


class FemInputWriterZ88(writerbase.FemInputWriter):
    def __init__(
        self,
        analysis_obj,
        solver_obj,
        mesh_obj,
        member,
        dir_name=None
    ):
        writerbase.FemInputWriter.__init__(
            self,
            analysis_obj,
            solver_obj,
            mesh_obj,
            member,
            dir_name
        )
        self.file_name = join(self.dir_name, "z88")

    # ********************************************************************************************
    # write solver input
    def write_solver_input(self):
        timestart = time.process_time()
        FreeCAD.Console.PrintMessage("\n")  # because of time print in separate line
        FreeCAD.Console.PrintMessage("Z88 solver input writing...\n")
        FreeCAD.Console.PrintLog(
            "FemInputWriterZ88 --> self.dir_name  -->  {}\n"
            .format(self.dir_name)
        )
        FreeCAD.Console.PrintMessage(
            "FemInputWriterZ88 --> self.file_name  -->  {}\n"
            .format(self.file_name)
        )
        FreeCAD.Console.PrintMessage(
            "Write z88 input files to: {}\n"
            .format(self.dir_name)
        )
        control = self.set_z88_elparam()
        if control is False:
            return None
        self.write_z88_mesh()
        self.write_z88_constraints()
        self.write_z88_face_loads()
        self.write_z88_materials()
        self.write_z88_elements_properties()
        self.write_z88_integration_properties()
        self.write_z88_memory_parameter()
        self.write_z88_solver_parameter()
        writing_time_string = (
            "Writing time input file: {} seconds"
            .format(round((time.process_time() - timestart), 2))
        )
        FreeCAD.Console.PrintMessage(
            "{}\n\n".format(writing_time_string))
        return self.dir_name

    # ********************************************************************************************
    def set_z88_elparam(self):
        # TODO: param should be moved to the solver object like the known analysis
        z8804 = {"INTORD": "0", "INTOS": "0", "IHFLAG": "0", "ISFLAG": "1"}  # seg2 --> stab4
        z8824 = {"INTORD": "7", "INTOS": "7", "IHFLAG": "1", "ISFLAG": "1"}  # tria6 --> schale24
        z8823 = {"INTORD": "3", "INTOS": "0", "IHFLAG": "1", "ISFLAG": "0"}  # quad8 --> schale23
        z8817 = {"INTORD": "4", "INTOS": "0", "IHFLAG": "0", "ISFLAG": "0"}  # tetra4 --> volume17
        z8816 = {"INTORD": "4", "INTOS": "0", "IHFLAG": "0", "ISFLAG": "0"}  # tetra10 --> volume16
        z8801 = {"INTORD": "2", "INTOS": "2", "IHFLAG": "0", "ISFLAG": "1"}  # hexa8 --> volume1
        z8810 = {"INTORD": "3", "INTOS": "0", "IHFLAG": "0", "ISFLAG": "0"}  # hexa20 --> volume10
        param = {4: z8804, 24: z8824, 23: z8823, 17: z8817, 16: z8816, 1: z8801, 10: z8810}
        # TODO: test elements 17, 16, 10, INTORD etc
        self.z88_element_type = importZ88Mesh.get_z88_element_type(
            self.femmesh,
            self.femelement_table
        )
        if self.z88_element_type in param:
            self.z88_elparam = param[self.z88_element_type]
        else:
            FreeCAD.Console.PrintError(
                "Element type not supported by Z88. Can not write Z88 solver input.\n")
            return False
        FreeCAD.Console.PrintMessage(self.z88_elparam)
        FreeCAD.Console.PrintMessage("\n")
        return True

    # ********************************************************************************************
    def write_z88_mesh(self):
        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        if not self.femelement_table:
            self.femelement_table = meshtools.get_femelement_table(self.femmesh)
            self.element_count = len(self.femelement_table)
        mesh_file_path = self.file_name + "i1.txt"
        f = open(mesh_file_path, "w")
        importZ88Mesh.write_z88_mesh_to_file(
            self.femnodes_mesh,
            self.femelement_table,
            self.z88_element_type,
            f
        )
        f.close()

    # ********************************************************************************************
    def write_z88_constraints(self):
        constraints_data = []  # will be a list of tuple for better sorting

        # fixed constraints
        # write nodes to constraints_data (different from writing to file in ccxInpWriter
        for femobj in self.member.cons_fixed:
            for n in femobj["Nodes"]:
                constraints_data.append((n, "{}  1  2  0\n".format(n)))
                constraints_data.append((n, "{}  2  2  0\n".format(n)))
                constraints_data.append((n, "{}  3  2  0\n".format(n)))

        # forces constraints
        # write node loads to constraints_data
        # a bit different from writing to file for ccxInpWriter
        for femobj in self.member.cons_force:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            direction_vec = femobj["Object"].DirectionVector
            for ref_shape in femobj["NodeLoadTable"]:
                for n in sorted(ref_shape[1]):
                    # the loads in ref_shape[1][n] are without unit
                    node_load = ref_shape[1][n]
                    if (direction_vec.x != 0.0):
                        v1 = direction_vec.x * node_load
                        constraints_data.append((n, "{}  1  1  {}\n".format(n, v1)))
                    if (direction_vec.y != 0.0):
                        v2 = direction_vec.y * node_load
                        constraints_data.append((n, "{}  2  1  {}\n".format(n, v2)))
                    if (direction_vec.z != 0.0):
                        v3 = direction_vec.z * node_load
                        constraints_data.append((n, "{}  3  1  {}\n".format(n, v3)))

        # write constraints_data to file
        constraints_file_path = self.file_name + "i2.txt"
        f = open(constraints_file_path, "w")
        f.write(str(len(constraints_data)) + "\n")
        for c in sorted(constraints_data):
            f.write(c[1])
        f.close()

    # ********************************************************************************************
    def write_z88_face_loads(self):
        # not yet supported
        face_load_file_path = self.file_name + "i5.txt"
        f = open(face_load_file_path, "w")
        f.write(" 0")
        f.write("\n")
        f.close()

    # ********************************************************************************************
    def write_z88_materials(self):
        mat_obj = self.member.mats_linear[0]["Object"]
        material_data_file_name = "51.txt"
        materials_file_path = self.file_name + "mat.txt"
        fms = open(materials_file_path, "w")
        fms.write("1\n")
        fms.write("1 {} {}".format(self.element_count, material_data_file_name))
        fms.write("\n")
        fms.close()
        material_data_file_path = join(self.dir_name, material_data_file_name)
        fmd = open(material_data_file_path, "w")
        YM = FreeCAD.Units.Quantity(mat_obj.Material["YoungsModulus"])
        YM_in_MPa = YM.getValueAs("MPa")
        PR = float(mat_obj.Material["PoissonRatio"])
        fmd.write("{0} {1:.3f}".format(YM_in_MPa, PR))
        fmd.write("\n")
        fmd.close()

    # ********************************************************************************************
    def write_z88_elements_properties(self):
        element_properties_file_path = self.file_name + "elp.txt"
        elements_data = []
        if meshtools.is_edge_femmesh(self.femmesh):
            beam_obj = self.member.geos_beamsection[0]["Object"]
            area = 0
            if beam_obj.SectionType == "Rectangular":
                width = beam_obj.RectWidth.getValueAs("mm").Value
                height = beam_obj.RectHeight.getValueAs("mm").Value
                area = width * height
            elif beam_obj.SectionType == "Circular":
                diameter = beam_obj.CircDiameter.getValueAs("mm").Value
                from math import pi
                area = 0.25 * pi * diameter * diameter
            else:
                FreeCAD.Console.PrintError(
                    "Cross section type {} not supported, "
                    "cross section area will be 0 in solver input.\n"
                    .format(beam_obj.SectionType)
                )
                # TODO make the check in prechecks and delete it here
                # no extensive errorhandling in writer
                # this way the solver will fail and an exception is raised somehow
            elements_data.append(
                "1 {} {} 0 0 0 0 0 0 "
                .format(self.element_count, area)
            )
            FreeCAD.Console.PrintWarning(
                "Be aware, only trusses are supported for edge meshes!\n"
            )
        elif meshtools.is_face_femmesh(self.femmesh):
            thick_obj = self.member.geos_shellthickness[0]["Object"]
            thickness = thick_obj.Thickness.getValueAs("mm").Value
            elements_data.append(
                "1 {} {} 0 0 0 0 0 0 "
                .format(self.element_count, thickness)
            )
        elif meshtools.is_solid_femmesh(self.femmesh):
            elements_data.append(
                "1 {} 0 0 0 0 0 0 0"
                .format(self.element_count)
            )
        else:
            FreeCAD.Console.PrintError("Error!\n")
        f = open(element_properties_file_path, "w")
        f.write(str(len(elements_data)) + "\n")
        for e in elements_data:
            f.write(e)
        f.write("\n")
        f.close()

    # ********************************************************************************************
    def write_z88_integration_properties(self):
        integration_data = []
        integration_data.append("1 {} {} {}".format(
            self.element_count,
            self.z88_elparam["INTORD"],
            self.z88_elparam["INTOS"]
        ))
        integration_properties_file_path = self.file_name + "int.txt"
        f = open(integration_properties_file_path, "w")
        f.write("{}\n".format(len(integration_data)))
        for i in integration_data:
            f.write(i)
        f.write("\n")
        f.close()

    # ********************************************************************************************
    def write_z88_solver_parameter(self):
        global z88_man_template
        z88_man_template = z88_man_template.replace(
            "$z88_param_ihflag", str(self.z88_elparam["IHFLAG"])
        )
        z88_man_template = z88_man_template.replace(
            "$z88_param_isflag", str(self.z88_elparam["ISFLAG"])
        )
        solver_parameter_file_path = self.file_name + "man.txt"
        f = open(solver_parameter_file_path, "w")
        f.write(z88_man_template)
        f.close()

    # ********************************************************************************************
    def write_z88_memory_parameter(self):
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Z88")
        MaxGS = prefs.GetInt("MaxGS", 100000000)
        MaxKOI = prefs.GetInt("MaxKOI", 2800000)
        global z88_dyn_template
        template_array = z88_dyn_template.splitlines()
        output = ""
        for line in template_array:
            if line.find("MAXGS") > -1:
                line = "    MAXGS  {}".format(MaxGS)
            if line.find("MAXKOI") > -1:
                line = "    MAXKOI   {}".format(MaxKOI)
            output += line + "\n"

        solver_parameter_file_path = self.file_name + ".dyn"
        f = open(solver_parameter_file_path, "w")
        f.write(output)
        f.close()


# for solver parameter file Z88man.txt
z88_man_template = """DYNAMIC START
---------------------------------------------------------------------------
Z88V14OS
---------------------------------------------------------------------------

---------------------------------------------------------------------------
GLOBAL
---------------------------------------------------------------------------

GLOBAL START
   IBFLAG          0
   IPFLAG          0
   IHFLAG          $z88_param_ihflag
GLOBAL END

---------------------------------------------------------------------------
LINEAR SOLVER
---------------------------------------------------------------------------

SOLVER START
   MAXIT           10000
   EPS             1e-007
   RALPHA          0.0001
   ROMEGA          1.1
SOLVER END

---------------------------------------------------------------------------
STRESS
---------------------------------------------------------------------------

STRESS START
   KDFLAG        0
   ISFLAG        $z88_param_isflag
STRESS END

DYNAMIC END
"""

# for memory parameter file z88.dyn
z88_dyn_template = """DYNAMIC START
---------------------------------------------------------------------------
Z88 new version 14OS                   Z88 neue Version 14OS
---------------------------------------------------------------------------

---------------------------------------------------------------------------
LANGUAGE                   SPRACHE
---------------------------------------------------------------------------
GERMAN

---------------------------------------------------------------------------
Entries for mesh generator Z88N        Daten fuer Netzgenerator
---------------------------------------------------------------------------
  NET START
    MAXSE  40000
    MAXESS   800
    MAXKSS  4000
    MAXAN     15
  NET END

---------------------------------------------------------------------------
Common entries for all modules         gemeinsame Daten fuer alle Module
---------------------------------------------------------------------------

  COMMON START
    MAXGS  100000000
    MAXKOI   2800000
    MAXK       60000
    MAXE      300000
    MAXNFG    200000
    MAXMAT        32
    MAXPEL        32
    MAXJNT        32
    MAXPR      10000
    MAXRBD     15000
    MAXIEZ   6000000
    MAXGP    2000000
  COMMON END

---------------------------------------------------------------------------
Entries for Cuthill-McKee Z88H         Daten fuer Cuthill- McKee Programm
---------------------------------------------------------------------------
  CUTKEE START
    MAXGRA  200
    MAXNDL 1000
  CUTKEE END


DYNAMIC END
"""

##  @}
