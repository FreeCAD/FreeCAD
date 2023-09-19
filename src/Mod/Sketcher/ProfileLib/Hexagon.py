# ***************************************************************************
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

import FreeCAD, FreeCADGui, Sketcher, Part


__title__ = "Hexagon profile lib"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecad.org"

App = FreeCAD
Gui = FreeCADGui


def makeHexagonSimple(sketchName=None):
    if not sketchName:
        sketch = App.ActiveDocument.addObject("Sketcher::SketchObject", "Hexagon")
    else:
        sketch = App.ActiveDocument.getObject(sketchName)

    geoList = []
    geoList.append(Part.LineSegment(App.Vector(-20.00, 34.64, 0), App.Vector(20.00, 34.64, 0)))
    geoList.append(Part.LineSegment(App.Vector(20.00, 34.64, 0), App.Vector(47.082363, 0.00, 0)))
    geoList.append(Part.LineSegment(App.Vector(40.00, 0.00, 0), App.Vector(20.00, -34.64, 0)))
    geoList.append(Part.LineSegment(App.Vector(20.00, -34.64, 0), App.Vector(-20.00, -34.64, 0)))
    geoList.append(Part.LineSegment(App.Vector(-20.00, -34.64, 0), App.Vector(-40.00, 0.00, 0)))
    geoList.append(Part.LineSegment(App.Vector(-40.00, 0.00, 0), App.Vector(-20.00, 34.64, 0)))
    (l1, l2, l3, l4, l5, l6) = sketch.addGeometry(geoList)

    conList = []
    conList.append(Sketcher.Constraint("Coincident", l1, 2, l2, 1))
    conList.append(Sketcher.Constraint("Coincident", l2, 2, l3, 1))
    conList.append(Sketcher.Constraint("Coincident", l3, 2, l4, 1))
    conList.append(Sketcher.Constraint("Coincident", l4, 2, l5, 1))
    conList.append(Sketcher.Constraint("Coincident", l5, 2, l6, 1))
    conList.append(Sketcher.Constraint("Coincident", l6, 2, l1, 1))
    conList.append(Sketcher.Constraint("Equal", l1, l2))
    conList.append(Sketcher.Constraint("Equal", l2, l3))
    conList.append(Sketcher.Constraint("Equal", l3, l4))
    conList.append(Sketcher.Constraint("Equal", l4, l5))
    conList.append(Sketcher.Constraint("Equal", l5, l6))
    conList.append(Sketcher.Constraint("Angle", l1, 2, l2, 1, App.Units.Quantity("120.000000 deg")))
    conList.append(Sketcher.Constraint("Angle", l3, 2, l4, 1, App.Units.Quantity("120.000000 deg")))
    conList.append(Sketcher.Constraint("Angle", l5, 2, l6, 1, App.Units.Quantity("120.000000 deg")))
    sketch.addConstraint(conList)
    return
