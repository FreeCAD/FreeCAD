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
from FreeCAD import Vector, Matrix, Placement
import Part
import Units
import FreeCAD as App
import FreeCADGui as Gui
import Instance as ShipInstance
import WeightInstance
import TankInstance
from shipHydrostatics import Tools as Hydrostatics


G = 9.81
MAX_EQUILIBRIUM_ITERS = 10
DENS = 1.025  # [tons/m3], salt water
TRIM_RELAX_FACTOR = 10.0


def solve(ship, weights, tanks, rolls, var_trim=True):
    """ Compute the ship GZ curve.
    @param ship Ship instance.
    @param weights Considered weights.
    @param tanks Considered tanks.
    @param rolls List of considered roll angles.
    @param var_trim True if the trim angle should be recomputed at each roll
    angle, False otherwise.
    @return GZ values, drafts and trim angles, for each roll angle (in 3
    separated lists)
    """
    # Get the unloaded weight (ignoring the tanks for the moment).
    W = 0.0
    COG = Vector()
    for w in weights:
        W += w.Proxy.getMass(w).getValueAs('kg').Value
        m = w.Proxy.getMoment(w)
        COG.x += m[0].getValueAs('kg*m').Value
        COG.y += m[1].getValueAs('kg*m').Value
        COG.z += m[2].getValueAs('kg*m').Value
    COG = COG.multiply(1.0 / W)
    W = W * G

    # Get the tanks weight
    TW = 0.0
    VOLS = []
    for t in tanks:
        # t[0] = tank object
        # t[1] = load density
        # t[2] = filling level
        vol = t[0].Proxy.getVolume(t[0], t[2]).getValueAs('m^3').Value
        VOLS.append(vol)
        TW += vol * t[1]
    TW = TW * G

    gzs = []
    drafts = []
    trims = []
    for i,roll in enumerate(rolls):
        App.Console.PrintMessage("{0} / {1}\n".format(i + 1, len(rolls)))
        gz, draft, trim = solve_point(W, COG, TW, VOLS,
                                      ship, tanks, roll, var_trim)
        if gz is None:
            return []
        gzs.append(gz)
        drafts.append(draft)
        trims.append(trim)

    return gzs, drafts, trims

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
    gz = 0.0
    
    # Look for the equilibrium draft (and eventually the trim angle too)
    max_draft = ship.Shape.BoundBox.ZMax / Units.Metre.Value
    draft = ship.Draft.getValueAs('m').Value
    max_disp = ship.Shape.Volume / Units.Metre.Value**3 * DENS * 1000.0 * G
    if max_disp < W + TW:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "Too much weight! The ship will never displace water enough",
            None,
            QtGui.QApplication.UnicodeUTF8)
        App.Console.PrintError(msg + ' ({} tons vs. {} tons)\n'.format(
            max_disp / 1000.0 / G, (W + TW) / 1000.0 / G))
        return None
    trim = 0.0

    for i in range(MAX_EQUILIBRIUM_ITERS):
        # Get the displacement, and the bouyance application point
        disp, B, Cb = Hydrostatics.displacement(ship, draft, roll, trim)
        disp *= 1000.0 * G
        # Add the tanks effect on the center of gravity
        cog = Vector(COG.x * W, COG.y * W, COG.z * W)
        for i,t in enumerate(tanks):
            tank_weight = VOLS[i] * t[1] * G
            cog += t[0].Proxy.getCoG(t[0], VOLS[i], roll, trim).multiply(
                tank_weight / Units.Metre.Value)
        cog = cog.multiply(1.0 / (W + TW))
        # Compute the errors
        draft_error = -(disp - W - TW) / max_disp
        R = cog - B
        if not var_trim:
            trim_error = 0.0
        else:
            trim_error = -TRIM_RELAX_FACTOR * R.x / ship.Length.getValueAs('m').Value

        # Check if we can tolerate the errors
        if abs(draft_error) < 0.01 and abs(trim_error) < 0.05:
            break

        # Get the new draft and trim
        draft += draft_error * max_draft
        trim += trim_error

    # GZ should be provided in the Free surface oriented frame of reference
    c = math.cos(math.radians(roll))
    s = math.sin(math.radians(roll))
    return c * R.y - s * R.z, draft, trim