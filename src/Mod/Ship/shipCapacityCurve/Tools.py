#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016                                                    *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************


import FreeCAD as App
import Units
import WeightInstance as Instance
import shipUtils.Units as USys
from PySide import QtGui


def tankCapacityCurve(tank, n):
    """Create a tank capacity curve

    Position arguments:
    tank -- Tank object (see createTank)
    ship -- n Number of filling levels to test

    Returned value:
    List of computed points. Each point contains the filling level percentage
    (interval [0, 1]), the filling level (0 for the bottom of the tank), and
    the volume.
    """
    bbox = tank.Shape.BoundBox
    dz = Units.Quantity(bbox.ZMax - bbox.ZMin, Units.Length)
    dlevel = 1.0 / (n - 1)
    out = [(0.0, Units.parseQuantity("0 m"), Units.parseQuantity("0 m^3"))]

    msg = QtGui.QApplication.translate(
        "ship_console",
        "Computing capacity curves",
        None)
    App.Console.PrintMessage(msg + '...\n')
    for i in range(1, n):
        App.Console.PrintMessage("\t{} / {}\n".format(i + 1, n))
        level = i * dlevel
        vol = tank.Proxy.getVolume(tank, level)
        out.append((level, level * dz, level * vol))
    return out
