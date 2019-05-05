# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016 - Yorik van Havre <yorik@uncreated.net>            *
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

import FreeCAD, ArchComponent
if FreeCAD.GuiUp:
    import FreeCADGui, Arch_rc, os
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchPipe
#  \ingroup ARCH
#  \brief The Pipe object and tools
#
#  This module provides tools to build Pipe and Pipe connector objects.
#  Pipes are tubular objects extruded along a base line.

__title__ = "Arch Pipe tools"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


def makePipe(baseobj=None,diameter=0,length=0,placement=None,name="Pipe"):

    "makePipe([baseobj,diamerter,length,placement,name]): creates an pipe object from the given base object"

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj= FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    obj.Label = name
    _ArchPipe(obj)
    if FreeCAD.GuiUp:
        _ViewProviderPipe(obj.ViewObject)
        if baseobj:
            baseobj.ViewObject.hide()
    if baseobj:
        obj.Base = baseobj
    else:
        if length:
            obj.Length = length
        else:
            obj.Length = 1000
    if diameter:
        obj.Diameter = diameter
    else:
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        obj.Diameter = p.GetFloat("PipeDiameter",50)
    if placement:
        obj.Placement = placement
    return obj


def makePipeConnector(pipes,radius=0,name="Connector"):

    "makePipeConnector(pipes,[radius,name]): creates a connector between the given pipes"

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj= FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    obj.Label = name
    _ArchPipeConnector(obj)
    obj.Pipes = pipes
    if not radius:
        radius = pipes[0].Diameter
    obj.Radius = radius
    if FreeCAD.GuiUp:
        _ViewProviderPipe(obj.ViewObject)
    return obj


class _CommandPipe:


    "the Arch Pipe command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Pipe',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Pipe","Pipe"),
                'Accel': "P, I",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Pipe","Creates a pipe object from a given Wire or Line")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        s = FreeCADGui.Selection.getSelection()
        if s:
            for obj in s:
                if obj.isDerivedFrom("Part::Feature"):
                    if len(obj.Shape.Wires) == 1:
                        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Pipe"))
                        FreeCADGui.addModule("Arch")
                        FreeCADGui.doCommand("obj = Arch.makePipe(FreeCAD.ActiveDocument."+obj.Name+")")
                        FreeCADGui.addModule("Draft")
                        FreeCADGui.doCommand("Draft.autogroup(obj)")
                        FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Pipe"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("obj = Arch.makePipe()")
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _CommandPipeConnector:


    "the Arch Pipe command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_PipeConnector',
                'MenuText': QT_TRANSLATE_NOOP("Arch_PipeConnector","Connector"),
                'Accel': "P, C",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Pipe","Creates a connector between 2 or 3 selected pipes")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        import Draft
        s = FreeCADGui.Selection.getSelection()
        if not (len(s) in [2,3]):
            FreeCAD.Console.PrintError(translate("Arch","Please select exactly 2 or 3 Pipe objects")+"\n")
            return
        o = "["
        for obj in s:
            if Draft.getType(obj) != "Pipe":
                FreeCAD.Console.PrintError(translate("Arch","Please select only Pipe objects")+"\n")
                return
            o += "FreeCAD.ActiveDocument."+obj.Name+","
        o += "]"
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Connector"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("obj = Arch.makePipeConnector("+o+")")
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("Draft.autogroup(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _ArchPipe(ArchComponent.Component):


    "the Arch Pipe object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Pipe Segment"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Diameter" in pl:
            obj.addProperty("App::PropertyLength", "Diameter",    "Pipe", QT_TRANSLATE_NOOP("App::Property","The diameter of this pipe, if not based on a profile"))
        if not "Length" in pl:
            obj.addProperty("App::PropertyLength", "Length",      "Pipe", QT_TRANSLATE_NOOP("App::Property","The length of this pipe, if not based on an edge"))
        if not "Profile" in pl:
            obj.addProperty("App::PropertyLink",   "Profile",     "Pipe", QT_TRANSLATE_NOOP("App::Property","An optional closed profile to base this pipe on"))
        if not "OffsetStart" in pl:
            obj.addProperty("App::PropertyLength", "OffsetStart", "Pipe", QT_TRANSLATE_NOOP("App::Property","Offset from the start point"))
        if not "OffsetEnd" in pl:
            obj.addProperty("App::PropertyLength", "OffsetEnd",   "Pipe", QT_TRANSLATE_NOOP("App::Property","Offset from the end point"))
        self.Type = "Pipe"

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):

        import Part,DraftGeomUtils,math
        pl = obj.Placement
        w = self.getWire(obj)
        if not w:
            FreeCAD.Console.PrintError(translate("Arch","Unable to build the base path")+"\n")
            return
        if obj.OffsetStart.Value:
            e = w.Edges[0]
            v = e.Vertexes[-1].Point.sub(e.Vertexes[0].Point).normalize()
            v.multiply(obj.OffsetStart.Value)
            e = Part.LineSegment(e.Vertexes[0].Point.add(v),e.Vertexes[-1].Point).toShape()
            w = Part.Wire([e]+w.Edges[1:])
        if obj.OffsetEnd.Value:
            e = w.Edges[-1]
            v = e.Vertexes[0].Point.sub(e.Vertexes[-1].Point).normalize()
            v.multiply(obj.OffsetEnd.Value)
            e = Part.LineSegment(e.Vertexes[-1].Point.add(v),e.Vertexes[0].Point).toShape()
            w = Part.Wire(w.Edges[:-1]+[e])
        p = self.getProfile(obj)
        if not p:
            FreeCAD.Console.PrintError(translate("Arch","Unable to build the profile")+"\n")
            return
        # move and rotate the profile to the first point
        delta = w.Vertexes[0].Point-p.CenterOfMass
        p.translate(delta)
        v1 = w.Vertexes[1].Point-w.Vertexes[0].Point
        v2 = DraftGeomUtils.getNormal(p)
        rot = FreeCAD.Rotation(v2,v1)
        p.rotate(p.CenterOfMass,rot.Axis,math.degrees(rot.Angle))
        try:
            sh = w.makePipeShell([p],True,False,2)
        except:
            FreeCAD.Console.PrintError(translate("Arch","Unable to build the pipe")+"\n")
        else:
            obj.Shape = sh
            if obj.Base:
                obj.Length = w.Length
            else:
                obj.Placement = pl

    def getWire(self,obj):

        import Part
        if obj.Base:
            if not obj.Base.isDerivedFrom("Part::Feature"):
                FreeCAD.Console.PrintError(translate("Arch","The base object is not a Part")+"\n")
                return
            if len(obj.Base.Shape.Wires) != 1:
                FreeCAD.Console.PrintError(translate("Arch","Too many wires in the base shape")+"\n")
                return
            if obj.Base.Shape.Wires[0].isClosed():
                FreeCAD.Console.PrintError(translate("Arch","The base wire is closed")+"\n")
                return
            w = obj.Base.Shape.Wires[0]
        else:
            if obj.Length.Value == 0:
                return
            w = Part.Wire([Part.LineSegment(FreeCAD.Vector(0,0,0),FreeCAD.Vector(0,0,obj.Length.Value)).toShape()])
        return w

    def getProfile(self,obj):

        import Part
        if obj.Profile:
            if not obj.Profile.isDerivedFrom("Part::Part2DObject"):
                FreeCAD.Console.PrintError(translate("Arch","The profile is not a 2D Part")+"\n")
                return
            if len(obj.Profile.Shape.Wires) != 1:
                FreeCAD.Console.PrintError(translate("Arch","Too many wires in the profile")+"\n")
                return
            if not obj.Profile.Shape.Wires[0].isClosed():
                FreeCAD.Console.PrintError(translate("Arch","The profile is not closed")+"\n")
                return
            p = obj.Profile.Shape.Wires[0]
        else:
            if obj.Diameter.Value == 0:
                return
            p = Part.Wire([Part.Circle(FreeCAD.Vector(0,0,0),FreeCAD.Vector(0,0,1),obj.Diameter.Value/2).toShape()])
        return p


class _ViewProviderPipe(ArchComponent.ViewProviderComponent):


    "A View Provider for the Pipe object"

    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Pipe_Tree.svg"


class _ArchPipeConnector(ArchComponent.Component):


    "the Arch Pipe Connector object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Pipe Fitting"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Radius" in pl:
            obj.addProperty("App::PropertyLength",      "Radius",        "PipeConnector", QT_TRANSLATE_NOOP("App::Property","The curvature radius of this connector"))
        if not "Pipes" in pl:
            obj.addProperty("App::PropertyLinkList",    "Pipes",         "PipeConnector", QT_TRANSLATE_NOOP("App::Property","The pipes linked by this connector"))
        if not "ConnectorType" in pl:
            obj.addProperty("App::PropertyEnumeration", "ConnectorType", "PipeConnector", QT_TRANSLATE_NOOP("App::Property","The type of this connector"))
            obj.ConnectorType = ["Corner","Tee"]
            obj.setEditorMode("ConnectorType",1)
        self.Type = "PipeConnector"

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):

        tol = 1 # tolerance for alignment. This is only visual, we can keep it low...
        ptol = 0.001 # tolerance for coincident points

        import math,Part,DraftGeomUtils,ArchCommands
        if len(obj.Pipes) < 2:
            return
        if len(obj.Pipes) > 3:
            FreeCAD.Console.PrintWarning(translate("Arch","Only the 3 first wires will be connected")+"\n")
        if obj.Radius.Value == 0:
            return
        wires = []
        order = []
        for o in obj.Pipes:
            wires.append(o.Proxy.getWire(o))
        if wires[0].Vertexes[0].Point.sub(wires[1].Vertexes[0].Point).Length <= ptol:
            order = ["start","start"]
            point = wires[0].Vertexes[0].Point
        elif wires[0].Vertexes[0].Point.sub(wires[1].Vertexes[-1].Point).Length <= ptol:
            order = ["start","end"]
            point = wires[0].Vertexes[0].Point
        elif wires[0].Vertexes[-1].Point.sub(wires[1].Vertexes[-1].Point).Length <= ptol:
            order = ["end","end"]
            point = wires[0].Vertexes[-1].Point
        elif wires[0].Vertexes[-1].Point.sub(wires[1].Vertexes[0].Point).Length <= ptol:
            order = ["end","start"]
            point = wires[0].Vertexes[-1].Point
        else:
            FreeCAD.Console.PrintError(translate("Arch","Common vertex not found")+"\n")
            return
        if order[0] == "start":
            v1 = wires[0].Vertexes[1].Point.sub(wires[0].Vertexes[0].Point).normalize()
        else:
            v1 = wires[0].Vertexes[-2].Point.sub(wires[0].Vertexes[-1].Point).normalize()
        if order[1] == "start":
            v2 = wires[1].Vertexes[1].Point.sub(wires[1].Vertexes[0].Point).normalize()
        else:
            v2 = wires[1].Vertexes[-2].Point.sub(wires[1].Vertexes[-1].Point).normalize()
        p = obj.Pipes[0].Proxy.getProfile(obj.Pipes[0])
        p = Part.Face(p)
        if len(obj.Pipes) == 2:
            if obj.ConnectorType != "Corner":
                obj.ConnectorType = "Corner"
            if round(v1.getAngle(v2),tol) in [0,round(math.pi,tol)]:
                FreeCAD.Console.PrintError(translate("Arch","Pipes are already aligned")+"\n")
                return
            normal = v2.cross(v1)
            offset = math.tan(math.pi/2-v1.getAngle(v2)/2)*obj.Radius.Value
            v1.multiply(offset)
            v2.multiply(offset)
            self.setOffset(obj.Pipes[0],order[0],offset)
            self.setOffset(obj.Pipes[1],order[1],offset)
            # find center
            perp = v1.cross(normal).normalize()
            perp.multiply(obj.Radius.Value)
            center = point.add(v1).add(perp)
            # move and rotate the profile to the first point
            delta = point.add(v1)-p.CenterOfMass
            p.translate(delta)
            vp = DraftGeomUtils.getNormal(p)
            rot = FreeCAD.Rotation(vp,v1)
            p.rotate(p.CenterOfMass,rot.Axis,math.degrees(rot.Angle))
            sh = p.revolve(center,normal,math.degrees(math.pi-v1.getAngle(v2)))
            #sh = Part.makeCompound([sh]+[Part.Vertex(point),Part.Vertex(point.add(v1)),Part.Vertex(center),Part.Vertex(point.add(v2))])
        else:
            if obj.ConnectorType != "Tee":
                obj.ConnectorType = "Tee"
            if wires[2].Vertexes[0].Point == point:
                order.append("start")
            elif wires[0].Vertexes[-1].Point == point:
                order.append("end")
            else:
                FreeCAD.Console.PrintError(translate("Arch","Common vertex not found")+"\n")
            if order[2] == "start":
                v3 = wires[2].Vertexes[1].Point.sub(wires[2].Vertexes[0].Point).normalize()
            else:
                v3 = wires[2].Vertexes[-2].Point.sub(wires[2].Vertexes[-1].Point).normalize()
            if round(v1.getAngle(v2),tol) in [0,round(math.pi,tol)]:
                pair = [v1,v2,v3]
            elif round(v1.getAngle(v3),tol) in [0,round(math.pi,tol)]:
                pair = [v1,v3,v2]
            elif round(v2.getAngle(v3),tol) in [0,round(math.pi,tol)]:
                pair = [v2,v3,v1]
            else:
                FreeCAD.Console.PrintError(translate("Arch","At least 2 pipes must align")+"\n")
                return
            offset = obj.Radius.Value
            v1.multiply(offset)
            v2.multiply(offset)
            v3.multiply(offset)
            self.setOffset(obj.Pipes[0],order[0],offset)
            self.setOffset(obj.Pipes[1],order[1],offset)
            self.setOffset(obj.Pipes[2],order[2],offset)
            normal = pair[0].cross(pair[2])
            # move and rotate the profile to the first point
            delta = point.add(pair[0])-p.CenterOfMass
            p.translate(delta)
            vp = DraftGeomUtils.getNormal(p)
            rot = FreeCAD.Rotation(vp,pair[0])
            p.rotate(p.CenterOfMass,rot.Axis,math.degrees(rot.Angle))
            t1 = p.extrude(pair[1].multiply(2))
            # move and rotate the profile to the second point
            delta = point.add(pair[2])-p.CenterOfMass
            p.translate(delta)
            vp = DraftGeomUtils.getNormal(p)
            rot = FreeCAD.Rotation(vp,pair[2])
            p.rotate(p.CenterOfMass,rot.Axis,math.degrees(rot.Angle))
            t2 = p.extrude(pair[2].negative().multiply(2))
            # create a cut plane
            cp = Part.makePolygon([point,point.add(pair[0]),point.add(normal),point])
            cp = Part.Face(cp)
            if cp.normalAt(0,0).getAngle(pair[2]) < math.pi/2:
                cp.reverse()
            cf, cv, invcv = ArchCommands.getCutVolume(cp,t2)
            t2 = t2.cut(cv)
            sh = t1.fuse(t2)
        obj.Shape = sh

    def setOffset(self,pipe,pos,offset):

        if pos == "start":
            if pipe.OffsetStart != offset:
                pipe.OffsetStart = offset
                pipe.Proxy.execute(pipe)
        else:
            if pipe.OffsetEnd != offset:
                pipe.OffsetEnd = offset
                pipe.Proxy.execute(pipe)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Pipe',_CommandPipe())
    FreeCADGui.addCommand('Arch_PipeConnector',_CommandPipeConnector())

    class _ArchPipeGroupCommand:

        def GetCommands(self):
            return tuple(['Arch_Pipe','Arch_PipeConnector'])
        def GetResources(self):
            return { 'MenuText': QT_TRANSLATE_NOOP("Arch_PipeTools",'Pipe tools'),
                     'ToolTip': QT_TRANSLATE_NOOP("Arch_PipeTools",'Pipe tools')
                   }
        def IsActive(self):
            return not FreeCAD.ActiveDocument is None

    FreeCADGui.addCommand('Arch_PipeTools', _ArchPipeGroupCommand())
