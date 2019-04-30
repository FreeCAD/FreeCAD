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
# Modified Amritpal Singh <amrit3701@gmail.com> on 07-07-2017

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
#  inside concrete structures to reinforce them.

__title__="FreeCAD Rebar"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


def makeRebar(baseobj=None,sketch=None,diameter=None,amount=1,offset=None,name="Rebar"):

    """makeRebar([baseobj,sketch,diameter,amount,offset,name]): adds a Reinforcement Bar object
    to the given structural object, using the given sketch as profile."""

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Rebar")
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
        obj.Host = baseobj
    elif sketch and not baseobj:
        # a rebar could be based on a wire without the existence of a Structure
        obj.Base = sketch
        if FreeCAD.GuiUp:
            sketch.ViewObject.hide()
        obj.Host = None
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
    return obj


class _CommandRebar:

    "the Arch Rebar command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Rebar',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Rebar","Custom Rebar"),
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
                            # we have a structure and a wire: create the rebar now
                            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Rebar"))
                            FreeCADGui.addModule("Arch")
                            FreeCADGui.doCommand("Arch.makeRebar(FreeCAD.ActiveDocument."+obj.Name+",FreeCAD.ActiveDocument."+sk.Name+")")
                            FreeCAD.ActiveDocument.commitTransaction()
                            FreeCAD.ActiveDocument.recompute()
                            return
                else:
                    # we have only a structure: open the sketcher
                    FreeCADGui.activateWorkbench("SketcherWorkbench")
                    FreeCADGui.runCommand("Sketcher_NewSketch")
                    FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(obj,FreeCAD.ActiveDocument.Objects[-1],hide=False,nextCommand="Arch_Rebar")
                    FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)
                    return
            elif obj.isDerivedFrom("Part::Feature"):
                if len(obj.Shape.Wires) == 1:
                    # we have only a wire: extract its support object, if available, and make the rebar
                    support = "None"
                    if hasattr(obj,"Support"):
                        if obj.Support:
                            if len(obj.Support) != 0:
                                support = "FreeCAD.ActiveDocument."+obj.Support[0][0].Name
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Rebar"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("Arch.makeRebar("+support+",FreeCAD.ActiveDocument."+obj.Name+")")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return

        FreeCAD.Console.PrintMessage(translate("Arch","Please select a base face on a structural object")+"\n")
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
        FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Rebar")
        FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)


class _Rebar(ArchComponent.Component):

    "A parametric reinforcement bar (rebar) object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Reinforcing Bar"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Diameter" in pl:
            obj.addProperty("App::PropertyLength","Diameter","Rebar",QT_TRANSLATE_NOOP("App::Property","The diameter of the bar"))
        if not "OffsetStart" in pl:
            obj.addProperty("App::PropertyLength","OffsetStart","Rebar",QT_TRANSLATE_NOOP("App::Property","The distance between the border of the beam and the first bar (concrete cover)."))
        if not "OffsetEnd" in pl:
            obj.addProperty("App::PropertyLength","OffsetEnd","Rebar",QT_TRANSLATE_NOOP("App::Property","The distance between the border of the beam and the last bar (concrete cover)."))
        if not "Amount" in pl:
            obj.addProperty("App::PropertyInteger","Amount","Rebar",QT_TRANSLATE_NOOP("App::Property","The amount of bars"))
        if not "Spacing" in pl:
            obj.addProperty("App::PropertyLength","Spacing","Rebar",QT_TRANSLATE_NOOP("App::Property","The spacing between the bars"))
            obj.setEditorMode("Spacing", 1)
        if not "Distance" in pl:
            obj.addProperty("App::PropertyLength","Distance","Rebar",QT_TRANSLATE_NOOP("App::Property","The total distance to span the rebars over. Keep 0 to automatically use the host shape size."))
        if not "Direction" in pl:
            obj.addProperty("App::PropertyVector","Direction","Rebar",QT_TRANSLATE_NOOP("App::Property","The direction to use to spread the bars. Keep (0,0,0) for automatic direction."))
        if not "Rounding" in pl:
            obj.addProperty("App::PropertyFloat","Rounding","Rebar",QT_TRANSLATE_NOOP("App::Property","The fillet to apply to the angle of the base profile. This value is multiplied by the bar diameter."))
        if not "PlacementList" in pl:
            obj.addProperty("App::PropertyPlacementList","PlacementList","Rebar",QT_TRANSLATE_NOOP("App::Property","List of placement of all the bars"))
        if not "Host" in pl:
            obj.addProperty("App::PropertyLink","Host","Rebar",QT_TRANSLATE_NOOP("App::Property","The structure object that hosts this rebar"))
        if not "CustomSpacing" in pl:
            obj.addProperty("App::PropertyString", "CustomSpacing", "Rebar", QT_TRANSLATE_NOOP("App::Property","The custom spacing of rebar"))
        if not "Length" in pl:
            obj.addProperty("App::PropertyDistance", "Length", "Rebar", QT_TRANSLATE_NOOP("App::Property","Length of a single rebar"))
            obj.setEditorMode("Length", 1)
        if not "TotalLength" in pl:
            obj.addProperty("App::PropertyDistance", "TotalLength", "Rebar", QT_TRANSLATE_NOOP("App::Property","Total length of all rebars"))
            obj.setEditorMode("TotalLength", 1)
        self.Type = "Rebar"

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def getBaseAndAxis(self,wire):

        "returns a base point and orientation axis from the base wire"
        import DraftGeomUtils
        if wire:
            e = wire.Edges[0]
            #v = DraftGeomUtils.vec(e).normalize()
            v = e.tangentAt(e.FirstParameter)
            return e.Vertexes[0].Point,v
        if obj.Base:
            if obj.Base.Shape:
                if obj.Base.Shape.Wires:
                    e = obj.Base.Shape.Wires[0].Edges[0]
                    #v = DraftGeomUtils.vec(e).normalize()
                    v = e.tangentAt(e.FirstParameter)
                    return e.Vertexes[0].Point,v
        return None,None

    def getRebarData(self,obj):

        if not obj.Host:
            return
        if Draft.getType(obj.Host) != "Structure":
            return
        if not obj.Host.Shape:
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
        father = obj.Host
        wire = obj.Base.Shape.Wires[0]
        if Draft.getType(obj.Base) == "Wire": # Draft Wires can have "wrong" placement
            import DraftGeomUtils
            axis = DraftGeomUtils.getNormal(obj.Base.Shape)
        else:
            axis = obj.Base.Placement.Rotation.multVec(FreeCAD.Vector(0,0,-1))
        size = (ArchCommands.projectToVector(father.Shape.copy(),axis)).Length
        if hasattr(obj,"Direction"):
            if not DraftVecUtils.isNull(obj.Direction):
                axis = FreeCAD.Vector(obj.Direction)
                axis.normalize()
        if hasattr(obj,"Distance"):
            if obj.Distance.Value:
                size = obj.Distance.Value
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

    def onChanged(self,obj,prop):

        if prop == "Host":
            if hasattr(obj,"Host"):
                if obj.Host:
                    # mark host to recompute so it can detect this object
                    obj.Host.touch()

    def execute(self,obj):

        if self.clone(obj):
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
        father = obj.Host
        fathershape = None
        if not father:
            # support for old-style rebars
            if obj.InList:
                if hasattr(obj.InList[0],"Armatures"):
                    if obj in obj.InList[0].Armatures:
                        father = obj.InList[0]
        if father:
            if father.isDerivedFrom("Part::Feature"):
                fathershape = father.Shape

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
        if fathershape:
            size = (ArchCommands.projectToVector(fathershape.copy(),axis)).Length
        else:
            size = 1
        if hasattr(obj,"Direction"):
            if not DraftVecUtils.isNull(obj.Direction):
                axis = FreeCAD.Vector(obj.Direction)
                axis.normalize()
                if fathershape:
                    size = (ArchCommands.projectToVector(fathershape.copy(),axis)).Length
                else:
                    size = 1
        if hasattr(obj,"Distance"):
            if obj.Distance.Value:
                size = obj.Distance.Value
        spacinglist = None
        if hasattr(obj, "CustomSpacing"):
            if obj.CustomSpacing:
                spacinglist = strprocessOfCustomSpacing(obj.CustomSpacing)
                influenceArea = sum(spacinglist) - spacinglist[0] / 2 - spacinglist[-1] / 2
        if (obj.OffsetStart.Value + obj.OffsetEnd.Value) > size:
            return
        # all tests ok!
        if hasattr(obj, "Length"):
            length = getLengthOfRebar(obj)
            if length:
                obj.Length = length
        pl = obj.Placement
        import Part
        circle = Part.makeCircle(obj.Diameter.Value/2,bpoint,bvec)
        circle = Part.Wire(circle)
        try:
            bar = wire.makePipeShell([circle],True,False,2)
            basewire = wire.copy()
        except Part.OCCError:
            print("Arch: error sweeping rebar profile along the base sketch")
            return
        # building final shape
        shapes = []
        placementlist = []
        self.wires = []
        rot = FreeCAD.Rotation()
        if obj.Amount == 1:
            barplacement = CalculatePlacement(obj.Amount, 1, size, axis, rot, obj.OffsetStart.Value, obj.OffsetEnd.Value)
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
                barplacement = CalculatePlacement(obj.Amount, i+1, size, axis, rot, obj.OffsetStart.Value, obj.OffsetEnd.Value)
                placementlist.append(barplacement)
            if hasattr(obj,"Spacing"):
                obj.Spacing = interval
        # Calculate placement of bars from custom spacing.
        if spacinglist:
            placementlist[:] = []
            reqInfluenceArea = size - (obj.OffsetStart.Value + obj.OffsetEnd.Value)
            # Avoid unnecessary checks to pass like. For eg.: when we have values
            # like influenceArea is 100.00001 and reqInflueneArea is 100
            if round(influenceArea) > round(reqInfluenceArea):
                FreeCAD.Console.PrintWarning("Influence area of rebars is greater than "+ str(reqInfluenceArea) + ".\n")
            elif round(influenceArea) < round(reqInfluenceArea):
                FreeCAD.Console.PrintWarning("Last span is greater that end offset.\n")
            for i in range(len(spacinglist)):
                if i == 0:
                    barplacement = CustomSpacingPlacement(spacinglist, 1, axis, father.Placement.Rotation, obj.OffsetStart.Value, obj.OffsetEnd.Value)
                    placementlist.append(barplacement)
                else:
                    barplacement = CustomSpacingPlacement(spacinglist, i+1, axis, father.Placement.Rotation, obj.OffsetStart.Value, obj.OffsetEnd.Value)
                    placementlist.append(barplacement)
            obj.Amount = len(spacinglist)
            obj.Spacing = 0
        obj.PlacementList = placementlist
        for i in range(len(obj.PlacementList)):
            if i == 0:
                bar.Placement = obj.PlacementList[i]
                shapes.append(bar)
                basewire.Placement = obj.PlacementList[i]
                self.wires.append(basewire)
            else:
                bar = bar.copy()
                bar.Placement = obj.PlacementList[i]
                shapes.append(bar)
                w = basewire.copy()
                w.Placement = obj.PlacementList[i]
                self.wires.append(w)
        if shapes:
            obj.Shape = Part.makeCompound(shapes)
            obj.Placement = pl
        obj.TotalLength = obj.Length * len(obj.PlacementList)

class _ViewProviderRebar(ArchComponent.ViewProviderComponent):

    "A View Provider for the Rebar object"

    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        self.setProperties(vobj)
        vobj.ShapeColor = ArchCommands.getDefaultColor("Rebar")

    def setProperties(self,vobj):

        pl = vobj.PropertiesList
        if not "RebarShape" in pl:
            vobj.addProperty("App::PropertyString","RebarShape","Rebar",QT_TRANSLATE_NOOP("App::Property","Shape of rebar")).RebarShape
            vobj.setEditorMode("RebarShape",2)

    def onDocumentRestored(self,vobj):

        self.setProperties(vobj)

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

    def updateData(self,obj,prop):

        if prop == "Shape":
            if hasattr(self,"centerline"):
                if self.centerline:
                    self.centerlinegroup.removeChild(self.centerline)
            if hasattr(obj.Proxy,"wires"):
                if obj.Proxy.wires:
                    from pivy import coin
                    import re,Part
                    self.centerline = coin.SoSeparator()
                    comp = Part.makeCompound(obj.Proxy.wires)
                    pts = re.findall("point \[(.*?)\]",comp.writeInventor().replace("\n",""))
                    pts = [p.split(",") for p in pts]
                    for pt in pts:
                        ps = coin.SoSeparator()
                        plist = []
                        for p in pt:
                            c = []
                            for pstr in p.split(" "):
                                if pstr:
                                    c.append(float(pstr))
                            plist.append(c)
                        coords = coin.SoCoordinate3()
                        coords.point.setValues(plist)
                        ps.addChild(coords)
                        ls = coin.SoLineSet()
                        ls.numVertices = -1
                        ps.addChild(ls)
                        self.centerline.addChild(ps)
                    self.centerlinegroup.addChild(self.centerline)
        ArchComponent.ViewProviderComponent.updateData(self,obj,prop)

    def attach(self,vobj):

        from pivy import coin
        self.centerlinegroup = coin.SoSeparator()
        self.centerlinegroup.setName("Centerline")
        self.centerlinecolor = coin.SoBaseColor()
        self.centerlinestyle = coin.SoDrawStyle()
        self.centerlinegroup.addChild(self.centerlinecolor)
        self.centerlinegroup.addChild(self.centerlinestyle)
        vobj.addDisplayMode(self.centerlinegroup,"Centerline")
        ArchComponent.ViewProviderComponent.attach(self,vobj)

    def onChanged(self,vobj,prop):

        if (prop == "LineColor") and hasattr(vobj,"LineColor"):
            if hasattr(self,"centerlinecolor"):
                c = vobj.LineColor
                self.centerlinecolor.rgb.setValue(c[0],c[1],c[2])
        elif (prop == "LineWidth") and hasattr(vobj,"LineWidth"):
            if hasattr(self,"centerlinestyle"):
                self.centerlinestyle.lineWidth = vobj.LineWidth
        ArchComponent.ViewProviderComponent.onChanged(self,vobj,prop)

    def getDisplayModes(self,vobj):

        modes=["Centerline"]
        return modes+ArchComponent.ViewProviderComponent.getDisplayModes(self,vobj)

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

def CustomSpacingPlacement(spacinglist, barnumber, axis, rotation, offsetstart, offsetend):

    """ CustomSpacingPlacement(spacinglist, barnumber, axis, rotation, offsetstart, offsetend):
    Calculate placement of the bar from custom spacing list."""
    if barnumber == 1:
        bardistance = offsetstart
    else:
        bardistance = sum(spacinglist[0:barnumber])
        bardistance = bardistance - spacinglist[0] / 2
        bardistance = bardistance - spacinglist[barnumber - 1] / 2
        bardistance = bardistance + offsetstart
    barplacement = DraftVecUtils.scaleTo(axis, bardistance)
    placement = FreeCAD.Placement(barplacement, rotation)
    return placement

def strprocessOfCustomSpacing(span_string):

    """ strprocessOfCustomSpacing(span_string): This function take input
    in specific syntax and return output in the form of list. For eg.
    Input: "3@100+2@200+3@100"
    Output: [100, 100, 100, 200, 200, 100, 100, 100]"""
    # import string
    span_st = span_string.strip()
    span_sp = span_st.split('+')
    index = 0
    spacinglist = []
    while index < len(span_sp):
        # Find "@" recursively in span_sp array.
        # If not found, append the index value to "spacinglist" array.
        if span_sp[index].find('@') == -1:
            spacinglist.append(float(span_sp[index]))
        else:
            in_sp = span_sp[index].split('@')
            count = 0
            while count < int(in_sp[0]):
                spacinglist.append(float(in_sp[1]))
                count += 1
        index += 1
    return spacinglist

def getLengthOfRebar(rebar):

    """ getLengthOfRebar(RebarObject): Calculates the length of the rebar."""
    base = rebar.Base
    # When rebar is derived from DWire
    if hasattr(base, "Length"):
        return base.Length
    # When rebar is derived from Sketch
    elif base.isDerivedFrom("Sketcher::SketchObject"):
        length = 0
        for geo in base.Geometry:
            length += geo.length()
        return length
    else:
        FreeCAD.Console.PrintError("Cannot calculate rebar length from its base object\n")
        return None

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Rebar',_CommandRebar())
