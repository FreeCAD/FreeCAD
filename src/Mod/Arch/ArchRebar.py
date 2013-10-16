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

import FreeCAD,FreeCADGui,Draft,ArchComponent,DraftVecUtils,ArchCommands
from FreeCAD import Vector
from PyQt4 import QtCore
from DraftTools import translate

__title__="FreeCAD Rebar"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

    
def makeRebar(baseobj,sketch,diameter=None,amount=1,offset=None,name="Rebar"):
    """makeRebar(baseobj,sketch,[diameter,amount,offset,name]): adds a Reinforcement Bar object
    to the given structural object, using the given sketch as profile."""
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Rebar(obj)
    _ViewProviderRebar(obj.ViewObject)
    if hasattr(sketch,"Support"):
        if sketch.Support:
            if isinstance(sketch.Support,tuple):
                if sketch.Support[0] == baseobj:
                    sketch.Support = None
            elif sketch.Support == baseobj:
                sketch.Support = None
    obj.Base = sketch
    sketch.ViewObject.hide()
    a = baseobj.Armatures
    a.append(obj)
    baseobj.Armatures = a
    if diameter:
        obj.Diameter = diameter
    else:
        obj.Diameter = p.GetFloat("RebarDiameter",6)
    obj.Amount = amount
    if offset:
        obj.OffsetStart = offset
        obj.OffsetEnd = offset
    else:
        obj.OffsetStart = p.GetFloat("RebarOffset",30)
        obj.OffsetEnd = p.GetFloat("RebarOffset",30)
    obj.ViewObject.ShapeColor = ArchCommands.getDefaultColor("Rebar")
    return obj


class _CommandRebar:
    "the Arch Rebar command definition"
    
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Rebar',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Rebar","Rebar"),
                'Accel': "R, B",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Rebar","Creates a Reinforcement bar from the selected face of a structural object")}

    def Activated(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            obj = sel[0].Object
            if Draft.getType(obj) == "Structure":
                if len(sel) > 1:
                    sk = sel[1].Object
                    if Draft.getType(sk) == "Sketch":
                        # we have a base object and a sketch: create the rebar now
                        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Rebar")))
                        FreeCADGui.doCommand("import Arch")
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
            elif Draft.getType(obj) == "Sketch":
                # we have only the sketch: extract the base object from it
                if hasattr(obj,"Support"):
                    if obj.Support:
                        if isinstance(obj.Support,tuple):
                            sup = obj.Support[0]
                        else:
                            sup = obj.Support
                        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Rebar")))
                        FreeCADGui.doCommand("import Arch")
                        FreeCADGui.doCommand("Arch.makeRebar(FreeCAD.ActiveDocument."+sup.Name+",FreeCAD.ActiveDocument."+obj.Name+")")
                        FreeCAD.ActiveDocument.commitTransaction()
                        FreeCAD.ActiveDocument.recompute()
                        return
                    else:
                        print "Arch: error: couldn't extract a base object"
                        return

        FreeCAD.Console.PrintMessage(str(translate("Arch","Please select a base face on a structural object\n")))
        FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
        FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Rebar")
        FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)


class _Rebar(ArchComponent.Component):
    "A parametric reinforcement bar (rebar) object"
    
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLength","Diameter","Arch","The diameter of the bar")
        obj.addProperty("App::PropertyLength","OffsetStart","Arch","The distance between the border of the beam and the fist bar (concrete cover).")
        obj.addProperty("App::PropertyLength","OffsetEnd","Arch","The distance between the border of the beam and the last bar (concrete cover).")
        obj.addProperty("App::PropertyInteger","Amount","Arch","The amount of bars")
        obj.addProperty("App::PropertyLength","Spacing","Arch","The spacing between the bars")
        obj.addProperty("App::PropertyVector","Direction","Arch","The direction to use to spread the bars. Keep (0,0,0) for automatic direction.")
        obj.addProperty("App::PropertyFloat","Rounding","Arch","The fillet to apply to the angle of the base profile. This value is multiplied by the bar diameter.")
        self.Type = "Component"
        obj.setEditorMode("Spacing",1)

    def getBaseAndAxis(self,obj):
        "returns a base point and orientation axis from the base sketch"
        if obj.Base:
            if obj.Base.Shape:
                if obj.Base.Shape.Wires:
                    e = obj.Base.Shape.Wires[0].Edges[0]
                    import DraftGeomUtils
                    v = DraftGeomUtils.vec(e).normalize()
                    return e.Vertexes[0].Point,v
        return None,None
        
    def execute(self,obj):
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
        if not obj.Diameter:
            return
        if not obj.Amount:
            return
        father = obj.InList[0]
        wire = obj.Base.Shape.Wires[0]
        if hasattr(obj,"Rounding"):
            print obj.Rounding
            if obj.Rounding:
                radius = obj.Rounding * obj.Diameter
                import DraftGeomUtils
                wire = DraftGeomUtils.filletWire(wire,radius)
        bpoint, bvec = self.getBaseAndAxis(obj)
        if not bpoint:
            return
        axis = obj.Base.Placement.Rotation.multVec(FreeCAD.Vector(0,0,-1))
        size = (ArchCommands.projectToVector(father.Shape.copy(),axis)).Length
        if hasattr(obj,"Direction"):
            if not DraftVecUtils.isNull(obj.Direction):
                axis = FreeCAD.Vector(obj.Direction) #.normalize()
                # don't normalize so the vector can also be used to determine the distance
                size = axis.Length
        #print axis
        #print size
        if (obj.OffsetStart+obj.OffsetEnd) > size:
            return

        # all tests ok!
        pl = obj.Placement
        import Part
        circle = Part.makeCircle(obj.Diameter/2,bpoint,bvec)
        circle = Part.Wire(circle)
        try:
            bar = wire.makePipeShell([circle],1,0,2)
        except:
            print "Arch: error sweeping rebar profile along the base sketch"
            return
        # building final shape
        shapes = []
        if obj.Amount == 1:
            offset = DraftVecUtils.scaleTo(axis,size/2)
            bar.translate(offset)
            shapes.append(bar)
            if hasattr(obj,"Spacing"):
                obj.Spacing = 0
        else:
            if obj.OffsetStart:
                baseoffset = DraftVecUtils.scaleTo(axis,obj.OffsetStart)
            else:
                baseoffset = None
            interval = size - (obj.OffsetStart + obj.OffsetEnd)
            interval = interval / (obj.Amount - 1)
            vinterval = DraftVecUtils.scaleTo(axis,interval)
            for i in range(obj.Amount):
                if i == 0:
                    if baseoffset:
                        bar.translate(baseoffset)
                    shapes.append(bar)
                else:
                    bar = bar.copy()
                    bar.translate(vinterval)
                    shapes.append(bar)
            if hasattr(obj,"Spacing"):
                obj.Spacing = interval
        if shapes:
            obj.Shape = Part.makeCompound(shapes)
            obj.Placement = pl

        
class _ViewProviderRebar(ArchComponent.ViewProviderComponent):
    "A View Provider for the Rebar object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Rebar_Tree.svg"


FreeCADGui.addCommand('Arch_Rebar',_CommandRebar())
