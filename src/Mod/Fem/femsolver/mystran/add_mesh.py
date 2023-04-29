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

__title__ = "Mystran add fem mesh"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecad.org"

## \addtogroup FEM
#  @{

from feminout import exportNastranMesh
from femmesh import meshtools


def add_mesh(f, model, mystran_writer):

    # needed basic data
    if not mystran_writer.femnodes_mesh:
        mystran_writer.femnodes_mesh = mystran_writer.femmesh.Nodes
    if not mystran_writer.femelement_table:
        mystran_writer.femelement_table = meshtools.get_femelement_table(
            mystran_writer.femmesh
        )
    mesh_eletype = exportNastranMesh.get_export_element_type(
        mystran_writer.femmesh,
        mystran_writer.femelement_table
    )

    # get the pynas code
    mesh_pynas_code = exportNastranMesh.get_pynastran_mesh(
        mystran_writer.femnodes_mesh,
        mystran_writer.femelement_table,
        mesh_eletype
    )
    # print(mesh_pynas_code)

    # write the pyNastran code
    f.write(mesh_pynas_code)

    # execute pyNastran code to add grid to the model
    # print(model.get_bdf_stats())
    exec(mesh_pynas_code)
    # print(model.get_bdf_stats())

    return model


##  @}
