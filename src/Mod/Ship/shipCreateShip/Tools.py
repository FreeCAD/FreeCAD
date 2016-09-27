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
import Instance


def createShip(solids, L, B, T):
    """Create a new ship instance

    Position arguments:
    solids -- List of hull solid shapes
    L -- Ship length between perpendiculars
    B -- Ship Breadth
    T -- Ship design draft

    Returned value:
    The new ship object

    The solids can be easily extracted from an already existing object. For
    instance, to get the solids from the selected object simply type the
    following command:

    solids = Gui.ActiveDocument.ActiveObject.Object.Shape.Solids

    Regarding the Lenght, Breadth, and Draft, it is strongly recommended to use
    Units.parseQuantity method, e.g. The following obfuscated code snippet build
    such variables:

    import Units
    L = Units.parseQuantity("25.5 m")
    B = Units.parseQuantity("3.9 m")
    T = Units.parseQuantity("1.0 m")
    """
    obj = App.ActiveDocument.addObject("Part::FeaturePython", "Ship")
    ship = Instance.Ship(obj, solids)
    Instance.ViewProviderShip(obj.ViewObject)

    obj.Length = L
    obj.Breadth = B
    obj.Draft = T

    App.ActiveDocument.recompute()
    return obj