# SPDX-License-Identifier: LGPL-2.1-or-later
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

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class MachineState:
    Tracked = [
        "X",
        "Y",
        "Z",
        "A",
        "B",
        "C",
        "F",
        "S",
        "T",
        "Coolant",  # true/false for on/off
        "WCS",  # work-coordinate system Gcode
        "Spindle",  # rpms
        "ReturnMode",  # Z(G98) or R(G99) for drills
        "G0F",  # F for G0's, distinct from all other move F. all G0's should have an F now.
    ]

    def __init__(self, initial: None | dict = None):
        """an initial state is optional, and doesn't have to set all Tracked properties"""
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
        self.ReturnMode = "Z"  #: str = Z|R for G98/G99, for drill cycles
        self.G0F = 0.0  #: float = field(default=None)
        self.S = 0  #: int = field(default=0)
        self.T = None  #: int = field(default=None)
        if missing := [k for k in self.Tracked if k not in dir(self)]:
            raise Exception(f"Internal: didn't initialize a Tracked Parameter {missing}")

        if initial:
            self.setState(initial)

    def addCommand(self, command):
        """Processes a command and updates the internal state of the machine.
        Returns true if the command has alterned the machine state"""
        oldstate = self.getState()
        if command.Name == "M6":
            self.T = int(command.Parameters["T"])
            return not oldstate == self.getState()

        if command.Name in ["M3", "M4"]:
            self.S = command.Parameters["S"]
            self.Spindle = "CW" if command.Name == "M3" else "CCW"
            return not oldstate == self.getState()

        if command.Name in ["G98", "G99"]:
            self.ReturnMode = "R" if command.Name == "G99" else "Z"
            return not oldstate == self.getState()

        if command.Name in ["M2", "M5"]:
            self.S = 0
            self.Spindle = "off"
            return not oldstate == self.getState()

        if command.Name in self.WCSLIST:
            self.WCS = command.Name
            return not oldstate == self.getState()

        if command.Name in Path.Geom.CmdMoveDrill:
            # Special logic for drill: old-Z or R
            for p in command.Parameters:

                # Z depends on ReturnMode
                if p == "Z":
                    continue

                if p in self.Tracked:
                    self.__setattr__(p, command.Parameters[p])

            if self.ReturnMode == "R":
                self.Z = command.Parameters["R"]
            else:  # Z/G98 mode
                oldZ = self.Z
                r = command.Parameters.get("R", None)
                if oldZ is None or r is None:
                    # can't test R vs old Z
                    # You need to establish an old Z, and specify an R
                    # Which shouldn't happen unless testing
                    self.Z = oldZ
                else:
                    self.Z = max(oldZ, r)

            return not oldstate == self.getState()

        # just the usual GCode Parameters (except G0's F)
        for p in self.Tracked:
            if p not in command.Parameters:
                continue

            # G0's F is distinct
            if command.Name in ["G0", "G00"] and p == "F":
                self.G0F = command.Parameters[p]
            else:
                self.__setattr__(p, command.Parameters[p])

        return not oldstate == self.getState()

    def addCommands(self, commands):
        """Processes a command or list of commands and updates the internal state of the machine"""
        if isinstance(commands, (list, tuple)):
            for command in commands:
                self.addCommand(command)
        else:
            return self.addCommand(commands)

        return False

    def copy(self):
        return MachineState(self.getState())

    def setState(self, state: dict | None):
        """Set the state from a dict
        Convenience mode: None causes all parameters=None
        """
        for s in self.Tracked:
            if state is None:
                setattr(self, s, None)
            elif s in state:
                setattr(self, s, state[s])

    def getState(self):
        """
        Returns a dictionary of the current machine state
        """
        return {k: getattr(self, k) for k in self.Tracked}

    def getPosition(self):
        """
        Returns a vector of the current machine position
        """

        # This is technical debt.  The actual position may include a rotation
        # component as well.  We should probably be returning a placement
        return FreeCAD.Vector(self.X, self.Y, self.Z)

    def __str__(self):
        return f"MachineState({self.getState()})"
