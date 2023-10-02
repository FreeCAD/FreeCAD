# ***************************************************************************
# *   Copyright (c) 2015 Przemo Firszt <przemo@firszt.eu>                   *
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver CalculiX writer"
__author__ = "Przemo Firszt, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import time
from os.path import join

import FreeCAD
from FreeCAD import Units

from . import write_constraint_centrif as con_centrif
from . import write_constraint_contact as con_contact
from . import write_constraint_displacement as con_displacement
from . import write_constraint_fixed as con_fixed
from . import write_constraint_fluidsection as con_fluidsection
from . import write_constraint_force as con_force
from . import write_constraint_heatflux as con_heatflux
from . import write_constraint_initialtemperature as con_itemp
from . import write_constraint_planerotation as con_planerotation
from . import write_constraint_pressure as con_pressure
from . import write_constraint_sectionprint as con_sectionprint
from . import write_constraint_selfweight as con_selfweight
from . import write_constraint_temperature as con_temperature
from . import write_constraint_tie as con_tie
from . import write_constraint_transform as con_transform
from . import write_femelement_geometry
from . import write_femelement_material
from . import write_femelement_matgeosets
from . import write_footer
from . import write_mesh
from . import write_step_equation
from . import write_step_output
from .. import writerbase
from femtools import constants


# Interesting forum topic: https://forum.freecad.org/viewtopic.php?&t=48451
# TODO somehow set units at beginning and every time a value is retrieved use this identifier
# this would lead to support of unit system, force might be retrieved in base writer!


# the following text will be at the end of the main calculix input file
units_information = """***********************************************************
**  About units:
**  See ccx manual, ccx does not know about any unit.
**  Golden rule: The user must make sure that the numbers they provide have consistent units.
**  The user is the FreeCAD calculix writer module ;-)
**
**  The unit system which is used at Guido Dhondt's company: mm, N, s, K
**  Since Length and Mass are connected by Force, if Length is mm the Mass is in t to get N
**  The following units are used to write to inp file:
**
**  Length: mm (this includes the mesh geometry)
**  Mass: t
**  TimeSpan: s
**  Temperature: K
**
**  This leads to:
**  Force: N
**  Pressure: N/mm^2 == MPa (Young's Modulus has unit Pressure)
**  Density: t/mm^3
**  Gravity: mm/s^2
**  Thermal conductivity: t*mm/K/s^3 == as W/m/K == kW/mm/K
**  Specific Heat: mm^2/s^2/K = J/kg/K == kJ/t/K
"""


# TODO
# {0:.13G} or {:.13G} should be used on all places writing floating points to ccx
# All floating points fields read from ccx are F20.0 FORTRAN input fields.
# see in dload.f in ccx's source
# https://forum.freecad.org/viewtopic.php?f=18&p=516518#p516433
# https://forum.freecad.org/viewtopic.php?f=18&t=22759&#p176578
# example "{:.13G}".format(math.sqrt(2.)*-1e100) and count chars
# a property type is best checked in FreeCAD objects definition
# see femobjects package for Python objects or in objects App


class FemInputWriterCcx(writerbase.FemInputWriter):
    def __init__(
        self,
        analysis_obj,
        solver_obj,
        mesh_obj,
        member,
        dir_name=None,
        mat_geo_sets=None
    ):
        writerbase.FemInputWriter.__init__(
            self,
            analysis_obj,
            solver_obj,
            mesh_obj,
            member,
            dir_name,
            mat_geo_sets
        )
        self.mesh_name = self.mesh_object.Name
        self.file_name = join(self.dir_name, self.mesh_name + ".inp")
        self.femmesh_file = ""  # the file the femmesh is in, no matter if one or split input file
        self.gravity = int(Units.Quantity(constants.gravity()).getValueAs("mm/s^2"))  # 9820 mm/s2
        self.units_information = units_information

    # ********************************************************************************************
    # write calculix input
    def write_solver_input(self):

        time_start = time.process_time()
        FreeCAD.Console.PrintMessage("\n")  # because of time print in separate line
        FreeCAD.Console.PrintMessage("CalculiX solver input writing...\n")
        FreeCAD.Console.PrintMessage(
            "Input file:{}\n"
            .format(self.file_name)
        )

        if self.solver_obj.SplitInputWriter is True:
            FreeCAD.Console.PrintMessage("Split input file.\n")
            self.split_inpfile = True
        else:
            FreeCAD.Console.PrintMessage("One monster input file.\n")
            self.split_inpfile = False

        # mesh
        inpfile = write_mesh.write_mesh(self)

        # element sets for materials and element geometry
        write_femelement_matgeosets.write_femelement_matgeosets(inpfile, self)

        # some fluidsection objs need special treatment, mat_geo_sets are needed for this
        inpfile = con_fluidsection.handle_fluidsection_liquid_inlet_outlet(inpfile, self)

        # element sets constraints
        self.write_constraints_meshsets(inpfile, self.member.cons_centrif, con_centrif)

        # node sets
        self.write_constraints_meshsets(inpfile, self.member.cons_fixed, con_fixed)
        self.write_constraints_meshsets(inpfile, self.member.cons_displacement, con_displacement)
        self.write_constraints_meshsets(inpfile, self.member.cons_planerotation, con_planerotation)
        self.write_constraints_meshsets(inpfile, self.member.cons_transform, con_transform)
        self.write_constraints_meshsets(inpfile, self.member.cons_temperature, con_temperature)

        # surface sets
        self.write_constraints_meshsets(inpfile, self.member.cons_contact, con_contact)
        self.write_constraints_meshsets(inpfile, self.member.cons_tie, con_tie)
        self.write_constraints_meshsets(inpfile, self.member.cons_sectionprint, con_sectionprint)

        # materials and fem element types
        write_femelement_material.write_femelement_material(inpfile, self)
        self.write_constraints_propdata(inpfile, self.member.cons_initialtemperature, con_itemp)
        write_femelement_geometry.write_femelement_geometry(inpfile, self)

        # constraints independent from steps
        self.write_constraints_propdata(inpfile, self.member.cons_planerotation, con_planerotation)
        self.write_constraints_propdata(inpfile, self.member.cons_contact, con_contact)
        self.write_constraints_propdata(inpfile, self.member.cons_tie, con_tie)
        self.write_constraints_propdata(inpfile, self.member.cons_transform, con_transform)

        # step equation
        write_step_equation.write_step_equation(inpfile, self)

        # constraints dependent from steps
        self.write_constraints_propdata(inpfile, self.member.cons_fixed, con_fixed)
        self.write_constraints_propdata(inpfile, self.member.cons_displacement, con_displacement)
        self.write_constraints_propdata(inpfile, self.member.cons_sectionprint, con_sectionprint)
        self.write_constraints_propdata(inpfile, self.member.cons_selfweight, con_selfweight)
        self.write_constraints_propdata(inpfile, self.member.cons_centrif, con_centrif)
        self.write_constraints_meshsets(inpfile, self.member.cons_force, con_force)
        self.write_constraints_meshsets(inpfile, self.member.cons_pressure, con_pressure)
        self.write_constraints_propdata(inpfile, self.member.cons_temperature, con_temperature)
        self.write_constraints_meshsets(inpfile, self.member.cons_heatflux, con_heatflux)
        con_fluidsection.write_constraints_fluidsection(inpfile, self)

        # output and step end
        write_step_output.write_step_output(inpfile, self)
        write_step_equation.write_step_end(inpfile, self)

        # footer
        write_footer.write_footer(inpfile, self)

        # close file
        inpfile.close()

        writetime = round((time.process_time() - time_start), 3)
        FreeCAD.Console.PrintMessage(
            "Writing time CalculiX input file: {} seconds.\n".format(writetime)
        )

        # return
        if self.femelement_count_test is True:
            return self.file_name
        else:
            FreeCAD.Console.PrintError(
                "Problems on writing input file, check report prints.\n\n"
            )
            return ""

##  @}
