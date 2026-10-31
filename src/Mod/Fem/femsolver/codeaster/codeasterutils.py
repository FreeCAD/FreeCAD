# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Tim Swait <t.swait@sheffield.ac.uk                 *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "Utilies related the Code Aster solver and laminates"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"


class Layup:
    """
    Object to contain layup information, comprising:
    name: the name of the layup, used by DEFI_COMPOSITE
    groups: a list of groups this layup is applied to
    parentgroup: if the group it's applied to is part of a larger group
    matnames: list of the names of materials used
    thicknesses: list of the ply thicknesses
    orientations: list of the ply orientations
    """

    def __init__(
        self,
        name,
        groups=[],
        parentgroup=None,
        ninit=0,
        nfin=0,
        matnames=[],
        thicknesses=[],
        orientations=[],
    ):
        self.name = name
        self.groups = groups
        self.parentgroup = parentgroup
        self.ninit = ninit
        self.nfin = nfin
        self.matnames = matnames
        self.thicknesses = thicknesses
        self.orientations = orientations
