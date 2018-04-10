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
from FreeCAD import Units
import TankInstance as Instance


def createTank(solids, ship):
    """Create a new tank instance

    Position arguments:
    solids -- List of solid shapes
    ship -- Ship owner

    Returned value:
    The new tank object

    The tool will claim the new tank as a child of the ship object. Please do
    not remove the partner ship object before removing this new tank before.
    """
    obj = App.ActiveDocument.addObject("Part::FeaturePython", "Tank")
    tank = Instance.Tank(obj, solids, ship)
    Instance.ViewProviderTank(obj.ViewObject)

    # Set it as a child of the ship
    tanks = ship.Tanks[:]
    tanks.append(obj.Name)
    ship.Tanks = tanks
    ship.Proxy.cleanWeights(ship)
    ship.Proxy.cleanTanks(ship)
    ship.Proxy.cleanLoadConditions(ship)

    App.ActiveDocument.recompute()
    return obj