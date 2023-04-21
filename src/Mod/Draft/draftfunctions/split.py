# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""Provides functions to split wires into separate wire objects."""
## @package split
# \ingroup draftfunctions
# \brief Provides functions to split wires into separate wire objects.

## \addtogroup draftfunctions
# @{
import draftutils.utils as utils
import draftmake.make_wire as make_wire


def split(wire, newPoint, edgeIndex):
    if utils.get_type(wire) != "Wire":
        return
    elif wire.Closed:
        split_closed_wire(wire, edgeIndex)
    else:
        split_open_wire(wire, newPoint, edgeIndex)


def split_closed_wire(wire, edgeIndex):
    wire.Closed = False
    if edgeIndex == len(wire.Points):
        make_wire.make_wire([wire.Placement.multVec(wire.Points[0]),
            wire.Placement.multVec(wire.Points[-1])], placement=wire.Placement)
    else:
        make_wire.make_wire([wire.Placement.multVec(wire.Points[edgeIndex-1]),
            wire.Placement.multVec(wire.Points[edgeIndex])], placement=wire.Placement)
        wire.Points = list(reversed(wire.Points[0:edgeIndex])) + list(reversed(wire.Points[edgeIndex:]))


splitClosedWire = split_closed_wire


def split_open_wire(wire, newPoint, edgeIndex):
    wire1Points = []
    wire2Points = []
    for index, point in enumerate(wire.Points):
        if index == edgeIndex:
            wire1Points.append(wire.Placement.inverse().multVec(newPoint))
            wire2Points.append(newPoint)
            wire2Points.append(wire.Placement.multVec(point))
        elif index < edgeIndex:
            wire1Points.append(point)
        elif index > edgeIndex:
            wire2Points.append(wire.Placement.multVec(point))
    wire.Points = wire1Points
    make_wire.make_wire(wire2Points, placement=wire.Placement)


splitOpenWire = split_open_wire

## @}
