# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import Path
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Thread Milling generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "CAM thread milling operation."

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


def _comment(path, msg):
    """Nice for debugging to insert markers into generated g-code"""
    if False:
        path.append(Path.Command("(------- {} -------)".format(msg)))


class _Thread(object):
    """Helper class for dealing with different thread types"""

    def __init__(self, cmd, zStart, zFinal, pitch, internal):
        self.cmd = cmd
        if zStart < zFinal:
            self.pitch = pitch
        else:
            self.pitch = -pitch
        self.hPitch = self.pitch / 2
        self.zStart = zStart
        self.zFinal = zFinal
        self.internal = internal

    def overshoots(self, z):
        """overshoots(z) ... returns true if adding another half helix goes beyond the thread bounds"""
        if self.pitch < 0:
            return z + self.hPitch < self.zFinal
        return z + self.hPitch > self.zFinal

    def adjustX(self, x, dx):
        """adjustX(x, dx) ... move x by dx, the direction depends on the thread settings"""
        if self.isG3() == (self.pitch > 0):
            return x + dx
        return x - dx

    def adjustY(self, y, dy):
        """adjustY(y, dy) ... move y by dy, the direction depends on the thread settings"""
        if self.isG3():
            return y - dy
        return y - dy

    def isG3(self):
        """isG3() ... returns True if this is a G3 command"""
        return self.cmd in ["G3", "G03", "g3", "g03"]

    def isUp(self):
        """isUp() ... returns True if the thread goes from the bottom up"""
        return self.pitch > 0

    def g4Opposite(self):
        return "G2" if self.isG3() else "G3"

    def g4LeadInOut(self):
        if self.internal:
            return self.cmd
        return self.g4Opposite()

    def g4Start2Elevator(self):
        return self.g4Opposite()


def generate(center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, start):
    """generate(center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, start) ... returns the g-code to mill the given internal thread"""
    thread = _Thread(cmd, zStart, zFinal, pitch, radius > elevator)

    yMin = center.y - radius
    yMax = center.y + radius

    path = []
    # at this point the tool is at a safe heiht (depending on the previous thread), so we can move
    # into position first, and then drop to the start height. If there is any material in the way this
    # op hasn't been setup properly.
    if leadInOut:
        _comment(path, "lead-in")
        if start is None:
            path.append(Path.Command("G0", {"X": center.x, "Y": center.y + elevator}))
            path.append(Path.Command("G0", {"Z": thread.zStart}))
        else:
            path.append(
                Path.Command(
                    thread.g4Start2Elevator(),
                    {
                        "X": center.x,
                        "Y": center.y + elevator,
                        "Z": thread.zStart,
                        "I": (center.x - start.x) / 2,
                        "J": (center.y + elevator - start.y) / 2,
                        "K": (thread.zStart - start.z) / 2,
                    },
                )
            )
        path.append(
            Path.Command(
                thread.g4LeadInOut(),
                {"Y": yMax, "J": (yMax - (center.y + elevator)) / 2},
            )
        )
        _comment(path, "lead-in")
    else:
        if start is None:
            path.append(Path.Command("G0", {"X": center.x, "Y": center.y + elevator}))
            path.append(Path.Command("G0", {"Z": thread.zStart}))
        else:
            path.append(
                Path.Command(
                    "G0", {"X": center.x, "Y": center.y + elevator, "Z": thread.zStart}
                )
            )
        path.append(Path.Command("G1", {"Y": yMax}))

    z = thread.zStart
    r = -radius
    i = 0
    while not Path.Geom.isRoughly(z, thread.zFinal):
        if thread.overshoots(z):
            break
        if 0 == (i & 0x01):
            y = yMin
        else:
            y = yMax
        path.append(Path.Command(thread.cmd, {"Y": y, "Z": z + thread.hPitch, "J": r}))
        r = -r
        i = i + 1
        z = z + thread.hPitch

    if Path.Geom.isRoughly(z, thread.zFinal):
        x = center.x
        y = yMin if (i & 0x01) else yMax
    else:
        n = math.fabs(thread.zFinal - thread.zStart) / thread.hPitch
        k = n - int(n)
        dy = math.cos(k * math.pi)
        dx = math.sin(k * math.pi)
        y = thread.adjustY(center.y, r * dy)
        x = thread.adjustX(center.x, r * dx)
        _comment(path, "finish-thread")
        path.append(
            Path.Command(thread.cmd, {"X": x, "Y": y, "Z": thread.zFinal, "J": r})
        )
        _comment(path, "finish-thread")

    a = math.atan2(y - center.y, x - center.x)
    dx = math.cos(a) * (radius - elevator)
    dy = math.sin(a) * (radius - elevator)
    Path.Log.debug("")
    Path.Log.debug("a={}: dx={:.2f}, dy={:.2f}".format(a / math.pi * 180, dx, dy))

    elevatorX = x - dx
    elevatorY = y - dy
    Path.Log.debug(
        "({:.2f}, {:.2f}) -> ({:.2f}, {:.2f})".format(x, y, elevatorX, elevatorY)
    )

    if leadInOut:
        _comment(path, "lead-out")
        path.append(
            Path.Command(
                thread.g4LeadInOut(),
                {"X": elevatorX, "Y": elevatorY, "I": -dx / 2, "J": -dy / 2},
            )
        )
        _comment(path, "lead-out")
    else:
        path.append(Path.Command("G1", {"X": elevatorX, "Y": elevatorY}))

    return (path, FreeCAD.Vector(elevatorX, elevatorY, thread.zFinal))
