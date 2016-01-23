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


def createWeight(shapes, ship, density):
    """Create a new weight instance

    Position arguments:
    shapes -- List of shapes of the weight
    ship -- Ship owner
    density -- Density of the object. 4 possibilities are considered here:
        * Density as Mass/Volume: then the weight will be considered as a
        volumetric object. Used for blocks or similar.
        * Density as Mass/Area: then the weight will be considered as an area
        element. Used for structural shells.
        * Density as Mass/Length: then the weight will be cosidered as a linear
        element. Used for linear structural reinforcements.
        * Mass: Then a punctual mass will be considered. Used for complex
        weights, like engines or other machines.

    Returned value:
    The new weight object

    It is strongly recommended to pass just shapes matching with the provided
    density, e.g. don't use volumetric shapes with a linear density value (kg/m)

    The tool will claim the new weight as a child of the ship object. Please do
    not remove the partner ship object before removing this new weight before.
    """
    # Create the object
    obj = App.ActiveDocument.addObject("Part::FeaturePython", "Weight")
    weight = Instance.Weight(obj, shapes, ship)
    Instance.ViewProviderWeight(obj.ViewObject)

    # Setup the mass/density value
    m_unit = "kg"
    l_unit = "m"
    m_qty = Units.Quantity(1, Units.Mass)
    l_qty = Units.Quantity(1, Units.Length)
    a_qty = Units.Quantity(1, Units.Area)
    v_qty = Units.Quantity(1, Units.Volume)
    if density.Unit == m_qty.Unit:
        w_unit = m_unit
        obj.Mass = density.getValueAs(w_unit).Value
    elif density.Unit == (m_qty / l_qty).Unit:
        w_unit = m_unit + '/' + l_unit
        obj.LineDens = density.getValueAs(w_unit).Value
    elif density.Unit == (m_qty / a_qty).Unit:
        w_unit = m_unit + '/' + l_unit + '^2'
        obj.AreaDens = density.getValueAs(w_unit).Value
    elif density.Unit == (m_qty / v_qty).Unit:
        w_unit = m_unit + '/' + l_unit + '^3'
        obj.Dens = density.getValueAs(w_unit).Value

    # Set it as a child of the ship
    weights = ship.Weights[:]
    weights.append(obj.Name)
    ship.Weights = weights
    ship.Proxy.cleanWeights(ship)
    ship.Proxy.cleanTanks(ship)
    ship.Proxy.cleanLoadConditions(ship)

    App.ActiveDocument.recompute()

    return obj