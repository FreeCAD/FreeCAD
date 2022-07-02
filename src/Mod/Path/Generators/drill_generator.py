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


import PathScripts.PathLog as PathLog
import Path
import numpy

__title__ = "Drilling Path Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Generates the drilling toolpath for a single spotshape"


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


def generate(edge, dwelltime=0.0, peckdepth=0.0, repeat=1, retractheight=None):
    startPoint = edge.Vertexes[0].Point
    endPoint = edge.Vertexes[1].Point

    PathLog.debug(startPoint)
    PathLog.debug(endPoint)

    PathLog.debug(numpy.isclose(startPoint.sub(endPoint).x, 0, rtol=1e-05, atol=1e-06))
    PathLog.debug(numpy.isclose(startPoint.sub(endPoint).y, 0, rtol=1e-05, atol=1e-06))
    PathLog.debug(endPoint)

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

    if repeat > 1:
        cmdParams["L"] = repeat

    if peckdepth == 0.0:
        if dwelltime > 0.0:
            cmd = "G82"
            cmdParams["P"] = dwelltime
        else:
            cmd = "G81"
    else:
        cmd = "G83"
        cmdParams["Q"] = peckdepth

    return [Path.Command(cmd, cmdParams)]
