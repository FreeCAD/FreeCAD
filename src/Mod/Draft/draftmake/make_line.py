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
"""Provides functions to create two-point Wire objects."""
## @package make_line
# \ingroup draftmake
# \brief Provides functions to create two-point Wire objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftmake.make_wire as make_wire


def make_line(first_param, last_param=None):
    """makeLine(first_param, p2)

    Creates a line from 2 points or from a given object.

    Parameters
    ----------
    first_param :
        Base.Vector -> First point of the line (if p2 is None)
        Part.LineSegment -> Line is created from the given Linesegment
        Shape -> Line is created from the give Shape

    last_param : Base.Vector
        Second point of the line, if not set the function evaluates
        the first_param to look for a Part.LineSegment or a Shape
    """
    if last_param:
        p1 = first_param
        p2 = last_param
    else:
        if hasattr(first_param, "StartPoint") and hasattr(first_param, "EndPoint"):
            p2 = first_param.EndPoint
            p1 = first_param.StartPoint
        elif hasattr(p1,"Vertexes"):
            p2 = first_param.Vertexes[-1].Point
            p1 = first_param.Vertexes[0].Point
        else:
            _err = "Unable to create a line from the given parameters"
            App.Console.PrintError(_err + "\n")
            return

    obj = make_wire.make_wire([p1,p2])

    return obj


makeLine = make_line

## @}
