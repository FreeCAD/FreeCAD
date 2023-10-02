# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

__title__ = "PartEnums module"
__author__ = "Werner Mayer"
__url__ = "http://www.freecad.org"
__doc__ = "Enum types"

from enum import IntEnum

class JoinType(IntEnum):
    Arc = 0
    Tangent = 1
    Intersection = 2

class Shape(IntEnum):
    C0 = 0
    G1 = 1
    C1 = 2
    G2 = 3
    C2 = 4
    C3 = 5
    CN = 6

class FillingStyle(IntEnum):
    StretchStyle = 0
    CoonsStyle = 1
    CurvedStyle = 2

class Orientation(IntEnum):
    FORWARD = 0
    REVERSED = 1
    INTERNAL = 2
    EXTERNAL = 3

class ShapeEnum(IntEnum):
    COMPOUND = 0
    COMPSOLID = 1
    SOLID = 2
    SHELL= 3
    FACE = 4
    WIRE = 5
    EDGE = 6
    VERTEX = 7
    SHAPE = 8

class HLRBRep_TypeOfResultingEdge(IntEnum):
    Undefined = 0
    IsoLine = 1
    OutLine = 2
    Rg1Line = 3
    RgNLine = 4
    Sharp = 5

