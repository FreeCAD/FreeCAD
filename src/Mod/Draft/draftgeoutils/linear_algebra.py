# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides various functions for linear algebra.

This includes calculating linear equation parameters, and matrix determinants.
"""
## @package linear_algebra
# \ingroup draftgeoutils
# \brief Provides various functions for linear algebra.

## \addtogroup draftgeoutils
# @{
import FreeCAD as App


def linearFromPoints(p1, p2):
    """Calculate linear equation from points.

    Calculate the slope and offset parameters of the linear equation
    of a line defined by two points.

    Linear equation:
    y = m * x + b
    m = dy / dx
    m ... Slope
    b ... Offset (point where the line intersects the y axis)
    dx/dy ... Delta x and y. Using both as a vector results
    in a non-offset direction vector.
    """
    if not isinstance(p1, App.Vector) and not isinstance(p2, App.Vector):
        return None

    line = {}
    line['dx'] = (p2.x - p1.x)
    line['dy'] = (p2.y - p1.y)
    line['slope'] = line['dy'] / line['dx']
    line['offset'] = p1.y - line['slope'] * p1.x
    return line


def determinant(mat, n):
    """Return the determinant of an N-matrix.

    It recursively expands the minors.
    """
    matTemp = [[0.0, 0.0, 0.0], [0.0, 0.0, 0.0], [0.0, 0.0, 0.0]]
    if (n > 1):
        if n == 2:
            d = mat[0][0] * mat[1][1] - mat[1][0] * mat[0][1]
        else:
            d = 0.0
            for j1 in range(n):
                # Create minor
                for i in range(1, n):
                    j2 = 0
                    for j in range(n):
                        if j == j1:
                            continue
                        matTemp[i-1][j2] = mat[i][j]
                        j2 += 1
                d += ((-1.0)**(1.0 + j1 + 1.0)
                      * mat[0][j1] * determinant(matTemp, n-1))
        return d
    else:
        return 0

## @}
