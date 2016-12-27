#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2016                                              *
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

import math
import FreeCAD as App
import FreeCADGui as Gui
from FreeCAD import Vector, Matrix, Placement
import Part
import Units
import Instance as ShipInstance
import WeightInstance
import TankInstance
from shipHydrostatics import Tools as Hydrostatics


G = Units.parseQuantity("9.81 m/s^2")
MAX_EQUILIBRIUM_ITERS = 10
DENS = Units.parseQuantity("1025 kg/m^3")
TRIM_RELAX_FACTOR = 10.0


def solve(ship, weights, tanks, rolls, var_trim=True):
    """Compute the ship GZ stability curve

    Position arguments:
    ship -- Ship object
    weights -- List of weights to consider
    tanks -- List of tanks to consider (each one should be a tuple with the
    tank instance, the density of the fluid inside, and the filling level ratio)
    rolls -- List of roll angles

    Keyword arguments:
    var_trim -- True if the equilibrium trim should be computed for each roll
    angle, False if null trim angle can be used instead.

    Returned value:
    List of GZ curve points. Each point contains the GZ stability length, the
    equilibrium draft, and the equilibrium trim angle (0 deg if var_trim is
    False)
    """
    # Get the unloaded weight (ignoring the tanks for the moment).
    W = Units.parseQuantity("0 kg")
    mom_x = Units.parseQuantity("0 kg*m")
    mom_y = Units.parseQuantity("0 kg*m")
    mom_z = Units.parseQuantity("0 kg*m")
    for w in weights:
        W += w.Proxy.getMass(w)
        m = w.Proxy.getMoment(w)
        mom_x += m[0]
        mom_y += m[1]
        mom_z += m[2]
    COG = Vector(mom_x / W, mom_y / W, mom_z / W)
    W = W * G

    # Get the tanks weight
    TW = Units.parseQuantity("0 kg")
    VOLS = []
    for t in tanks:
        # t[0] = tank object
        # t[1] = load density
        # t[2] = filling level
        vol = t[0].Proxy.getVolume(t[0], t[2])
        VOLS.append(vol)
        TW += vol * t[1]
    TW = TW * G

    points = []
    for i,roll in enumerate(rolls):
        App.Console.PrintMessage("{0} / {1}\n".format(i + 1, len(rolls)))
        point = solve_point(W, COG, TW, VOLS,
                            ship, tanks, roll, var_trim)
        if point is None:
            return []
        points.append(point)

    return points


def solve_point(W, COG, TW, VOLS, ship, tanks, roll, var_trim=True):
    """ Compute the ship GZ value.
    @param W Empty ship weight.
    @param COG Empty ship Center of mass.
    @param TW Tanks weights.
    @param VOLS List of tank volumes.
    @param tanks Considered tanks.
    @param roll Roll angle.
    @param var_trim True if the trim angle should be recomputed at each roll
    angle, False otherwise.
    @return GZ value, equilibrium draft, and equilibrium trim angle (0 if
    variable trim has not been requested)
    """    
    # Look for the equilibrium draft (and eventually the trim angle too)
    max_draft = Units.Quantity(ship.Shape.BoundBox.ZMax, Units.Length)
    draft = ship.Draft
    max_disp = Units.Quantity(ship.Shape.Volume, Units.Volume) * DENS * G
    if max_disp < W + TW:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "Too much weight! The ship will never displace water enough",
            None)
        App.Console.PrintError(msg + ' ({} vs. {})\n'.format(
            (max_disp / G).UserString, ((W + TW) / G).UserString))
        return None

    trim = Units.parseQuantity("0 deg")
    for i in range(MAX_EQUILIBRIUM_ITERS):
        # Get the displacement, and the bouyance application point
        disp, B, _ = Hydrostatics.displacement(ship,
                                               draft,
                                               roll,
                                               trim)
        disp *= G

        # Add the tanks effect on the center of gravity
        mom_x = Units.Quantity(COG.x, Units.Length) * W
        mom_y = Units.Quantity(COG.y, Units.Length) * W
        mom_z = Units.Quantity(COG.z, Units.Length) * W
        for i,t in enumerate(tanks):
            tank_weight = VOLS[i] * t[1] * G
            tank_cog = t[0].Proxy.getCoG(t[0], VOLS[i], roll, trim)
            mom_x += Units.Quantity(tank_cog.x, Units.Length) * tank_weight
            mom_y += Units.Quantity(tank_cog.y, Units.Length) * tank_weight
            mom_z += Units.Quantity(tank_cog.z, Units.Length) * tank_weight
        cog_x = mom_x / (W + TW)
        cog_y = mom_y / (W + TW)
        cog_z = mom_z / (W + TW)
        # Compute the errors
        draft_error = -((disp - W - TW) / max_disp).Value
        R_x = cog_x - Units.Quantity(B.x, Units.Length)
        R_y = cog_y - Units.Quantity(B.y, Units.Length)
        R_z = cog_z - Units.Quantity(B.z, Units.Length)
        if not var_trim:
            trim_error = 0.0
        else:
            trim_error = -TRIM_RELAX_FACTOR * R_x / ship.Length

        # Check if we can tolerate the errors
        if abs(draft_error) < 0.01 and abs(trim_error) < 0.1:
            break

        # Get the new draft and trim
        draft += draft_error * max_draft
        trim += trim_error * Units.Degree

    # GZ should be provided in the Free surface oriented frame of reference
    c = math.cos(roll.getValueAs('rad'))
    s = math.sin(roll.getValueAs('rad'))
    return c * R_y - s * R_z, draft, trim


def gz(lc, rolls, var_trim=True):
    """Compute the ship GZ stability curve

    Position arguments:
    lc -- Load condition spreadsheet
    rolls -- List of roll angles to compute

    Keyword arguments:
    var_trim -- True if the equilibrium trim should be computed for each roll
    angle, False if null trim angle can be used instead.

    Returned value:
    List of GZ curve points. Each point contains the GZ stability length, the
    equilibrium draft, and the equilibrium trim angle (0 deg if var_trim is
    False)
    """
    # B1 cell must be a ship
    # B2 cell must be the loading condition itself
    doc = lc.Document
    try:
        if lc not in doc.getObjectsByLabel(lc.get('B2')):
            return[]
        ships = doc.getObjectsByLabel(lc.get('B1'))
        if len(ships) != 1:
            if len(ships) == 0:
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "Wrong Ship label! (no instances labeled as"
                    "'{}' found)",
                    None)
                App.Console.PrintError(msg + '\n'.format(
                    lc.get('B1')))
            else:
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "Ambiguous Ship label! ({} instances labeled as"
                    "'{}' found)",
                    None)
                App.Console.PrintError(msg + '\n'.format(
                    len(ships),
                    lc.get('B1')))
            return[]
        ship = ships[0]
        if ship is None or not ship.PropertiesList.index("IsShip"):
            return[]
    except ValueError:
        return[]
    # Extract the weights and the tanks
    weights = []
    index = 6
    while True:
        try:
            ws = doc.getObjectsByLabel(lc.get('A{}'.format(index)))
        except ValueError:
            break
        index += 1
        if len(ws) != 1:
            if len(ws) == 0:
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "Wrong Weight label! (no instances labeled as"
                    "'{}' found)",
                    None)
                App.Console.PrintError(msg + '\n'.format(
                    lc.get('A{}'.format(index - 1))))
            else:
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "Ambiguous Weight label! ({} instances labeled as"
                    "'{}' found)",
                    None)
                App.Console.PrintError(msg + '\n'.format(
                    len(ws),
                    lc.get('A{}'.format(index - 1))))
            continue
        w = ws[0]
        try:
            if w is None or not w.PropertiesList.index("IsWeight"):
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "Invalid Weight! (the object labeled as"
                    "'{}' is not a weight)",
                    None)
                App.Console.PrintError(msg + '\n'.format(
                    len(ws),
                    lc.get('A{}'.format(index - 1))))
                continue
        except ValueError:
            continue
        weights.append(w)
    tanks = []
    index = 6
    while True:
        try:
            ts = doc.getObjectsByLabel(lc.get('C{}'.format(index)))
            dens = float(lc.get('D{}'.format(index)))
            level = float(lc.get('E{}'.format(index)))
            dens = Units.parseQuantity("{} kg/m^3".format(dens))
        except ValueError:
            break
        index += 1
        if len(ts) != 1:
            if len(ts) == 0:
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "Wrong Tank label! (no instances labeled as"
                    "'{}' found)",
                    None)
                App.Console.PrintError(msg + '\n'.format(
                    lc.get('C{}'.format(index - 1))))
            else:
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "Ambiguous Tank label! ({} instances labeled as"
                    "'{}' found)",
                    None)
                App.Console.PrintError(msg + '\n'.format(
                    len(ts),
                    lc.get('C{}'.format(index - 1))))
            continue
        t = ts[0]
        try:
            if t is None or not t.PropertiesList.index("IsTank"):
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "Invalid Tank! (the object labeled as"
                    "'{}' is not a tank)",
                    None)
                App.Console.PrintError(msg + '\n'.format(
                    len(ws),
                    lc.get('C{}'.format(index - 1))))
                continue
        except ValueError:
            continue
        tanks.append((t, dens, level))

    return solve(ship, weights, tanks, rolls, var_trim)
