# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

__title__ = "BasicShapes.Utils"
__author__ = "Werner Mayer"
__url__ = "http://www.freecad.org"
__doc__ = "Utilities for shapes"


import Part

def makeCompoundFromPoints(geometry, distance = 1.0):
    """Get sampled points from geometry and create a compound."""
    try:
        points = geometry.getPropertyOfGeometry().getPoints(distance)[0]
        return Part.makeCompound([Part.Point(m).toShape() for m in points])
    except AttributeError:
        return None

def showCompoundFromPoints(geometry, distance = 1.0):
    """Create a compound from geometry and show it."""
    try:
        compound = makeCompoundFromPoints(geometry, distance)
        return Part.show(compound, geometry.Label + "_pts")
    except AttributeError:
        return None
