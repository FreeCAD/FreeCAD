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
__url__ = "https://www.freecadweb.org"

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
from . import write_constraint_initialtemperature as con_initialtemp
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


# Interesting forum topic: https://forum.freecadweb.org/viewtopic.php?&t=48451
# TODO somehow set units at beginning and every time a value is retrieved use this identifier
# this would lead to support of unit system, force might be retrieved in base writer!


# the following text will be at the end of the main calculix input file
units_information = """***********************************************************
**  About units:
**  See ccx manual, ccx does not know about any unit.
**  Golden rule: The user must make sure that the numbers he provides have consistent units.
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
**  Pressure: N/mm^2
**  Density: t/mm^3
**  Gravity: mm/s^2
**  Thermal conductivity: t*mm/K/s^3 (same as W/m/K)
**  Specific Heat: mm^2/s^2/K (same as J/kg/K)
"""


class FemInputWriterCcx(writerbase.FemInputWriter):
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
        self.mesh_name = self.mesh_object.Name
        self.include = join(self.dir_name, self.mesh_name)
        self.file_name = self.include + ".inp"
        self.femmesh_file = ""  # the file the femmesh is in, no matter if one or split input file
        self.gravity = int(Units.Quantity(constants.gravity()).getValueAs("mm/s^2"))  # 9820 mm/s2
        self.units_information = units_information

    # ********************************************************************************************
    # write calculix input
    def write_calculix_input_file(self):
        timestart = time.process_time()
        FreeCAD.Console.PrintMessage("Start writing CalculiX input file\n")
        FreeCAD.Console.PrintMessage("Write ccx input file to: {}\n".format(self.file_name))
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.mesh_name  -->  {}\n".format(self.mesh_name)
        )
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.dir_name  -->  {}\n".format(self.dir_name)
        )
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.include  -->  {}\n".format(self.mesh_name)
        )
        FreeCAD.Console.PrintLog(
            "writerbaseCcx --> self.file_name  -->  {}\n".format(self.file_name)
        )

        self.get_mesh_sets()
        self.write_file()

        FreeCAD.Console.PrintMessage(
            "Writing time CalculiX input file: {} seconds \n\n"
            .format(round((time.process_time() - timestart), 2))
        )
        if self.femelement_count_test is True:
            return self.file_name
        else:
            FreeCAD.Console.PrintError(
                "Problems on writing input file, check report prints.\n\n"
            )
            return ""

    def write_file(self):
        FreeCAD.Console.PrintMessage("Start writing input file\n")
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

        if self.fluidsection_objects:
            # some fluidsection objs need special treatment, ccx_elsets are needed for this
            inpfile = con_fluidsection.handle_fluidsection_liquid_inlet_outlet(inpfile, self)

        # element sets constraints
        self.write_constraints_sets(inpfile, self.centrif_objects, con_centrif)

        # node sets
        self.write_constraints_sets(inpfile, self.fixed_objects, con_fixed)
        self.write_constraints_sets(inpfile, self.displacement_objects, con_displacement)
        self.write_constraints_sets(inpfile, self.planerotation_objects, con_planerotation)
        self.write_constraints_sets(inpfile, self.transform_objects, con_transform)
        self.write_constraints_sets(inpfile, self.temperature_objects, con_temperature)

        # surface sets
        self.write_constraints_sets(inpfile, self.contact_objects, con_contact)
        self.write_constraints_sets(inpfile, self.tie_objects, con_tie)
        self.write_constraints_sets(inpfile, self.sectionprint_objects, con_sectionprint)

        # materials and fem element types
        write_femelement_material.write_femelement_material(inpfile, self)
        self.write_constraints_data(inpfile, self.initialtemperature_objects, con_initialtemp)
        write_femelement_geometry.write_femelement_geometry(inpfile, self)

        # constraints independent from steps
        self.write_constraints_data(inpfile, self.planerotation_objects, con_planerotation)
        self.write_constraints_data(inpfile, self.contact_objects, con_contact)
        self.write_constraints_data(inpfile, self.tie_objects, con_tie)
        self.write_constraints_data(inpfile, self.transform_objects, con_transform)

        # step equation
        write_step_equation.write_step_equation(inpfile, self)

        # constraints dependent from steps
        self.write_constraints_data(inpfile, self.fixed_objects, con_fixed)
        self.write_constraints_data(inpfile, self.displacement_objects, con_displacement)
        self.write_constraints_data(inpfile, self.sectionprint_objects, con_sectionprint)
        self.write_constraints_data(inpfile, self.selfweight_objects, con_selfweight)
        self.write_constraints_data(inpfile, self.centrif_objects, con_centrif)
        self.write_constraints_sets(inpfile, self.force_objects, con_force)
        self.write_constraints_sets(inpfile, self.pressure_objects, con_pressure)
        self.write_constraints_data(inpfile, self.temperature_objects, con_temperature)
        self.write_constraints_sets(inpfile, self.heatflux_objects, con_heatflux)
        con_fluidsection.write_constraints_fluidsection(inpfile, self)

        # output and step end
        write_step_output.write_step_output(inpfile, self)
        write_step_equation.write_step_end(inpfile, self)

        # footer
        write_footer.write_footer(inpfile, self)
        inpfile.close()

    # ********************************************************************************************
    # write constraint node sets, constraint face sets, constraint element sets
    def write_constraints_sets(
        self,
        f,
        femobjs,
        con_module
    ):
        if not femobjs:
            return

        analysis_types = con_module.get_analysis_types()
        if analysis_types != "all" and self.analysis_type not in analysis_types:
            return

        def constraint_sets_loop_writing(the_file, femobjs, write_before, write_after):
            if write_before != "":
                f.write(write_before)
            for femobj in femobjs:
                # femobj --> dict, FreeCAD document object is femobj["Object"]
                the_obj = femobj["Object"]
                f.write("** {}\n".format(the_obj.Label))
                con_module.write_meshdata_constraint(the_file, femobj, the_obj, self)
            if write_after != "":
                f.write(write_after)

        write_before = con_module.get_before_write_meshdata_constraint()
        write_after = con_module.get_after_write_meshdata_constraint()

        # write sets to file
        write_name = con_module.get_sets_name()
        f.write("\n{}\n".format(59 * "*"))
        f.write("** {}\n".format(write_name.replace("_", " ")))

        if self.split_inpfile is True:
            file_name_split = "{}_{}.inp".format(self.mesh_name, write_name)
            f.write("** {}\n".format(write_name.replace("_", " ")))
            f.write("*INCLUDE,INPUT={}\n".format(file_name_split))
            inpfile_split = open(join(self.dir_name, file_name_split), "w")
            constraint_sets_loop_writing(inpfile_split, femobjs, write_before, write_after)
            inpfile_split.close()
        else:
            constraint_sets_loop_writing(f, femobjs, write_before, write_after)

    # ********************************************************************************************
    # write constraint data
    def write_constraints_data(
        self,
        f,
        femobjs,
        con_module
    ):

        if not femobjs:
            return

        analysis_types = con_module.get_analysis_types()
        if analysis_types != "all" and self.analysis_type not in analysis_types:
            return

        write_before = con_module.get_before_write_constraint()
        write_after = con_module.get_after_write_constraint()

        # write constraint to file
        f.write("\n{}\n".format(59 * "*"))
        f.write("** {}\n".format(con_module.get_constraint_title()))
        if write_before != "":
            f.write(write_before)
        for femobj in femobjs:
            # femobj --> dict, FreeCAD document object is femobj["Object"]
            the_obj = femobj["Object"]
            f.write("** {}\n".format(the_obj.Label))
            con_module.write_constraint(f, femobj, the_obj, self)
        if write_after != "":
            f.write(write_after)

##  @}
