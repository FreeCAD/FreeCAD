# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Mystran Writer"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecad.org"

## \addtogroup FEM
#  @{

import time
from os.path import join

import FreeCAD

# we need to import FreeCAD before the non FreeCAD library because of the print
try:
    from pyNastran.bdf.bdf import BDF
except Exception:
    FreeCAD.Console.PrintError(
        "Module pyNastran not found. Writing Mystran solver input will not be work.\n"
    )

from . import add_mesh
from . import add_femelement_material
from . import add_femelement_geometry
from . import add_con_force
from . import add_con_fixed
from . import add_solver_control
from .. import writerbase


class FemInputWriterMystran(writerbase.FemInputWriter):
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
        # basename (only for implementation purpose later delete this code
        # the mesh should never be None for Calculix solver
        # working dir and input file
        if self.mesh_object is not None:
            self.basename = self.mesh_object.Name
        else:
            self.basename = "Mesh"
        self.solverinput_file = join(self.dir_name, self.basename + ".bdf")
        self.pynasinput_file = join(self.dir_name, self.basename + ".py")
        FreeCAD.Console.PrintLog(
            "FemInputWriterMystran --> self.dir_name  -->  {}\n"
            .format(self.dir_name)
        )
        FreeCAD.Console.PrintMessage(
            "FemInputWriterMystra --> self.solverinput_file  -->  {}\n"
            .format(self.solverinput_file)
        )
        FreeCAD.Console.PrintMessage(
            "FemInputWriterMystra --> self.pynasf_name  -->  {}\n"
            .format(self.pynasinput_file)
        )

    def write_solver_input(self):

        timestart = time.process_time()

        model = BDF()

        pynasf = open(self.pynasinput_file, "w")

        # comment and model init
        pynasf.write("# written by FreeCAD\n\n")
        pynasf.write("from pyNastran.bdf.bdf import BDF\n")
        pynasf.write("model = BDF()\n\n")

        model = add_mesh.add_mesh(pynasf, model, self)
        model = add_femelement_material.add_femelement_material(pynasf, model, self)
        model = add_femelement_geometry.add_femelement_geometry(pynasf, model, self)
        model = add_con_force.add_con_force(pynasf, model, self)
        model = add_con_fixed.add_con_fixed(pynasf, model, self)
        model = add_solver_control.add_solver_control(pynasf, model, self)

        pynasf.write(
            "\n\nmodel.write_bdf('{}', enddata=True)\n"
            .format(join(self.dir_name, self.basename + "_pyNas.bdf"))
        )

        pynasf.close()

        # print(model.get_bdf_stats())
        model.write_bdf(self.solverinput_file, enddata=True)

        writing_time_string = (
            "Writing time input file: {} seconds"
            .format(round((time.process_time() - timestart), 2))
        )
        FreeCAD.Console.PrintMessage(writing_time_string + " \n\n")

        return self.solverinput_file


##  @}
