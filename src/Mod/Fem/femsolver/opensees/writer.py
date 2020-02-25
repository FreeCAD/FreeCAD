# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "OpenSees Writer"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD
import time
from .. import writerbase as FemInputWriter


class FemInputWriterOpenSees(FemInputWriter.FemInputWriter):
    def __init__(
        self,
        analysis_obj,
        solver_obj,
        mesh_obj,
        member,
        dir_name=None
    ):
        FemInputWriter.FemInputWriter.__init__(
            self,
            analysis_obj,
            solver_obj,
            mesh_obj,
            member,
            dir_name
        )
        # working dir and input file
        from os.path import join
        # self.main_file_name = self.mesh_object.Name + ".in"
        self.main_file_name = "2DPlaneStress.in"
        self.file_name = join(self.dir_name, self.main_file_name)
        FreeCAD.Console.PrintLog(
            "FemInputWriterCcx --> self.dir_name  -->  " + self.dir_name + "\n"
        )
        FreeCAD.Console.PrintLog(
            "FemInputWriterCcx --> self.main_file_name  -->  " + self.main_file_name + "\n"
        )
        FreeCAD.Console.PrintMessage(
            "FemInputWriterCcx --> self.file_name  -->  " + self.file_name + "\n"
        )

    def write_opensees_input_file(self):

        timestart = time.clock()

        inpfile = open(self.file_name, "w")

        inpfile.write(example_input_file)
        inpfile.close()
        writing_time_string = (
            "Writing time input file: {} seconds"
            .format(round((time.clock() - timestart), 2))
        )
        FreeCAD.Console.PrintMessage(writing_time_string + " \n\n")
        return self.file_name


example_input_file = """2DPlaneStress.out
Patch test of PlaneStress2d elements -> pure compression
LinearStatic nsteps 1 nmodules 1
vtkxml tstep_all domain_all  primvars 1 1 vars 5 1 2 4 5 27 stype 2
domain 2dPlaneStress
OutputManager tstep_all dofman_all element_all
ndofman 8 nelem 5 ncrosssect 1 nmat 1 nbc 3 nic 0 nltf 1 nset 3
node 1 coords 3  0.0   0.0   0.0
node 2 coords 3  0.0   4.0   0.0
node 3 coords 3  2.0   2.0   0.0
node 4 coords 3  3.0   1.0   0.0
node 5 coords 3  8.0   0.8   0.0
node 6 coords 3  7.0   3.0   0.0
node 7 coords 3  9.0   0.0   0.0
node 8 coords 3  9.0   4.0   0.0
PlaneStress2d 1 nodes 4 1 4 3 2  NIP 1
PlaneStress2d 2 nodes 4 1 7 5 4  NIP 1
PlaneStress2d 3 nodes 4 4 5 6 3  NIP 1
PlaneStress2d 4 nodes 4 3 6 8 2  NIP 1
PlaneStress2d 5 nodes 4 5 7 8 6  NIP 1
Set 1 elementranges {(1 5)}
Set 2 nodes 2 1 2
Set 3 nodes 2 7 8
SimpleCS 1 thick 1.0 width 1.0 material 1 set 1
IsoLE 1 d 0. E 15.0 n 0.25 talpha 1.0
BoundaryCondition 1 loadTimeFunction 1 dofs 2 1 2 values 1 0.0 set 2
BoundaryCondition 2 loadTimeFunction 1 dofs 1 2 values 1 0.0 set 3
NodalLoad 3 loadTimeFunction 1 dofs 2 1 2 components 2 2.5 0.0 set 3
ConstantFunction 1 f(t) 1.0
"""

##  @}
