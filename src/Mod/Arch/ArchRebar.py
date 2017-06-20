#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013                                                    *
#*   Yorik van Havre <yorik@uncreated.net>                                 *
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

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
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

## @package ArchRebar
#  \ingroup ARCH
#  \brief The Rebar object and tools
#
#  This module provides tools to build Rebar objects.
#  Rebars (or Reinforcing Bars) are metallic bars placed
#  inside concrete strutures to reinforce them.

__title__="FreeCAD Rebar"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


def makeRebar(baseobj=None,sketch=None,diameter=None,amount=1,offset=None,name="Rebar"):
    """makeRebar([baseobj,sketch,diameter,amount,offset,name]): adds a Reinforcement Bar object
    to the given structural object, using the given sketch as profile."""
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    obj.Label = translate("Arch",name)
    _Rebar(obj)
    if FreeCAD.GuiUp:
        _ViewProviderRebar(obj.ViewObject)
    if baseobj and sketch:
        if hasattr(sketch,"Support"):
            if sketch.Support:
                if isinstance(sketch.Support,tuple):
                    if sketch.Support[0] == baseobj:
                        sketch.Support = None
                elif sketch.Support == baseobj:
                    sketch.Support = None
        obj.Base = sketch
        if FreeCAD.GuiUp:
            sketch.ViewObject.hide()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        if p.GetBool("archRemoveExternal",False):
            a = baseobj.Armatures
            a.append(obj)
            baseobj.Armatures = a
        else:
            import Arch
            host = getattr(Arch,"make"+Draft.getType(baseobj))(baseobj)
            a = host.Armatures
            a.append(obj)
            host.Armatures = a
    if diameter:
        obj.Diameter = diameter
    else:
        obj.Diameter = p.GetFloat("RebarDiameter",6)
    obj.Amount = amount
    obj.Document.recompute()
    if offset:
        obj.OffsetStart = offset
        obj.OffsetEnd = offset
    else:
        obj.OffsetStart = p.GetFloat("RebarOffset",30)
        obj.OffsetEnd = p.GetFloat("RebarOffset",30)
    ArchCommands.fixDAG(obj)
    return obj


class _CommandRebar:
    "the Arch Rebar command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_Rebar',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Rebar","Rebar"),
                'Accel': "R, B",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Rebar","Creates a Reinforcement bar from the selected face of a structural object")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            obj = sel[0].Object
            if Draft.getType(obj) == "Structure":
                if len(sel) > 1:
                    sk = sel[1].Object
                    if sk.isDerivedFrom("Part::Feature"):
                        if len(sk.Shape.Wires) == 1:
                            # we have a base object and a sketch: create the rebar now
                            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Rebar"))
                            FreeCADGui.addModule("Arch")
                            FreeCADGui.doCommand("Arch.makeRebar(FreeCAD.ActiveDocument."+obj.Name+",FreeCAD.ActiveDocument."+sk.Name+")")
                            FreeCAD.ActiveDocument.commitTransaction()
                            FreeCAD.ActiveDocument.recompute()
                            return
                else:
                    # we have only a base object: open the sketcher
                    FreeCADGui.activateWorkbench("SketcherWorkbench")
                    FreeCADGui.runCommand("Sketcher_NewSketch")
                    FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(obj,FreeCAD.ActiveDocument.Objects[-1],hide=False,nextCommand="Arch_Rebar")
                    FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)
                    return
            elif obj.isDerivedFrom("Part::Feature"):
                if len(obj.Shape.Wires) == 1:
                # we have only the sketch: extract the base object from it
                    if hasattr(obj,"Support"):
                        if obj.Support:
                            if len(obj.Support) != 0:
                                sup = obj.Support[0][0]
                            else:
                                print("Arch: error: couldn't extract a base object")
                                return
                            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Rebar"))
                            FreeCADGui.addModule("Arch")
                            FreeCADGui.doCommand("Arch.makeRebar(FreeCAD.ActiveDocument."+sup.Name+",FreeCAD.ActiveDocument."+obj.Name+")")
                            FreeCAD.ActiveDocument.commitTransaction()
                            FreeCAD.ActiveDocument.recompute()
                            return
                        else:
                            print("Arch: error: couldn't extract a base object")
                            return

        FreeCAD.Console.PrintMessage(translate("Arch","Please select a base face on a structural object\n"))
        FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
        FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Rebar")
        FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)


class _Rebar(ArchComponent.Component):
    "A parametric reinforcement bar (rebar) object"

    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLength","Diameter","Arch",QT_TRANSLATE_NOOP("App::Property","The diameter of the bar"))
        obj.addProperty("App::PropertyLength","OffsetStart","Arch",QT_TRANSLATE_NOOP("App::Property","The distance between the border of the beam and the fist bar (concrete cover)."))
        obj.addProperty("App::PropertyLength","OffsetEnd","Arch",QT_TRANSLATE_NOOP("App::Property","The distance between the border of the beam and the last bar (concrete cover)."))
        obj.addProperty("App::PropertyInteger","Amount","Arch",QT_TRANSLATE_NOOP("App::Property","The amount of bars"))
        obj.addProperty("App::PropertyLength","Spacing","Arch",QT_TRANSLATE_NOOP("App::Property","The spacing between the bars"))
        obj.addProperty("App::PropertyVector","Direction","Arch",QT_TRANSLATE_NOOP("App::Property","The direction to use to spread the bars. Keep (0,0,0) for automatic direction."))
        obj.addProperty("App::PropertyFloat","Rounding","Arch",QT_TRANSLATE_NOOP("App::Property","The fillet to apply to the angle of the base profile. This value is multiplied by the bar diameter."))
        obj.addProperty("App::PropertyPlacementList","PlacementList","Arch",QT_TRANSLATE_NOOP("App::Property","List of placement of all the bars"))
        self.Type = "Rebar"
        obj.setEditorMode("Spacing",1)

    def getBaseAndAxis(self,wire):
        "returns a base point and orientation axis from the base wire"
        import DraftGeomUtils
        if wire:
            e = wire.Edges[0]
            v = DraftGeomUtils.vec(e).normalize()
            return e.Vertexes[0].Point,v
        if obj.Base:
            if obj.Base.Shape:
                if obj.Base.Shape.Wires:
                    e = obj.Base.Shape.Wires[0].Edges[0]
                    v = DraftGeomUtils.vec(e).normalize()
                    return e.Vertexes[0].Point,v
        return None,None

    def getRebarData(self,obj):
        if len(obj.InList) != 1:
            return
        if Draft.getType(obj.InList[0]) != "Structure":
            return
        if not obj.InList[0].Shape:
            return
        if not obj.Base:
            return
        if not obj.Base.Shape:
            return
        if not obj.Base.Shape.Wires:
            return
        if not obj.Diameter.Value:
            return
        if not obj.Amount:
            return
        father = obj.InList[0]
        wire = obj.Base.Shape.Wires[0]
        axis = obj.Base.Placement.Rotation.multVec(FreeCAD.Vector(0,0,-1))
        size = (ArchCommands.projectToVector(father.Shape.copy(),axis)).Length
        if hasattr(obj,"Rounding"):
            if obj.Rounding:
                radius = obj.Rounding * obj.Diameter.Value
                import DraftGeomUtils
                wire = DraftGeomUtils.filletWire(wire,radius)
        wires = []
        if obj.Amount == 1:
            offset = DraftVecUtils.scaleTo(axis,size/2)
            wire.translate(offset)
            wires.append(wire)
        else:
            if obj.OffsetStart.Value:
                baseoffset = DraftVecUtils.scaleTo(axis,obj.OffsetStart.Value)
            else:
                baseoffset = None
            interval = size - (obj.OffsetStart.Value + obj.OffsetEnd.Value)
            interval = interval / (obj.Amount - 1)
            vinterval = DraftVecUtils.scaleTo(axis,interval)
            for i in range(obj.Amount):
                if i == 0:
                    if baseoffset:
                        wire.translate(baseoffset)
                    wires.append(wire)
                else:
                    wire = wire.copy()
                    wire.translate(vinterval)
                    wires.append(wire)
        return [wires,obj.Diameter.Value/2]

    def execute(self,obj):

        if self.clone(obj):
            return

        if len(obj.InList) != 1:
            return
        if Draft.getType(obj.InList[0]) != "Structure":
            return
        if not obj.InList[0].Shape:
            return
        if not obj.Base:
            return
        if not obj.Base.Shape:
            return
        if not obj.Base.Shape.Wires:
            return
        if not obj.Diameter.Value:
            return
        if not obj.Amount:
            return
        father = obj.InList[0]
        wire = obj.Base.Shape.Wires[0]
        if hasattr(obj,"Rounding"):
            #print(obj.Rounding)
            if obj.Rounding:
                radius = obj.Rounding * obj.Diameter.Value
                import DraftGeomUtils
                wire = DraftGeomUtils.filletWire(wire,radius)
        bpoint, bvec = self.getBaseAndAxis(wire)
        if not bpoint:
            return
        axis = obj.Base.Placement.Rotation.multVec(FreeCAD.Vector(0,0,-1))
        size = (ArchCommands.projectToVector(father.Shape.copy(),axis)).Length
        if hasattr(obj,"Direction"):
            if not DraftVecUtils.isNull(obj.Direction):
                axis = FreeCAD.Vector(obj.Direction) #.normalize()
                # don't normalize so the vector can also be used to determine the distance
                size = axis.Length
        #print(axis)
        #print(size)
        if (obj.OffsetStart.Value + obj.OffsetEnd.Value) > size:
            return

        # all tests ok!
        pl = obj.Placement
        import Part
        circle = Part.makeCircle(obj.Diameter.Value/2,bpoint,bvec)
        circle = Part.Wire(circle)
        try:
            bar = wire.makePipeShell([circle],True,False,2)
        except Part.OCCError:
            print("Arch: error sweeping rebar profile along the base sketch")
            return
        # building final shape
        shapes = []
        placementlist = []
        if obj.Amount == 1:
            barplacement = CalculatePlacement(obj.Amount, 1, size, axis, father.Placement.Rotation, obj.OffsetStart.Value, obj.OffsetEnd.Value)
            placementlist.append(barplacement)
            if hasattr(obj,"Spacing"):
                obj.Spacing = 0
        else:
            if obj.OffsetStart.Value:
                baseoffset = DraftVecUtils.scaleTo(axis,obj.OffsetStart.Value)
            else:
                baseoffset = None
            interval = size - (obj.OffsetStart.Value + obj.OffsetEnd.Value)
            interval = interval / (obj.Amount - 1)
            for i in range(obj.Amount):
                barplacement = CalculatePlacement(obj.Amount, i+1, size, axis, father.Placement.Rotation, obj.OffsetStart.Value, obj.OffsetEnd.Value)
                placementlist.append(barplacement)
            if hasattr(obj,"Spacing"):
                obj.Spacing = interval
        obj.PlacementList = placementlist
        for i in range(len(obj.PlacementList)):
            if i == 0:
                bar.Placement = obj.PlacementList[i]
                shapes.append(bar)
            else:
                bar = bar.copy()
                bar.Placement = obj.PlacementList[i]
                shapes.append(bar)
        if shapes:
            obj.Shape = Part.makeCompound(shapes)
            obj.Placement = pl

class _ViewProviderRebar(ArchComponent.ViewProviderComponent):
    "A View Provider for the Rebar object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        vobj.addProperty("App::PropertyString","RebarShape","Arch",QT_TRANSLATE_NOOP("App::Property","Shape of rebar")).RebarShape
        vobj.ShapeColor = ArchCommands.getDefaultColor("Rebar")
        vobj.setEditorMode("RebarShape",2)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Rebar_Tree.svg"

    def setEdit(self, vobj, mode):
        if mode == 0:
            if vobj.RebarShape:
                try:
                    # Import module of RebarShape
                    module = __import__(vobj.RebarShape)
                except ImportError:
                    FreeCAD.Console.PrintError("Unable to import RebarShape module\n")
                    return
                module.editDialog(vobj)

def CalculatePlacement(baramount, barnumber, size, axis, rotation, offsetstart, offsetend):
    """ CalculatePlacement([baramount, barnumber, size, axis, rotation, offsetstart, offsetend]):
    Calculate the placement of the bar from given values."""
    if baramount == 1:
        interval = offsetstart
    else:
        interval = size - (offsetstart + offsetend)
        interval = interval / (baramount - 1)
    bardistance = (interval * (barnumber - 1)) + offsetstart
    barplacement = DraftVecUtils.scaleTo(axis, bardistance)
    placement = FreeCAD.Placement(barplacement, rotation)
    return placement

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Rebar',_CommandRebar())
