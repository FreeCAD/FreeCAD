# ***************************************************************************
# *   Copyright (c) 2014 Johan Kristensen                                   *
# *   Copyright (c) 2014 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

import FreeCAD, FreeCADGui, Sketcher, Part, math


__title__ = "Regular polygon profile lib"
__author__ = "Johan Kristensen"
__url__ = "http://www.freecad.org"

App = FreeCAD
Gui = FreeCADGui


def makeRegularPolygon(
    sketch,
    sides,
    centerPoint=App.Vector(0, 0, 0),
    firstCornerPoint=App.Vector(-20.00, 34.64, 0),
    construction=False,
):

    if not sketch:
        App.Console.PrintError("No sketch specified in 'makeRegularPolygon'")
        return
    if sides < 3:
        App.Console.PrintError("Number of sides must be at least 3 in 'makeRegularPolygon'")
        return

    diffVec = firstCornerPoint - centerPoint
    diffVec.z = 0
    angular_diff = 2 * math.pi / sides
    pointList = []
    for i in range(0, sides):
        cos_v = math.cos(angular_diff * i)
        sin_v = math.sin(angular_diff * i)
        pointList.append(
            centerPoint
            + App.Vector(
                cos_v * diffVec.x - sin_v * diffVec.y, cos_v * diffVec.y + sin_v * diffVec.x, 0
            )
        )

    geoList = []
    for i in range(0, sides - 1):
        geoList.append(Part.LineSegment(pointList[i], pointList[i + 1]))
    geoList.append(Part.LineSegment(pointList[sides - 1], pointList[0]))
    geoList.append(Part.Circle(centerPoint, App.Vector(0, 0, 1), diffVec.Length))
    geoIndices = sketch.addGeometry(geoList, construction)

    sketch.setConstruction(geoIndices[-1], True)

    conList = []
    for i in range(0, sides - 1):
        conList.append(Sketcher.Constraint("Coincident", geoIndices[i], 2, geoIndices[i + 1], 1))
    conList.append(Sketcher.Constraint("Coincident", geoIndices[sides - 1], 2, geoIndices[0], 1))
    for i in range(0, sides - 1):
        conList.append(Sketcher.Constraint("Equal", geoIndices[0], geoIndices[i + 1]))
    for i in range(0, sides):
        conList.append(Sketcher.Constraint("PointOnObject", geoIndices[i], 2, geoIndices[-1]))
    sketch.addConstraint(conList)
    return
