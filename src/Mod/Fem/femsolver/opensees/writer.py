# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "OpenSees Writer"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import FreeCAD
import time
from .. import writerbase as FemInputWriter
from femmesh import meshtools as FemMeshTools
from feminout import importOpenSeesMesh

from .heading import Heading
from .nodes import Nodes
# from elements import Elements
# from sets import Sets
# from bcs import BCs
# from materials import Materials
# from steps import Steps


__all__ = [
    'FemInputWriterOpenSees',
]


comments = {
    'abaqus': '**',
    'opensees': '#',
    'sofistik': '$',
    'ansys': '!',
}


class FemInputWriterOpenSees(FemInputWriter.FemInputWriter,
                             # Steps, Materials, BCs, Sets, Elements,
                             Nodes,
                             Heading):

    """ Initialises base file writer.

    Parameters
    ----------
    None

    Returns
    -------
    None

    """

    def __init__(self,
                 analysis_obj,
                 solver_obj,
                 mesh_obj,
                 member,
                 dir_name=None,
                 structure=None,
                 software='opensees',
                 filename='/home/ebi/freecad_opensees.tcl',
                 fields=None,
                 ndof=6,
                 ):
        FemInputWriter.FemInputWriter.__init__(self,
                                               analysis_obj,
                                               solver_obj,
                                               mesh_obj,
                                               member,
                                               dir_name,
                                               )

        self.comment = comments[software]
        self.filename = filename
        self.file_name = filename
        self.member = member
        self.ndof = ndof
        self.software = software
        self.structure = structure
        self.fields = fields
        self.spacer = {'abaqus': ', ', 'opensees': ' ', 'ansys': ' '}
        if not self.femnodes_mesh:
            self.femnodes_mesh = self.femmesh.Nodes
        if not self.femelement_table:
            self.femelement_table = FemMeshTools.get_femelement_table(self.femmesh)
            self.element_count = len(self.femelement_table)

        print(f'self.femelement_table = {self.femelement_table}')
        print(f'self.element_count = {self.element_count}')

    def write_opensees_input_file(self):

        input_generate(self.analysis,
                       self.solver_obj,
                       self.mesh_object,
                       self.member,
                       self.structure,
                       self.fields,
                       self.ndof,
                       self.filename,
                       )

        # timestart = time.clock()

        # inpfile = open(self.file_name, "w")

        # inpfile.write(example_input_file)
        # inpfile.close()
        # writing_time_string = (
        #     "Writing time input file: {} seconds"
        #     .format(round((time.clock() - timestart), 2))
        # )
        # FreeCAD.Console.PrintMessage(writing_time_string + " \n\n")
        return self.file_name

    def __enter__(self):

        self.file = open(self.filename, 'w')
        return self

    def __exit__(self, type, value, traceback):

        self.file.close()

    def blank_line(self):

        self.file.write('{0}\n'.format(self.comment))

    def divider_line(self):

        self.file.write('{0}------------------------------------------------------------------\n'.format(self.comment))

    def write_line(self, line):

        self.file.write('{0}\n'.format(line))

    def write_section(self, section):

        self.divider_line()
        self.write_line('{0} {1}'.format(self.comment, section))
        self.divider_line()

    def write_subsection(self, subsection):

        self.write_line('{0} {1}'.format(self.comment, subsection))
        self.write_line('{0}-{1}'.format(self.comment, '-' * len(subsection)))
        self.blank_line()


def input_generate(analysis_obj,
                   solver_obj,
                   mesh_obj,
                   member,
                   structure,
                   fields,
                   ndof,
                   filename,
                   ):
    """ Creates the OpenSees .tcl file from the Structure object.

    Parameters
    ----------
    structure : obj
        The Structure object to read from.
    fields : list
        Data field requests.
    output : bool
        Print terminal output.
    ndof : int
        Number of degrees-of-freedom in the model, 3 or 6.

    Returns
    -------
    None

    """

    # filename = '{0}{1}.tcl'.format(structure.path, structure.name)

    with FemInputWriterOpenSees(analysis_obj,
                                solver_obj,
                                mesh_obj,
                                member,
                                structure=structure,
                                software='opensees',
                                filename=filename,
                                fields=fields,
                                ndof=ndof) as writer:

        writer.write_heading()
        writer.write_nodes()
        # writer.write_boundary_conditions()
        # writer.write_materials()
        # writer.write_elements()
        # writer.write_steps()

    print('***** OpenSees input file generated: {0} *****\n'.format(filename))
