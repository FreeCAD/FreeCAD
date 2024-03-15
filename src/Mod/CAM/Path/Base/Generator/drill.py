# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
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


import Path
import numpy

__title__ = "Drilling Toolpath Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Generates the drilling toolpath for a single spotshape"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def generate(
    edge, dwelltime=0.0, peckdepth=0.0, repeat=1, retractheight=None, chipBreak=False
):
    """
    Generates Gcode for drilling a single hole.

    Takes as input an edge. It assumes the edge is trivial with just two vectors.
    The edge must be aligned with the Z axes (Vector(0,0,1)) or it is an error.

    The first vertex of the edge will be the startpoint
    The second vertex of the edge will be the endpoint.
    All other vertices are ignored.

    additionally, you can pass in a dwelltime, peckdepth, and repeat value.

    These will result in appropriate G81,G82, and G83 codes.

    If chipBreak is True, the generator will produce G73 cycles instead of G83.
    Chipbreaking cycles produce very small retracts to break the chip rather than
    full retracts to clear chips from the hole.
    http://linuxcnc.org/docs/html/gcode/g-code.html#gcode:g73

    """
    startPoint = edge.Vertexes[0].Point
    endPoint = edge.Vertexes[1].Point

    Path.Log.debug(startPoint)
    Path.Log.debug(endPoint)

    Path.Log.debug(numpy.isclose(startPoint.sub(endPoint).x, 0, rtol=1e-05, atol=1e-06))
    Path.Log.debug(numpy.isclose(startPoint.sub(endPoint).y, 0, rtol=1e-05, atol=1e-06))
    Path.Log.debug(endPoint)

    if dwelltime > 0.0 and peckdepth > 0.0:
        raise ValueError("Peck and Dwell cannot be used together")

    if repeat < 1:
        raise ValueError("repeat must be 1 or greater")

    if not type(repeat) is int:
        raise ValueError("repeat value must be an integer")

    if not type(peckdepth) is float:
        raise ValueError("peckdepth must be a float")

    if not type(dwelltime) is float:
        raise ValueError("dwelltime must be a float")

    if retractheight is not None and not type(retractheight) is float:
        raise ValueError("retractheight must be a float")

    if not (
        numpy.isclose(startPoint.sub(endPoint).x, 0, rtol=1e-05, atol=1e-06)
        and (numpy.isclose(startPoint.sub(endPoint).y, 0, rtol=1e-05, atol=1e-06))
    ):
        raise ValueError("edge is not aligned with Z axis")

    if startPoint.z < endPoint.z:
        raise ValueError("start point is below end point")

    cmdParams = {}
    cmdParams["X"] = startPoint.x
    cmdParams["Y"] = startPoint.y
    cmdParams["Z"] = endPoint.z
    cmdParams["R"] = retractheight if retractheight is not None else startPoint.z

    if repeat < 1:
        raise ValueError("repeat must be 1 or greater")

    if not type(repeat) is int:
        raise ValueError("repeat value must be an integer")

    if repeat > 1:
        cmdParams["L"] = repeat

    if peckdepth == 0.0:
        if dwelltime > 0.0:
            cmd = "G82"
            cmdParams["P"] = dwelltime
        else:
            cmd = "G81"
    else:
        cmd = "G73" if chipBreak else "G83"
        cmdParams["Q"] = peckdepth

    return [Path.Command(cmd, cmdParams)]
