# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Johannes Hartung <j.hartung@gmx.net>             *
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
from __future__ import print_function

__title__ = "FreeCAD Fenics XDMF mesh reader"
__author__ = "Johannes Hartung"
__url__ = "http://www.freecadweb.org"

## @package importFenicsXDMF
#  \ingroup FEM
#  \brief FreeCAD Fenics Mesh XDMF reader for FEM workbench


def read_fenics_mesh_xdmf(xdmffilename):

    print("Not operational, yet")

    return {'Nodes': {},
            'Hexa8Elem': {}, 'Penta6Elem': {}, 'Tetra4Elem': {}, 'Tetra10Elem': {},
            'Penta15Elem': {}, 'Hexa20Elem': {}, 'Tria3Elem': {}, 'Tria6Elem': {},
            'Quad4Elem': {}, 'Quad8Elem': {}, 'Seg2Elem': {}
            }
