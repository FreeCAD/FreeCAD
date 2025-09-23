# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2024 sliptonic <shopinthewoods@gmail.com>               *
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

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import Path.Base.Util as PathUtil
import Path.Dressup.Utils as PathDressup
import PathScripts.PathUtils as PathUtils
import Path.Geom as PathGeom
from Path.Geom import CmdMoveArc, edgeForCmd, cmdsForEdge, CmdMoveRapid, CmdMoveStraight, CmdMoveDrill, CmdMoveArc
import Part
import Path.Base.MachineState as PathMachineState
import math

if True:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate

PI = math.pi

def normalizeAngle(a):
    """Normalize angle to be within -PI to PI range."""
    while a > PI:
        a = a - 2 * PI
    while a < -PI:
        a = a + 2 * PI
    return a


class Instruction(object):
    """An Instruction is pure python replacement of Path.Command which also tracks its begin position."""

    def __init__(self, begin, cmd, param=None):
        self.begin = begin
        if type(cmd) == Path.Command:
            self.cmd = cmd.Name
            self.param = cmd.Parameters
        else:
            self.cmd = cmd
            if param is None:
                self.param = {}
            else:
                self.param = param

    def setPositionBegin(self, begin):
        self.begin = begin

    def positionBegin(self):
        """positionBegin() ... returns a Vector of the begin position"""
        return self.begin

    def positionEnd(self):
        """positionEnd() ... returns a Vector of the end position"""
        return FreeCAD.Vector(self.x(self.begin.x), self.y(self.begin.y), self.z(self.begin.z))

    def isMove(self):
        return False

    def x(self, default=0):
        return self.param.get('X', default)

    def y(self, default=0):
        return self.param.get('Y', default)

    def z(self, default=0):
        return self.param.get('Z', default)

    def a(self, default=0):
        return self.param.get('A', default)

    def b(self, default=0):
        return self.param.get('B', default)

    def c(self, default=0):
        return self.param.get('C', default)

    def i(self, default=0):
        return self.param.get('I', default)

    def j(self, default=0):
        return self.param.get('J', default)

    def k(self, default=0):
        return self.param.get('K', default)

    def xyBegin(self):
        """xyBegin() ... internal convenience function"""
        return FreeCAD.Vector(self.begin.x, self.begin.y, 0)
    
    def xyEnd(self):
        """xyEnd() ... internal convenience function"""
        return FreeCAD.Vector(self.x(self.begin.x), self.y(self.begin.y), 0)

    def __repr__(self):
        return f"{self.cmd}{self.param}"


class MoveStraight(Instruction):

    def anglesOfTangents(self):
        """anglesOfTangents() ... return a tuple with the tangent angles at begin and end position"""
        begin = self.xyBegin()
        end = self.xyEnd()
        if end == begin:
            Path.Log.debug("MoveStraight: zero-length move, returning 0 angle")
            return (0, 0)
        a = PathGeom.getAngle(end - begin)
        Path.Log.debug(f"MoveStraight: calculated angle {math.degrees(a):.2f}° from {begin} to {end}")
        return (a, a)

    def isMove(self):
        return True


class MoveArc(Instruction):

    def anglesOfTangents(self):
        """anglesOfTangents() ... return a tuple with the tangent angles at begin and end position"""
        begin = self.xyBegin()
        end = self.xyEnd()
        center = FreeCAD.Vector(self.begin.x + self.i(), self.begin.y + self.j(), 0)
        
        Path.Log.debug(f"MoveArc: begin={begin}, end={end}, center={center}")
        
        # calculate angle of the hypotenuse at begin and end
        s0 = PathGeom.getAngle(begin - center)
        s1 = PathGeom.getAngle(end - center)
        
        Path.Log.debug(f"MoveArc: radial angles s0={math.degrees(s0):.2f}°, s1={math.degrees(s1):.2f}°")
        
        # the tangents are perpendicular to the hypotenuse with the sign determined by the
        # direction of the arc
        arc_dir = self.arcDirection()
        t0 = normalizeAngle(s0 + arc_dir)
        t1 = normalizeAngle(s1 + arc_dir)
        
        Path.Log.debug(f"MoveArc: arc direction={math.degrees(arc_dir):.2f}°, tangent angles t0={math.degrees(t0):.2f}°, t1={math.degrees(t1):.2f}°")
        
        return (t0, t1)

    def isMove(self):
        return True


class MoveArcCW(MoveArc):
    def arcDirection(self):
        return -PI/2


class MoveArcCCW(MoveArc):
    def arcDirection(self):
        return PI/2


class Maneuver(object):
    """A series of instructions and moves"""

    def __init__(self, begin=None, instr=None):
        self.instr = instr if instr else []
        self.setPositionBegin(begin if begin else FreeCAD.Vector(0, 0, 0))

    def setPositionBegin(self, begin):
        self.begin = begin
        for i in self.instr:
            i.setPositionBegin(begin)
            begin = i.positionEnd()

    def positionBegin(self):
        return self.begin

    def getMoves(self):
        return [instr for instr in self.instr if instr.isMove()]

    def __repr__(self):
        if self.instr:
            return '\n'.join([str(i) for i in self.instr])
        return ''

    @classmethod
    def InstructionFromCommand(cls, cmd, begin):
        """InstructionFromCommand(cmd, begin) ... create an instruction from a command and a begin position"""
        Path.Log.debug(f"Maneuver.InstructionFromCommand: creating instruction for {cmd.Name} from position {begin}")

        if cmd.Name in CmdMoveStraight + PathGeom.CmdMoveRapid:
            instr = MoveStraight(begin, cmd.Name, cmd.Parameters)
            Path.Log.debug(f"  Created MoveStraight instruction")
            return instr
        if cmd.Name in PathGeom.CmdMoveCW:
            instr = MoveArcCW(begin, cmd.Name, cmd.Parameters)
            Path.Log.debug(f"  Created MoveArcCW instruction")
            return instr
        if cmd.Name in PathGeom.CmdMoveCCW:
            instr = MoveArcCCW(begin, cmd.Name, cmd.Parameters)
            Path.Log.debug(f"  Created MoveArcCCW instruction")
            return instr
        
        instr = Instruction(begin, cmd.Name, cmd.Parameters)
        Path.Log.debug(f"  Created generic Instruction")
        return instr

    @classmethod
    def FromPath(cls, path):
        """FromPath(path) ... create a maneuver from a Path"""
        Path.Log.debug(f"Maneuver.FromPath: creating maneuver from path with {len(path.Commands)} commands")
        
        maneuver = cls()
        begin = FreeCAD.Vector(0, 0, 0)
        instr = []
        
        for i, cmd in enumerate(path.Commands):
            Path.Log.debug(f"Maneuver.FromPath: processing command {i}: {cmd.Name}")
            instruction = cls.InstructionFromCommand(cmd, begin)
            instr.append(instruction)
            begin = instruction.positionEnd()
            Path.Log.debug(f"Maneuver.FromPath: position after command {i}: {begin}")
        
        maneuver.instr = instr
        Path.Log.debug(f"Maneuver.FromPath: created maneuver with {len(instr)} instructions")
        return maneuver


class DressupTangential(object):
    """Dressup to apply tangential parameters for oscillating tangential knife operations."""

    def __init__(self, obj, base, job):
        Path.Log.debug(f"DressupTangential.__init__: initializing with base={base}, job={job}")
        
        obj.addProperty("App::PropertyLink", "Base", "Base", "The base path to modify")
        obj.addProperty("App::PropertyFloat", "TangentialAngle", "Tangential", "Tangential angle offset in degrees")
        obj.Base = base
        obj.TangentialAngle = 0.0
        obj.Proxy = self
        
        Path.Log.debug(f"DressupTangential.__init__: properties set, TangentialAngle={obj.TangentialAngle}")
        Path.Log.info(f"DressupTangential initialized for base: {base.Label if hasattr(base, 'Label') else 'Unknown'}")

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def execute(self, obj):
        """Execute the tangential dressup operation."""
        Path.Log.debug("DressupTangential.execute() starting")
        
        if not obj.Base:
            Path.Log.warning("No base object found, aborting tangential dressup")
            return

        if obj.Base.isDerivedFrom("Path::Feature"):
            if obj.Base.Path:
                Path.Log.debug(f"Processing base path with {len(obj.Base.Path.Commands)} commands")
                Path.Log.debug(f"Tangential angle offset: {obj.TangentialAngle}°")
                
                # Create maneuver from the base path
                maneuver = Maneuver.FromPath(obj.Base.Path)
                output_commands = []
                current_position = FreeCAD.Vector(0, 0, 0)
                move_count = 0
                
                for i, cmd in enumerate(obj.Base.Path.Commands):
                    Path.Log.debug(f"Processing command {i}: {cmd.Name} with params: {cmd.Parameters}")
                    
                    if cmd.Name in PathGeom.CmdMoveAll:
                        # This is a move command, calculate tangent angle
                        move_count += 1
                        Path.Log.debug(f"  Move command #{move_count}: {cmd.Name}")
                        
                        try:
                            instr = Maneuver.InstructionFromCommand(cmd, current_position)
                            tangent_angles = instr.anglesOfTangents()
                            tangent_angle_rad = tangent_angles[1]  # Use end angle
                            tangent_angle_deg = math.degrees(tangent_angle_rad)
                            
                            # Apply user-configured tangential angle offset
                            final_angle = tangent_angle_deg + obj.TangentialAngle
                            
                            Path.Log.debug(f"  Raw tangent angle: {tangent_angle_deg:.3f}°")
                            Path.Log.debug(f"  User offset: {obj.TangentialAngle:.3f}°")
                            Path.Log.debug(f"  Final C-axis angle: {final_angle:.3f}°")
                            
                            # Create new command with C parameter
                            new_params = dict(cmd.Parameters)
                            new_params['C'] = final_angle
                            new_cmd = Path.Command(cmd.Name, new_params)
                            output_commands.append(new_cmd)
                            
                            # Update current position
                            old_position = current_position
                            current_position = instr.positionEnd()
                            Path.Log.debug(f"  Position: {old_position} -> {current_position}")
                            
                        except Exception as e:
                            Path.Log.error(f"  Error processing move command: {e}")
                            # Fall back to original command
                            output_commands.append(cmd)
                    else:
                        # Non-move command, pass through unchanged
                        Path.Log.debug(f"  Non-move command: {cmd.Name}")
                        output_commands.append(cmd)
                        
                        # Update position if this command has position parameters
                        if hasattr(cmd, 'Parameters'):
                            old_position = current_position
                            if 'X' in cmd.Parameters:
                                current_position.x = cmd.Parameters['X']
                            if 'Y' in cmd.Parameters:
                                current_position.y = cmd.Parameters['Y']
                            if 'Z' in cmd.Parameters:
                                current_position.z = cmd.Parameters['Z']
                            if old_position != current_position:
                                Path.Log.debug(f"  Position updated: {old_position} -> {current_position}")
                
                obj.Path = Path.Path(output_commands)
                Path.Log.info(f"Tangential dressup completed: processed {len(output_commands)} commands ({move_count} moves)")
                Path.Log.debug(f"Output path length: {obj.Path.Length}")
            else:
                Path.Log.warning("Base object has no path, creating empty path")
                obj.Path = Path.Path()
        else:
            Path.Log.error(f"Base object is not a Path::Feature: {type(obj.Base)}")
            obj.Path = Path.Path()
        
        Path.Log.debug("DressupTangential.execute() completed")


def Create(base, name="TangentialDressup"):
    """Create a tangential dressup object."""
    Path.Log.debug(f"Creating tangential dressup for base: {base.Label if hasattr(base, 'Label') else base}")
    
    FreeCAD.ActiveDocument.openTransaction("Create Tangential Dressup")
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    job = PathUtils.findParentJob(base)
    
    Path.Log.debug(f"Found parent job: {job.Label if job and hasattr(job, 'Label') else 'None'}")
    
    dressup = DressupTangential(obj, base, job)
    Path.Log.debug("DressupTangential object created")
    
    if FreeCAD.GuiUp:
        import Path.Dressup.Gui.Tangential as TangentialGui
        TangentialGui.ViewProvider(obj.ViewObject)
        Path.Log.debug("GUI ViewProvider attached")
    
    PathScripts.PathUtils.addToJob(obj)
    Path.Log.info(f"Tangential dressup '{obj.Label}' created successfully")
    
    FreeCAD.ActiveDocument.commitTransaction()
    return obj
