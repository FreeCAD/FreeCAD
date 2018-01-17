#  -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 LTS <SammelLothar@gmx.de> under LGPL               *
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
import FreeCADGui
import Path
import Part
from PySide import QtCore, QtGui
import math
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
from PathScripts.PathGeom import PathGeom

"""LeadInOut Dressup MASHIN-CRC USE ROLL-ON ROLL-OFF to profile"""

# Qt tanslation handling
def translate(text, context="PathDressup_LeadInOut", disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

movecommands = ['G1', 'G01', 'G2', 'G02', 'G3', 'G03']
rapidcommands = ['G0', 'G00']
arccommands = ['G2', 'G3', 'G02', 'G03']
global currLocation
currLocation = {}


class ObjectDressup:

    def __init__(self, obj):
        self.obj = obj
        obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))
        obj.addProperty("App::PropertyLink", "Base",  "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The base path to modify"))
        obj.addProperty("App::PropertyBool", "LeadIn", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Calculate roll-on to path"))
        obj.addProperty("App::PropertyBool", "LeadOut", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Calculate roll-off from path"))
        obj.addProperty("App::PropertyBool", "KeepToolDown", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Keep the Tool Down in Path"))
        obj.addProperty("App::PropertyBool", "UseMashineCRC", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Use Mashine Cutter Radius Compensation /Tool Path Offset G41/G42"))
        obj.addProperty("App::PropertyDistance", "Length", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Length or Radius of the approach"))
        obj.addProperty("App::PropertyEnumeration", "StyleOn", "Path", QtCore.QT_TRANSLATE_NOOP("PathDressup_LeadInOut", "The Style of LeadIn the Path"))
        obj.StyleOn = ["Arc", "Tangent", "Perpendicular"]
        obj.addProperty("App::PropertyEnumeration", "StyleOff", "Path", QtCore.QT_TRANSLATE_NOOP("PathDressup_LeadInOut", "The Style of LeadOut the Path"))
        obj.StyleOff = ["Arc", "Tangent", "Perpendicular"]
        obj.addProperty("App::PropertyEnumeration", "RadiusCenter", "Path", QtCore.QT_TRANSLATE_NOOP("PathDressup_LeadInOut", "The Mode of Point Radiusoffset or Center"))
        obj.RadiusCenter = ["Radius", "Center"]
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None
    def setup(self, obj):
        obj.Length = 5.0
        obj.LeadIn = True
        obj.LeadOut = True
        obj.KeepToolDown=False
        obj.UseMashineCRC=False
        obj.StyleOn = 'Arc'
        obj.StyleOff = 'Arc'
        obj.RadiusCenter = 'Radius'
        obj.ToolController = obj.Base.ToolController
 
    def execute(self, obj):
        if not obj.Base:
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            return
        if not obj.Base.Path:
            return
        if obj.Length < 0:
            PathLog.error(translate("Length/Radius positiv not Null\n"))
            obj.Length = 0.1
        self.wire, self.rapids = PathGeom.wireForPath(obj.Base.Path)
        obj.Path = self.generateLeadInOutCurve(obj)

    def getDirectionOfPath(self,obj):
        if obj.Base.Side == 'Outside':
            if obj.Base.Direction =='CW':
                return 'left'
            else:
                return 'right'
        else: 
            if obj.Base.Direction =='CW':
                return 'right'
        return 'left' 

    def normalize(self, Vector):
        x=Vector.x
        y=Vector.y
        len = math.sqrt( x*x + y*y )
        if((math.fabs(len))> 0.0000000000001):
            vx = round(x / len,0)
            vy = round(y / len,0)  
        return FreeCAD.Base.Vector(vx,vy,0)

    def getLeadStart(self,obj, queue,action):
        '''returns Lead In G-code.'''
        results = []
        horizFeed = obj.Base.ToolController.HorizFeed.Value
        vertFeed = obj.Base.ToolController.VertFeed.Value
        toolnummer = obj.Base.ToolController.ToolNumber
        q0P = queue[0].Placement.Base
        q1P = queue[1].Placement.Base
        if self.getDirectionOfPath(obj) == 'left':# set correct twist command
            arcdir = "G3"
        else:
            arcdir = "G2"
        R= obj.Length.Value #Radius of roll or length
        if queue[1].Name == "G1": #line
            v = self.normalize(q1P.sub(q0P))
            if self.getDirectionOfPath(obj) == 'right':
                off_v = FreeCAD.Base.Vector(v.y*R,-v.x*R,0.0)
            else:
                off_v = FreeCAD.Base.Vector(-v.y*R,v.x*R,0.0)
            offsetvector = FreeCAD.Base.Vector(v.x*R,v.y*R, 0) #IJ
            if obj.RadiusCenter == 'Radius':
                leadstart = (q0P.add(off_v)).sub(offsetvector) #Rmode
            else: 
                leadstart = q0P.add(off_v) #Dmode
        else:#arc
            r = obj.Length.Value #Radius of roll or length
            if obj.Base.Side == 'Inside':
                r=r*-1.0
            # get the center of the arc re roll on 
            arccenter = FreeCAD.Base.Vector(q0P.x + queue[1].I, q0P.y + queue[1].J,q1P.z)
            # Radius of arc to roll on 
            R = math.hypot(queue[1].I, queue[1].J)
            # find angle of original center to startpoint
            v1= q0P.sub(arccenter)
            segAngle = math.atan(round(v1.x/v1.y,4))
            # Find Points to roll on angle subtended by the offset
            RollCenter = FreeCAD.Base.Vector(0,0,q1P.z)
            RollCenter.x = arccenter.x + math.sin(segAngle)*(R+r)
            RollCenter.y = arccenter.y + math.cos(segAngle)*(R+r)
            RollExtend = FreeCAD.Base.Vector(0,0, q1P.z)
            RollExtend.x = arccenter.x + math.sin(segAngle)*(R+r*2)
            RollExtend.y = arccenter.y + math.cos(segAngle)*(R+r*2)
            dx = RollExtend.x - RollCenter.x
            dy = RollExtend.y - RollCenter.y
            if self.getDirectionOfPath(obj) == 'right':
                off_v = FreeCAD.Base.Vector(dy,-dx,0.0)
            else:
                off_v = FreeCAD.Base.Vector(-dy,dx,0.0)
            if obj.RadiusCenter == 'Radius':
                leadstart = RollCenter.add(off_v) #Rmode
            else: 
                leadstart = RollExtend #Dmode
            offsetvector = RollCenter.sub(leadstart)#IJ
        if action == 'start':
            extendcommand = Path.Command('G0', {"X": 0.0, "Y": 0.0, "Z": obj.Base.ClearanceHeight.Value})
            results.append(extendcommand)
            extendcommand = Path.Command('G0', {"X": leadstart.x, "Y": leadstart.y, "Z": obj.Base.ClearanceHeight.Value})
            results.append(extendcommand)
            extendcommand = Path.Command('G0', {"X": leadstart.x, "Y": leadstart.y, "Z": obj.Base.SafeHeight.Value})
            results.append(extendcommand)
        if action == 'layer':
            if not obj.KeepToolDown:
                extendcommand = Path.Command('G0', {"Z": obj.Base.SafeHeight.Value})
                results.append(extendcommand)
            extendcommand = Path.Command('G0', {"X": leadstart.x, "Y": leadstart.y})
            results.append(extendcommand)
        extendcommand = Path.Command('G1', {"X": leadstart.x, "Y": leadstart.y, "Z": q1P.z, "F": vertFeed})
        results.append(extendcommand)
        if obj.UseMashineCRC:
                if self.getDirectionOfPath(obj) == 'right':
                    results.append(Path.Command('G42', {'D':toolnummer}))
                else:
                    results.append(Path.Command('G41', {'D':toolnummer}))
        if obj.StyleOn == 'Arc':
            arcmove = Path.Command(arcdir, {"X": q0P.x, "Y": q0P.y, "I": offsetvector.x, "J": offsetvector.y, "F": horizFeed})  # add G2/G3 move
            results.append(arcmove)
        elif obj.StyleOn == 'Tangent':
            extendcommand = Path.Command('G1', {"X": q0P.x, "Y": q0P.y, "Z": q1P.z, "F": horizFeed})
            results.append(extendcommand)
        else :
            PathLog.notice(" CURRENT_IN Perp")
        return results     

    def getLeadEnd(self,obj, queue,action):
        '''returns the  Gcode of LeadOut.'''
        global currLocation
        results = []
        horizFeed = obj.Base.ToolController.HorizFeed.Value
        q0P = queue[0].Placement.Base
        q1P = queue[1].Placement.Base
        R= obj.Length.Value #Radius of roll or length
        if self.getDirectionOfPath(obj) == 'right':# set correct twist command
            arcdir = "G2"
        else:
            arcdir = "G3"
        if queue[1].Name == "G1": #line
            v = self.normalize(q1P.sub(q0P))
            if self.getDirectionOfPath(obj) == 'right':
                off_v = FreeCAD.Base.Vector(v.y*R,-v.x*R,0.0)
            else:
                off_v = FreeCAD.Base.Vector(-v.y*R,v.x*R,0.0)
            offsetvector = FreeCAD.Base.Vector(v.x*R,v.y*R, 0.0) 
            if obj.RadiusCenter == 'Radius':
                leadend = (q1P.add(off_v)).add(offsetvector) #Rmode
            else: 
                leadend = q1P.add(off_v) #Dmode
            IJ= off_v

        else:#dealing with a circle
            r = obj.Length.Value #Radius of roll or length
            if obj.Base.Side == 'Inside':
                r=r*-1.0
            # get the center of the arc we roll on 
            arccenter = FreeCAD.Base.Vector(q0P.x + queue[1].I, q0P.y + queue[1].J,q1P.z)
            # Radius of arc to roll on 
            R = math.hypot(queue[1].I, queue[1].J)
            # find angle of original center to startpoint
            v1= q1P.sub(arccenter)
            segAngle = math.atan(round(v1.x/v1.y,4))
            # Find Points to roll on angle subtended by the offset
            RollCenter = FreeCAD.Base.Vector(0,0,q1P.z)
            RollCenter.x = arccenter.x + math.sin(segAngle)*(R+r)
            RollCenter.y = arccenter.y + math.cos(segAngle)*(R+r)
            RollExtend = FreeCAD.Base.Vector(0,0, q1P.z)
            RollExtend.x = arccenter.x + math.sin(segAngle)*(R+r*2)
            RollExtend.y = arccenter.y + math.cos(segAngle)*(R+r*2)
            dx = RollExtend.x - RollCenter.x
            dy = RollExtend.y - RollCenter.y
            if self.getDirectionOfPath(obj) == 'right':
                off_v = FreeCAD.Base.Vector(-dy,dx,0.0)
            else:
                 off_v = FreeCAD.Base.Vector(dy,-dx,0.0)
            if obj.RadiusCenter == 'Radius':
                leadend = RollCenter.add(off_v) #Rmode
            else: 
                leadend = RollExtend #Dmode
            IJ = RollCenter.sub(q1P)#IJ
        if obj.StyleOff == 'Arc':
            arcmove = Path.Command(arcdir, {"X": leadend.x, "Y": leadend.y, "I": IJ.x, "J": IJ.y, "F": horizFeed})  # add G2/G3 move
            results.append(arcmove)
        elif obj.StyleOff == 'Tangent':
            extendcommand = Path.Command('G1', {"X": leadend.x, "Y": leadend.y, "Z": q1P.z, "F": horizFeed})
            results.append(extendcommand)
        else :
            PathLog.notice(" CURRENT_IN Perp")
        if obj.UseMashineCRC:#crc off
            results.append(Path.Command('G40', {}))
        return results     

    def generateLeadInOutCurve(self, obj):
        global currLocation
        firstmove = Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 0.0})
        currLocation.update(firstmove.Parameters)
        newpath = []
        queue = []
        num=0
        action= 'start'
        for curCommand in obj.Base.Path.Commands:
            replace = None
            # don't worry about non-move commands, just add to output
            if curCommand.Name not in movecommands + rapidcommands:
                newpath.append(curCommand)
                continue

            # rapid retract triggers exit move, else just add to output
            if curCommand.Name in rapidcommands:
                #detect start position
                if (curCommand.x != None) or (curCommand.y != None):
                    firstmove = curCommand
                currLocation.update(curCommand.Parameters)
                if action !='start':#done move out 
                    if obj.LeadOut:
                        temp = self.getLeadEnd(obj,queue,'end')
                        newpath.extend(temp)
                        newpath.append(curCommand) #Z clear DONE
 
            if curCommand.Name in movecommands:
                queue.append(curCommand)
                if action == 'start' and len(queue) <2:
                    continue
                if action == 'layer':
                    if len(queue) > 2: queue.pop(0)
                    if obj.LeadIn:
                        temp=self.getLeadStart(obj,queue,action)
                        newpath.extend(temp)
                        newpath.append(curCommand)
                        action='none'
                        currLocation.update(curCommand.Parameters)
                    else:
                        newpath.append(curCommand)
                if curCommand.z != currLocation["Z"] and action != 'start':# vertical feeding to depth
                    if obj.LeadOut:#fish cycle 
                        if len(queue) > 2: queue.pop(len(queue)-1)
                        temp = self.getLeadEnd(obj,queue,action)
                        newpath.extend(temp)
                        action = 'layer'
                        if len(queue) > 2: queue.pop(0)
                        continue
                    else:
                        newpath.append(curCommand)
                if len(queue) > 2: queue.pop(0)
                if obj.LeadIn and len(queue)>=2 and action == 'start':
                    temp=self.getLeadStart(obj,queue,action)
                    newpath.extend(temp)
                    newpath.append(curCommand)
                    action='none'
                    currLocation.update(curCommand.Parameters)
                else:
                    newpath.append(curCommand)
                currLocation.update(curCommand.Parameters)
        commands = newpath
        return Path.Path(commands)

class ViewProviderDressup:

    def __init__(self, vobj):
        vobj.Proxy = self

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
        return [self.obj.Base]

    def onDelete(self, arg1=None, arg2=None):
        PathLog.debug("Deleting Dressup")
        '''this makes sure that the base operation is added back to the project and visible'''
        FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
        job = PathUtils.findParentJob(self.obj)
        job.Proxy.addOperation(arg1.Object.Base)
        arg1.Object.Base = None
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class CommandPathDressupLeadInOut:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathDressup_LeadInOut", "LeadInOut Dressup"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathDressup_LeadInOut", "Creates a Cutter Radius Compensation G41/G42 Entry Dressup object from a selected path")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            PathLog.error(translate("Please select one path object\n"))
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            PathLog.error(translate("The selected object is not a path\n"))
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
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('PathScripts.PathDressupLeadInOut.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False')
        FreeCADGui.doCommand('dbo.setup(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('PathDressup_LeadInOut', CommandPathDressupLeadInOut())

PathLog.notice("Loading PathDressupLeadInOut... done\n")
