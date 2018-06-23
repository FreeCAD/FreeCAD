#  -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 LTS <SammelLothar@gmx.de> under LGPL               *
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
import PathScripts.PathDressup as PathDressup
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import PathScripts.PathGeom as PathGeom
import math
import DraftVecUtils as D
from PySide import QtCore


"""Trochoidal Milling Path Dressup to profile"""

# Qt tanslation handling
def translate(text, context="Path_DressupTrochoidal", disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

movecommands = ['G1', 'G01','G2', 'G3', 'G02', 'G03']
rapidcommands = ['G0', 'G00']
arccommands = ['G2', 'G3', 'G02', 'G03']
global currLocation
currLocation = {}

class ObjectDressup:

    def __init__(self, obj):
        self.obj = obj
        obj.addProperty("App::PropertyLink", "Base",  "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base path to modify"))
        obj.addProperty("App::PropertyDistance", "StepWidthAlongPath", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Stepwidth Along Path"))
        obj.addProperty("App::PropertyDistance", "MachiningAllowance", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Keep Material to Finish"))
        obj.addProperty("App::PropertyBool", "HelixDown", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Move Tool Helical Down in Path"))
        obj.addProperty("App::PropertyEnumeration", "ClimbOrConventional", "Path", QtCore.QT_TRANSLATE_NOOP("Path_DressupTrochoidal", "Mill Direction"))
        obj.ClimbOrConventional = ["Climb", "Conventional"]
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def setup(self, obj):
        
        tc = PathDressup.toolController(obj.Base)
        if hasattr(tc, 'Tool'):
            obj.StepWidthAlongPath = tc.Tool.Diameter / 10
        else:
            PathLog.error(translate("NO TOOL detected \n")) 
            obj.StepWidthAlongPath = 0.5   
        obj.MachiningAllowance = 0.0
        obj.HelixDown = True
        obj.ClimbOrConventional = "Conventional"

    def execute(self, obj):
        if not obj.Base:
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            return
        if not obj.Base.Path:
            return
        if obj.Base.JoinType == 'Round':
            PathLog.error(translate("SET JoinType of Profile SQUARE \n")) 
        self.wire, self.rapids = PathGeom.wireForPath(obj.Base.Path)
        global currLocation
        firstmove = Path.Command("G0", {"X": 0, "Y": 0, "Z": 0})
        currLocation.update(firstmove.Parameters)
        newpath = []
        firstMayBeArc = True
        for curCommand in obj.Base.Path.Commands:
            # don't worry about non-move commands, just add to output
            if curCommand.Name not in movecommands + rapidcommands:
                newpath.append(curCommand)
                continue
            # rapid move straigt no action
            if curCommand.Name in rapidcommands:
                newpath.append(curCommand)
                currLocation.update(curCommand.Parameters)
                continue
            # FEED Move TAKE action
            if curCommand.Name in movecommands:
                if curCommand.z != currLocation["Z"]:#moviung DOWN 
                    if obj.HelixDown:
                        temp = self.getHelixDown(obj,curCommand)
                        newpath.extend(temp)
                    else:
                        newpath.append(curCommand)
                    currLocation.update(curCommand.Parameters)
                    continue
                elif curCommand.Name not in arccommands: # Straight Feedmove 
                    firstMayBeArc = False
                    temp = self.getTrochoidal(obj,curCommand)
                    newpath.extend(temp)
                else : # ARC Feedmove
                    if firstMayBeArc:
                        PathLog.error(translate("DO NOT START on ARC \n")) 
                    temp = self.getArcTrochoidal(obj,curCommand)
                    newpath.extend(temp)
            currLocation.update(curCommand.Parameters)
           
        commands = newpath
        obj.Path = Path.Path(commands)

    def getHelixDown(self, obj,curCommand):
        '''returns Helix Down G-code.'''
        global currLocation
        results = []
        op = PathDressup.baseOp(obj.Base)
        tc = PathDressup.toolController(obj.Base)
        horizFeed = tc.HorizFeed.Value
        vertFeed = tc.VertFeed.Value
        helixradius = op.OffsetExtra.Value - obj.MachiningAllowance.Value
        if obj.ClimbOrConventional == "Conventional":
            extendcommand = Path.Command('G1', {"X": curCommand.x-helixradius, "Y": curCommand.y, "Z": currLocation["Z"], "F": horizFeed})
            results.append(extendcommand)
            momentaryZ= currLocation["Z"]
            while momentaryZ-obj.StepWidthAlongPath.Value > curCommand.z:
                momentaryZ = momentaryZ-obj.StepWidthAlongPath.Value
                arcmove = Path.Command('G2', {"X": curCommand.x-helixradius, "Y": curCommand.y, "I": helixradius,"Z":momentaryZ, "F": vertFeed})  # add G2/G3 move
                results.append(arcmove)
            arcmove = Path.Command('G2', {"X": curCommand.x-helixradius, "Y": curCommand.y, "I": helixradius,"Z":curCommand.z, "F": vertFeed})  # add G2/G3 move
            results.append(arcmove) #full turn on bottem
        else: 
            extendcommand = Path.Command('G1', {"X": curCommand.x+helixradius, "Y": curCommand.y, "Z": currLocation["Z"], "F": horizFeed})
            results.append(extendcommand)
            momentaryZ= currLocation["Z"]
            while momentaryZ-obj.StepWidthAlongPath.Value > curCommand.z:
                momentaryZ = momentaryZ-obj.StepWidthAlongPath.Value
                arcmove = Path.Command('G3', {"X": curCommand.x+helixradius, "Y": curCommand.y, "I": helixradius*-1 ,"Z":momentaryZ, "F": vertFeed})  # add G2/G3 move
                results.append(arcmove)
            arcmove = Path.Command('G3', {"X": curCommand.x+helixradius, "Y": curCommand.y, "I": helixradius*-1,"Z":curCommand.z, "F": vertFeed})  # add G2/G3 move
            results.append(arcmove) #full turn on bottem
        movecenter = Path.Command('G1', {"X": currLocation["X"], "Y": currLocation["Y"], "Z": curCommand.z , "F": horizFeed})
        results.append(movecenter)
        return results 
    
    def togglePoints(self,vectorA,vectorB):
        return vectorB,vectorA

    def getArcTrochoidal(self,obj,curCommand):
        '''returns ARC Trochoidal Path G-code.'''
        global currLocation,test
        results = []
        op = PathDressup.baseOp(obj.Base)
        tc = PathDressup.toolController(obj.Base)
        horizFeed = tc.HorizFeed.Value
        helixradius = op.OffsetExtra.Value - obj.MachiningAllowance.Value
        #calculated all needed BASIC points from curCommand and last currLocation
        currendPos = FreeCAD.Vector(currLocation["X"],currLocation["Y"],currLocation["Z"])
        arcdir = 'CCW'
        if curCommand.Name == 'G2':
            arcdir= 'CW'
        p2 = curCommand.Placement.Base
        edge=PathGeom.edgeForCmd(curCommand,currendPos)
        arclength = edge.Length # length of the Circle Segment
        arccenter = FreeCAD.Vector(currendPos.x + curCommand.I, currendPos.y + curCommand.J, currendPos.z)
        radius = math.hypot(curCommand.I, curCommand.J)
        lineStartCenter= Part.makeLine(currendPos,arccenter)
        lineCenterEnd = Part.makeLine(arccenter,curCommand.Placement.Base)
        lineCenterStart = Part.makeLine(arccenter,currendPos)
        anglelineStartCenter = D.angle(arccenter.sub(currendPos), FreeCAD.Base.Vector(1, 0, 0), FreeCAD.Base.Vector(0, 0, -1))
        anglelineCenterEnd = D.angle(curCommand.Placement.Base.sub(arccenter), FreeCAD.Base.Vector(1, 0, 0), FreeCAD.Base.Vector(0, 0, -1))
        anglelineCenterStart = D.angle(currendPos.sub(arccenter), FreeCAD.Base.Vector(1, 0, 0), FreeCAD.Base.Vector(0, 0, -1))
        arcangle=PathGeom.diffAngle(anglelineCenterEnd,anglelineStartCenter,arcdir)
        stepsAlongCircle = int(abs(arclength/obj.StepWidthAlongPath.Value))#how many steps to the circle
        arcstepvalue = arcangle/stepsAlongCircle #radians
        angleLineCenterCurrentPos = D.angle(currendPos.sub(arccenter), FreeCAD.Base.Vector(1, 0, 0), FreeCAD.Base.Vector(0, 0, -1))
        if arcdir == 'CW':
            arcstepvalue = arcstepvalue*-1
        #lets make circles 
        for x in xrange(1,stepsAlongCircle+1):
            angleInCircle = angleLineCenterCurrentPos+arcstepvalue*x
            newCenterX = arccenter.x+(math.cos(angleInCircle)*radius)
            newCenterY = arccenter.y+(math.sin(angleInCircle)*radius)
            nextCenter = FreeCAD.Base.Vector(newCenterX,newCenterY,currLocation["Z"])
            v = nextCenter.sub(currendPos)
            segAngle = D.angle(v, FreeCAD.Base.Vector(1, 0, 0), FreeCAD.Base.Vector(0, 0, -1))
            xoffset = math.sin(segAngle) * helixradius #G
            yoffset = math.cos(segAngle) * helixradius #A
            offsetVectorA = FreeCAD.Vector(-xoffset,yoffset,0.0)
            offsetVectorB = FreeCAD.Vector(xoffset,-yoffset,0.0)
            arccommand = 'G2'
            pA = currendPos + offsetVectorA 
            pB = currendPos + offsetVectorB 
            if obj.ClimbOrConventional == "Climb":
                arccommand = 'G3'  
                pA,pB = self.togglePoints(pA,pB)
            IJ = currendPos - pA
            extendcommand = Path.Command('G1', {"X": pA.x, "Y": pA.y, "Z": curCommand.z, "F": horizFeed})
            results.append(extendcommand)
            extendcommand = Path.Command(arccommand, {"X": pB.x, "Y": pB.y,"I": IJ.x, "J": IJ.y, "F": horizFeed})
            results.append(extendcommand)
            extendcommand = Path.Command('G1', {"X": nextCenter.x, "Y": nextCenter.y, "Z": curCommand.z, "F": horizFeed})
            results.append(extendcommand)
            currendPos = nextCenter
        #move to center of Path for next  
        extendcommand = Path.Command('G1', {"X": curCommand.x, "Y": curCommand.y, "Z": curCommand.z, "F": horizFeed})
        results.append(extendcommand)
        return results          

    def getTrochoidal(self, obj,curCommand):
        '''returns Trochoidal Path G-code.'''
        global currLocation
        results = []
        op = PathDressup.baseOp(obj.Base)
        tc = PathDressup.toolController(obj.Base)
        horizFeed = tc.HorizFeed.Value
        helixradius = op.OffsetExtra.Value - obj.MachiningAllowance.Value
        #calulate points 
        p0 = FreeCAD.Vector(currLocation["X"],currLocation["Y"],currLocation["Z"])
        p1 = curCommand.Placement.Base
        line = Part.makeLine(p0,p1)
        lineLength = line.Length
        stepsAlongLine = int(abs(lineLength/obj.StepWidthAlongPath.Value)) 
        v = p1.sub(p0)
        segAngle = D.angle(v, FreeCAD.Base.Vector(1, 0, 0), FreeCAD.Base.Vector(0, 0, -1))
        xoffset = math.sin(segAngle) * helixradius #G
        yoffset = math.cos(segAngle) * helixradius #A
        stepoffsetX = math.sin(segAngle) * obj.StepWidthAlongPath.Value
        stepoffsetY = math.cos(segAngle) * obj.StepWidthAlongPath.Value
        currentPos = p0
        # Act on Direction not needed for straights only futher use 
        #if hasattr(op, 'Direction') :#and op.Direction == 'CW'
        offsetVectorA = FreeCAD.Vector(-xoffset,yoffset,0.0)
        offsetVectorB = FreeCAD.Vector(xoffset,-yoffset,0.0)
        stepVector = FreeCAD.Vector(stepoffsetY,stepoffsetX,0.0)
        arccommand = 'G2'
        for x in xrange(stepsAlongLine+1):
            oldpA = currentPos + offsetVectorA 
            oldpB = currentPos + offsetVectorB 
            if x == stepsAlongLine:
                currentPos = p1 # last Circle move to command Pos
            else:
                currentPos = currentPos + stepVector
            pA = currentPos + offsetVectorA 
            pB = currentPos + offsetVectorB 
            if obj.ClimbOrConventional == "Climb":
                arccommand = 'G3'  
                oldpA,oldpB = self.togglePoints(oldpA,oldpB)
                pA,pB = self.togglePoints(pA,pB)
            IJ = currentPos - pA
            extendcommand = Path.Command('G1', {"X": oldpA.x, "Y": oldpA.y, "Z": curCommand.z, "F": horizFeed})
            results.append(extendcommand)
            extendcommand = Path.Command('G1', {"X": pA.x, "Y": pA.y, "Z": curCommand.z, "F": horizFeed})
            results.append(extendcommand)
            extendcommand = Path.Command(arccommand, {"X": pB.x, "Y": pB.y,"I": IJ.x, "J": IJ.y, "F": horizFeed})
            results.append(extendcommand)
            extendcommand = Path.Command('G1', {"X": oldpB.x, "Y": oldpB.y, "Z": curCommand.z, "F": horizFeed})
            results.append(extendcommand)
        #move to center of Path for next  
        extendcommand = Path.Command('G1', {"X": currentPos.x, "Y": currentPos.y, "Z": curCommand.z, "F": horizFeed})
        results.append(extendcommand)
        return results

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


class CommandPathDressupTrochoidal:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_DressupTrochoidal", "Trochoidal Dressup"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_DressupTrochoidal", "Creates a Trochoidal Dressup object from a selected path")}

    def IsActive(self):
        op = PathDressup.selection()
        if op:
            return not PathDressup.hasEntryMethod(op)
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
        #force square joints
        if baseObject.JoinType == 'Round':
            baseObject.JoinType = "Square"
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("Create Trochoidal Dressup"))
        FreeCADGui.addModule("PathScripts.PathDressupTrochoidal")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "TrochoidalDressup")')
        FreeCADGui.doCommand('dbo = PathScripts.PathDressupTrochoidal.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('PathScripts.PathDressupTrochoidal.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False')
        FreeCADGui.doCommand('dbo.setup(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_DressupTrochoidal', CommandPathDressupTrochoidal())

PathLog.notice("Loading PathDressupLeadInOut... done\n")
