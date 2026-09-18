# ***************************************************************************
# *   Copyright (c) 2024 Tim Swait <timswait@gmail.com>              *
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

__title__ = "Code Aster add fem mesh"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{


def add_mesh(commtxt, ca_writer):
    ca_writer.tools.get_tmp_file_paths()
    ca_writer.tools.temp_file_mesh = ca_writer.IPmesh_file
    ca_writer.tools.temp_file_geo = ca_writer.geo_file
    ca_writer.tools.update_mesh_data()

    commtxt += "mesh = LIRE_MAILLAGE(UNITE=20)\n\n"

    return commtxt


##  @}
