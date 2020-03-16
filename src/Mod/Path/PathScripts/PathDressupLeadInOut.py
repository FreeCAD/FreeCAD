#  -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 LTS <SammelLothar@gmx.de> under LGPL               *
# *   Copyright (c) 2020 Schildkroet                                        *
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

from __future__ import print_function

import FreeCAD
import Path
import PathScripts.PathDressup as PathDressup
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import math
import copy

from PySide import QtCore

__doc__ = """LeadInOut Dressup MASHIN-CRC USE ROLL-ON ROLL-OFF to profile"""

if FreeCAD.GuiUp:
    import FreeCADGui

# Qt translation handling
def translate(text, context="Path_DressupLeadInOut", disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

movecommands = ['G1', 'G01', 'G2', 'G02', 'G3', 'G03']
rapidcommands = ['G0', 'G00']
arccommands = ['G2', 'G3', 'G02', 'G03']
currLocation = {}


class ObjectDressup:

    def __init__(self, obj):
        self.obj = obj
        obj.addProperty("App::PropertyLink", "Base",  "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base path to modify"))
        obj.addProperty("App::PropertyBool", "LeadIn", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Calculate roll-on to path"))
        obj.addProperty("App::PropertyBool", "LeadOut", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Calculate roll-off from path"))
        obj.addProperty("App::PropertyBool", "KeepToolDown", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Keep the Tool Down in Path"))
        obj.addProperty("App::PropertyBool", "UseMachineCRC", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Use Machine Cutter Radius Compensation /Tool Path Offset G41/G42"))
        obj.addProperty("App::PropertyDistance", "Length", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Length or Radius of the approach"))
        obj.addProperty("App::PropertyEnumeration", "StyleOn", "Path", QtCore.QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "The Style of LeadIn the Path"))
        obj.StyleOn = ["Arc", "Tangent", "Perpendicular"]
        obj.addProperty("App::PropertyEnumeration", "StyleOff", "Path", QtCore.QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "The Style of LeadOut the Path"))
        obj.StyleOff = ["Arc", "Tangent", "Perpendicular"]
        obj.addProperty("App::PropertyEnumeration", "RadiusCenter", "Path", QtCore.QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "The Mode of Point Radiusoffset or Center"))
        obj.RadiusCenter = ["Radius", "Center"]
        obj.Proxy = self
        obj.addProperty("App::PropertyDistance", "ExtendLeadIn", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Extends LeadIn distance"))
        obj.addProperty("App::PropertyDistance", "ExtendLeadOut", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Extends LeadOut distance"))
        
        self.wire = None
        self.rapids = None

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        # pylint: disable=unused-argument
        return None

    def setup(self, obj):
        obj.Length = 5.0
        obj.LeadIn = True
        obj.LeadOut = True
        obj.KeepToolDown = False
        obj.UseMachineCRC = False
        obj.StyleOn = 'Arc'
        obj.StyleOff = 'Arc'
        obj.RadiusCenter = 'Radius'
        obj.ExtendLeadIn = 0
        obj.ExtendLeadOut = 0

    def execute(self, obj):
        if not obj.Base:
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            return
        if not obj.Base.Path:
            return
        if obj.Length < 0:
            PathLog.error(translate("Length/Radius positive not Null")+"\n")
            obj.Length = 0.1
        self.wire, self.rapids = PathGeom.wireForPath(obj.Base.Path)
        obj.Path = self.generateLeadInOutCurve(obj)

    def getDirectionOfPath(self, obj):
        op = PathDressup.baseOp(obj.Base)
        if hasattr(op, 'Side') and op.Side == 'Outside':
            if hasattr(op, 'Direction') and op.Direction == 'CW':
                return 'left'
            else:
                return 'right'
        else:
            if hasattr(op, 'Direction') and op.Direction == 'CW':
                return 'right'
        return 'left'

    def normalize(self, Vector):
        x = Vector.x
        y = Vector.y
        length = math.sqrt(x*x + y*y)
        #print("Len: {}".format(length))
        if((math.fabs(length)) > 0.0000000000001):
            vx = round(x / length, 2)
            vy = round(y / length, 2)
        return FreeCAD.Vector(vx, vy, 0)
    
    def invert(self,  Vector):
        x = Vector.x * -1
        y = Vector.y * -1
        z = Vector.z * -1
        return FreeCAD.Vector(x, y, z)
    
    def multiply(self,  Vector,  len):
        x = Vector.x * len
        y = Vector.y * len
        z = Vector.z * len
        return FreeCAD.Vector(x, y, z)
    
    def rotate(self,  Vector,  angle):
        s = math.sin(math.radians(angle))
        c = math.cos(math.radians(angle))
        xnew = Vector.x * c - Vector.y * s;
        ynew = Vector.x * s + Vector.y * c;
        return FreeCAD.Vector(xnew, ynew, Vector.z)

    def getLeadStart(self, obj, queue, action):
        '''returns Lead In G-code.'''
        results = []
        # zdepth = currLocation["Z"]
        op = PathDressup.baseOp(obj.Base)
        tc = PathDressup.toolController(obj.Base)
        horizFeed = tc.HorizFeed.Value
        vertFeed = tc.VertFeed.Value
        toolnummer = tc.ToolNumber

        # set the correct twist command
        if self.getDirectionOfPath(obj) == 'left':
            arcdir = "G3"
        else:
            arcdir = "G2"

        R = obj.Length.Value  # Radius of roll or length
        if queue[1].Name == "G1":  # line
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            v = self.normalize(p1.sub(p0))
            # PathLog.notice(" CURRENT_IN : P0 Z:{} p1 Z:{}".format(p0.z,p1.z))
        else:
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            # PathLog.notice(" CURRENT_IN ARC : P0 X:{} Y:{} P1 X:{} Y:{} ".format(p0.x,p0.y,p1.x,p1.y))
            v = self.normalize(p1.sub(p0))

        if self.getDirectionOfPath(obj) == 'right':
            off_v = FreeCAD.Vector(v.y*R, -v.x*R, 0.0)
        else:
            off_v = FreeCAD.Vector(-v.y*R, v.x*R, 0.0)

        # Check if we enter at line or arc command
        if queue[1].Name in movecommands and queue[1].Name not in arccommands:
            # We have a line move
            vec = p1.sub(p0)
            vec_n = self.normalize(vec)
            vec_inv = self.invert(vec_n)
            vec_off = self.multiply(vec_inv,  obj.ExtendLeadIn)
            #print("LineCMD: {}, Vxinv: {}, Vyinv: {}, Vxoff: {}, Vyoff: {}".format(queue[0].Name, vec_inv.x, vec_inv.y,  vec_off.x,  vec_off.y))
        else:
            # We have an arc move
            #print("Arc X{} Y{} P {}".format(p0.x,  p0.y, queue[0].Parameters))
            pij = copy.deepcopy(p0)
            pij.x += queue[1].Parameters['I']
            pij.y += queue[1].Parameters['J']
            ve = pij.sub(p0)
            #print("I{} J{}, vx {} vy {}".format(pij.x,  pij.y,  ve.x,  ve.y))
            if arcdir == "G2":
                vec_rot = self.rotate(ve,  90)
            else:
                vec_rot = self.rotate(ve,  -90)
            
            #print("vro{} vro{}".format(vec_rot.x,  vec_rot.y))
            vec_n = self.normalize(vec_rot)
            v = self.invert(vec_n)
            
            if arcdir == "G3":
                off_v = FreeCAD.Vector(-v.y*R, v.x*R, 0.0)
            else:
                off_v = FreeCAD.Vector(v.y*R, -v.x*R, 0.0)
            
            vec_off = self.multiply(vec_rot,  obj.ExtendLeadIn)
            #print("vnx{} vny{}, vxi {} vyi {}".format(vec_n.x,  vec_n.y,  v.x,  v.y))
            #print("vxo{} vyo{}".format(off_v.x,  off_v.y))
            #print("vxo{} vyo{}".format(vec_off.x,  vec_off.y))
        
        offsetvector = FreeCAD.Vector(v.x*R-vec_off.x, v.y*R-vec_off.y, 0)  # IJ
        if obj.RadiusCenter == 'Radius':
            leadstart = (p0.add(off_v)).sub(offsetvector)  # Rmode
        else:
            leadstart = p0.add(off_v)  # Dmode

        if action == 'start':
            #print("Start")
            extendcommand = Path.Command('G0', {"X": 0.0, "Y": 0.0, "Z": op.ClearanceHeight.Value})
            results.append(extendcommand)
            extendcommand = Path.Command('G0', {"X": leadstart.x, "Y": leadstart.y, "Z": op.ClearanceHeight.Value})
            results.append(extendcommand)
            extendcommand = Path.Command('G0', {"X": leadstart.x, "Y": leadstart.y, "Z": op.SafeHeight.Value})
            results.append(extendcommand)
        
        if action == 'layer':
            #print("Layer")
            if not obj.KeepToolDown:
                extendcommand = Path.Command('G0', {"Z": op.SafeHeight.Value})
                results.append(extendcommand)
            
            extendcommand = Path.Command('G0', {"X": leadstart.x, "Y": leadstart.y})
            results.append(extendcommand)
        
        extendcommand = Path.Command('G1', {"X": leadstart.x, "Y": leadstart.y, "Z": p1.z, "F": vertFeed})
        results.append(extendcommand)
        
        if obj.UseMachineCRC:
            if self.getDirectionOfPath(obj) == 'right':
                results.append(Path.Command('G42', {'D': toolnummer}))
            else:
                results.append(Path.Command('G41', {'D': toolnummer}))
            
        if obj.StyleOn == 'Arc':
            arcmove = Path.Command(arcdir, {"X": p0.x+vec_off.x, "Y": p0.y+vec_off.y, "I": offsetvector.x+vec_off.x, "J": offsetvector.y+vec_off.y, "F": horizFeed})  # add G2/G3 move
            results.append(arcmove)
            if obj.ExtendLeadIn != 0:
                extendcommand = Path.Command('G1', {"X": p0.x, "Y": p0.y, "F": horizFeed})
                results.append(extendcommand)
        elif obj.StyleOn == 'Tangent':
            extendcommand = Path.Command('G1', {"X": p0.x, "Y": p0.y, "F": horizFeed})
            results.append(extendcommand)
        else:
            PathLog.notice(" CURRENT_IN Perp")
        
        return results

    def getLeadEnd(self, obj, queue, action):
        '''returns the Gcode of LeadOut.'''
         # pylint: disable=unused-argument
        results = []
        horizFeed = PathDressup.toolController(obj.Base).HorizFeed.Value
        R = obj.Length.Value  # Radius of roll or length
        
        # set the correct twist command
        if self.getDirectionOfPath(obj) == 'right':
            arcdir = "G2"
        else:
            arcdir = "G3"
        
        if queue[1].Name == "G1":  # line
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            v = self.normalize(p1.sub(p0))
        else:  # dealing with a circle
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            v = self.normalize(p1.sub(p0))
        
        if self.getDirectionOfPath(obj) == 'right':
            off_v = FreeCAD.Vector(v.y*R, -v.x*R, 0.0)
        else:
            off_v = FreeCAD.Vector(-v.y*R, v.x*R, 0.0)
        
        # Check if we leave at line or arc command
        if queue[1].Name in movecommands and queue[1].Name not in arccommands:
            # We have a line move
            vec = p1.sub(p0)
            vec_n = self.normalize(vec)
            vec_inv = self.invert(vec_n)
            vec_off = self.multiply(vec_inv,  obj.ExtendLeadOut)
            #print("LineCMD: {}, Vxinv: {}, Vyinv: {}, Vxoff: {}, Vyoff: {}".format(queue[0].Name, vec_inv.x, vec_inv.y,  vec_off.x,  vec_off.y))
        else:
            # We have an arc move
            #print("Arc0 X{} Y{} P {}".format(p0.x,  p0.y, queue[0].Parameters))
            #print("Arc1 X{} Y{} P {}".format(p1.x,  p1.y, queue[1].Parameters))
            pij = copy.deepcopy(p0)
            pij.x += queue[1].Parameters['I']
            pij.y += queue[1].Parameters['J']
            ve = pij.sub(p1)
           # print("I{} J{}, vx {} vy {}".format(pij.x,  pij.y,  ve.x,  ve.y))
            if arcdir == "G2":
                vec_rot = self.rotate(ve,  -90)
            else:
                vec_rot = self.rotate(ve,  90)
            
            #print("vro{} vro{}".format(vec_rot.x,  vec_rot.y))
            vec_n = self.normalize(vec_rot)
            v = vec_n
            
            if arcdir == "G3":
                off_v = FreeCAD.Vector(-v.y*R, v.x*R, 0.0)
            else:
                off_v = FreeCAD.Vector(v.y*R, -v.x*R, 0.0)
            
            vec_off = self.multiply(self.invert(vec_rot),  obj.ExtendLeadOut)
            #print("vnx{} vny{}, vxi {} vyi {}".format(vec_n.x,  vec_n.y,  v.x,  v.y))
            #print("vxo{} vyo{}".format(off_v.x,  off_v.y))
            #print("vxo{} vyo{}".format(vec_off.x,  vec_off.y))
        
        #print("Arc0 X{} Y{} P {}".format(p0.x,  p0.y, queue[0].Parameters))
        offsetvector = FreeCAD.Vector(v.x*R-vec_off.x, v.y*R-vec_off.y, 0.0)
        if obj.RadiusCenter == 'Radius':
            leadend = (p1.add(off_v)).add(offsetvector)  # Rmode
            #print("End: X {}, Y{}".format(leadend.x,  leadend.y))
        else:
            leadend = p1.add(off_v)  # Dmode
        
        IJ = off_v  # .negative()
        print("IJ: X {}, Y{}".format(IJ.x,  IJ.y))
        #results.append(queue[1])
        if obj.StyleOff == 'Arc':
            if obj.ExtendLeadOut != 0:
                extendcommand = Path.Command('G1', {"X": p1.x-vec_off.x, "Y": p1.y-vec_off.y, "F": horizFeed})
                results.append(extendcommand)
            arcmove = Path.Command(arcdir, {"X": leadend.x, "Y": leadend.y, "I": IJ.x, "J": IJ.y, "F": horizFeed})  # add G2/G3 move
            results.append(arcmove)
        elif obj.StyleOff == 'Tangent':
            extendcommand = Path.Command('G1', {"X": leadend.x, "Y": leadend.y, "F": horizFeed})
            results.append(extendcommand)
        else:
            PathLog.notice(" CURRENT_IN Perp")
        
        if obj.UseMachineCRC:  # crc off
            results.append(Path.Command('G40', {}))
        
        return results

    def generateLeadInOutCurve(self, obj):
        global currLocation # pylint: disable=global-statement
        firstmove = Path.Command("G0", {"X": 0, "Y": 0, "Z": 0})
        currLocation.update(firstmove.Parameters)
        newpath = []
        queue = []
        action = 'start'
        
        for curCommand in obj.Base.Path.Commands:
            # replace = None
            # don't worry about non-move commands, just add to output
            if curCommand.Name not in movecommands + rapidcommands:
                newpath.append(curCommand)
                continue

            # rapid retract triggers exit move, else just add to output
            if curCommand.Name in rapidcommands:
                # detect start position
                if (curCommand.x is not None) or (curCommand.y is not None):
                    firstmove = curCommand
                currLocation.update(curCommand.Parameters)
                if action != 'start':  # done move out
                    if obj.LeadOut:
                        temp = self.getLeadEnd(obj, queue, 'end')
                        newpath.extend(temp)
                        newpath.append(curCommand)  # Z clear DONE

            if curCommand.Name in movecommands:
                queue.append(curCommand)
                if action == 'start' and len(queue) < 2:
                    continue
                
                if action == 'layer':
                    if len(queue) > 2:
                        queue.pop(0)
                    if obj.LeadIn:
                        temp = self.getLeadStart(obj, queue, action)
                        newpath.extend(temp)
                        #newpath.append(curCommand)
                        action = 'none'
                        currLocation.update(curCommand.Parameters)
                    else:
                        newpath.append(curCommand)
                    
                if curCommand.z != currLocation["Z"] and action != 'start':  # vertical feeding to depth
                    if obj.LeadOut:  # fish cycle
                        if len(queue) > 2:
                            queue.pop(len(queue)-1)
                        
                        temp = self.getLeadEnd(obj, queue, action)
                        newpath.extend(temp)
                        action = 'layer'
                        
                        if len(queue) > 2:
                            queue.pop(0)
                        
                        continue
                    else:
                        newpath.append(curCommand)
                if len(queue) > 2:
                    queue.pop(0)
                
                if obj.LeadIn and len(queue) >= 2 and action == 'start':
                    temp = self.getLeadStart(obj, queue, action)
                    newpath.extend(temp)
                    newpath.append(curCommand)
                    action = 'none'
                    currLocation.update(curCommand.Parameters)
                else:
                    newpath.append(curCommand)
                
                currLocation.update(curCommand.Parameters)
            
        commands = newpath
        return Path.Path(commands)


class ViewProviderDressup:

    def __init__(self, vobj):
        self.obj = vobj.Object

    def attach(self, vobj):
        self.obj = vobj.Object

    def claimChildren(self):
        if hasattr(self.obj.Base, "InList"):
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
                    print(i.Group)
        # FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        return [self.obj.Base]

    def onDelete(self, arg1=None, arg2=None):
        '''this makes sure that the base operation is added back to the project and visible'''
        # pylint: disable=unused-argument
        PathLog.debug("Deleting Dressup")
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(self.obj)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        # pylint: disable=unused-argument
        return None


class CommandPathDressupLeadInOut:
    # pylint: disable=no-init

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "LeadInOut Dressup"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "Creates a Cutter Radius Compensation G41/G42 Entry Dressup object from a selected path")}

    def IsActive(self):
        op = PathDressup.selection()
        if op:
            return not PathDressup.hasEntryMethod(op)
        return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            PathLog.error(translate("Please select one path object")+"\n")
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            PathLog.error(translate("The selected object is not a path")+"\n")
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            PathLog.error(translate("Please select a Profile object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("Create LeadInOut Dressup"))
        FreeCADGui.addModule("PathScripts.PathDressupLeadInOut")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "LeadInOutDressup")')
        FreeCADGui.doCommand('dbo = PathScripts.PathDressupLeadInOut.ObjectDressup(obj)')
        FreeCADGui.doCommand('base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('job = PathScripts.PathUtils.findParentJob(base)')
        FreeCADGui.doCommand('obj.Base = base')
        FreeCADGui.doCommand('job.Proxy.addOperation(obj, base)')
        FreeCADGui.doCommand('obj.ViewObject.Proxy = PathScripts.PathDressupLeadInOut.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(base.Name).Visibility = False')
        FreeCADGui.doCommand('dbo.setup(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_DressupLeadInOut', CommandPathDressupLeadInOut())

PathLog.notice("Loading PathDressupLeadInOut... done\n")
