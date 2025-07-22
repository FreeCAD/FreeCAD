# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic shopinthewoods@gmail.com                 *
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

__title__ = "CAM Machine State"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Dataclass to implement a machinestate tracker"
__contributors__ = ""

import Path
import FreeCAD
import Path

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class MachineState:
    def __init__(self):
        self.WCSLIST = [
            "G53",
            "G54",
            "G55",
            "G56",
            "G57",
            "G58",
            "G59",
            "G59.1",
            "G59.2",
            "G59.3",
            "G59.4",
            "G59.5",
            "G59.6",
            "G59.7",
            "G59.8",
            "G59.9",
        ]

        self.X = 0.0  #: float = field(default=0)
        self.Y = 0.0  #: float = field(default=0)
        self.Z = 0.0  #: float = field(default=0)
        self.A = 0.0  #: float = field(default=0)
        self.B = 0.0  #: float = field(default=0)
        self.C = 0.0  #: float = field(default=0)
        self.F = 0.0  #: float = field(default=None)
        self.Coolant = False  #: bool = field(default=False)
        self.WCS = "G54"  #: str = field(default="G54")
        self.Spindle = "off"  #: str = field(default="off")
        self.S = 0  #: int = field(default=0)
        self.T = None  #: int = field(default=None)

    def addCommand(self, command):
        """Processes a command and updates the internal state of the machine. Returns true if the command has alterned the machine state"""
        oldstate = self.getState()
        if command.Name == "M6":
            self.T = int(command.Parameters["T"])
            return not oldstate == self.getState()

        if command.Name in ["M3", "M4"]:
            self.S = command.Parameters["S"]
            self.Spindle = "CW" if command.Name == "M3" else "CCW"
            return not oldstate == self.getState()

        if command.Name in ["M2", "M5"]:
            self.S = 0
            self.Spindle = "off"
            return not oldstate == self.getState()

        if command.Name in self.WCSLIST:
            self.WCS = command.Name
            return not oldstate == self.getState()

        if command.Name in Path.Geom.CmdMoveDrill:
            oldZ = self.Z
            for p in command.Parameters:
                self.__setattr__(p, command.Parameters[p])
            self.__setattr__("Z", oldZ)
            return not oldstate == self.getState()

        for p in command.Parameters:
            self.__setattr__(p, command.Parameters[p])

        return not oldstate == self.getState()

    def getState(self):
        """
        Returns a dictionary of the current machine state
        """
        state = {}
        state["X"] = self.X
        state["Y"] = self.Y
        state["Z"] = self.Z
        state["A"] = self.A
        state["B"] = self.B
        state["C"] = self.C
        state["F"] = self.F
        state["Coolant"] = self.Coolant
        state["WCS"] = self.WCS
        state["Spindle"] = self.Spindle
        state["S"] = self.S
        state["T"] = self.T

        return state

    def getPosition(self):
        """
        Returns a vector of the current machine position
        """

        # This is technical debt.  The actual position may include a rotation
        # component as well.  We should probably be returning a placement
        return FreeCAD.Vector(self.X, self.Y, self.Z)
