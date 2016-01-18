#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *
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


def solve(ship, weights, tanks, rolls, var_trim=True):
    """ Compute the ship GZ curve.
    @param ship Ship instance.
    @param weights Considered weights.
    @param tanks Considered tanks.
    @param rolls List of considered roll angles.
    @param var_trim True if the trim angle should be recomputed at each roll
    angle, False otherwise.
    @return GZ values for each roll angle
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
    for t in tanks:
        # t[0] = tank object
        # t[1] = load density
        # t[2] = filling level
        vol = t[0].Proxy.setFillingLevel(t[0], t[2]).getValueAs('m^3').Value
        TW += vol * t[1]
    TW = TW.getValueAs('kg').Value * G

    gzs = []
    for roll in rolls:
        gz = solve_point(W, COG, TW, ship, tanks, roll, var_trim)
        if gz is None:
            return []
        gzs.append(solve_point(W, COG, TW, ship, tanks, roll, var_trim))

    return gzs

def solve_point(W, COG, TW, ship, tanks, roll, var_trim=True):
    """ Compute the ship GZ value.
    @param W Empty ship weight.
    @param COG Empty ship Center of mass.
    @param tanks Considered tanks.
    @param roll Roll angle.
    @param var_trim True if the trim angle should be recomputed at each roll
    angle, False otherwise.
    @return GZ value
    """
    gz = 0.0
    
    # Look for the equilibrium draft (and eventually the trim angle too)
    max_draft = ship.Shape.BoundBox.ZMax
    draft = max_draft
    max_disp = ship.Shape.Volume.getValueAs('m^3').Value * DENS * 1000.0 * G
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
        # Get the empty ship weight transformed application point
        p = Part.makePoint(COG)
        p.translate(Vector(0.0, 0.0, -draft))
        m = Matrix()
        m.rotateX(math.radians(roll))
        m.rotateY(-math.radians(trim))
        p.rotate(Placement(m))
        # Add the tanks
        # TODO
        # ---

        # Compute the errors
        draft_error = abs(disp - W - TW) / max_disp
        if not var_trim:
            trim_error = 0.0
        else:
            dx = B.x - p.X 
            dz = B.z - p.Z
            if abs(dx) < 0.001 * ship.Length.getValueAs('m').Value:
                trim_error = 0.0
            else:
                trim_error = math.degrees(math.atan2(dz, dx))

        # Check if we can tolerate the errors
        if draft_error < 0.01 and trim_error < 1.0:
            break

        # Get the new draft and trim
        draft += draft_error * max_draft
        trim += 0.5 * trim_error

    return B.y - p.Y