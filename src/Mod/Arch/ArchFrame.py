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
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Arch Frame"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

# Possible roles for frames
Roles = ['Covering','Member','Railing','Shading Device','Tendon']

def makeFrame(baseobj,profile,name=translate("Arch","Frame")):
    """makeFrame(baseobj,profile,[name]): creates a frame object from a base sketch (or any other object
    containing wires) and a profile object (an extrudable 2D object containing faces or closed wires)"""
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    obj.Label = translate("Arch",name)
    _Frame(obj)
    if FreeCAD.GuiUp:
        _ViewProviderFrame(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
    if profile:
        obj.Profile = profile
        if FreeCAD.GuiUp:
            profile.ViewObject.hide()
    return obj

class _CommandFrame:
    "the Arch Frame command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Arch_Frame',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Frame","Frame"),
                'Accel': "F, R",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Frame","Creates a frame object from a planar 2D object and a profile")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        s = FreeCADGui.Selection.getSelection()
        if len(s) == 2:
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Frame"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.makeFrame(FreeCAD.ActiveDocument."+s[0].Name+",FreeCAD.ActiveDocument."+s[1].Name+")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


class _Frame(ArchComponent.Component):
    "A parametric frame object"

    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLink","Profile","Arch","The profile used to build this frame")
        obj.addProperty("App::PropertyBool","Align","Arch","Specifies if the profile must be aligned with the extrusion wires")
        obj.addProperty("App::PropertyVector","Offset","Arch","An offset vector between the base sketch and the frame")
        obj.addProperty("App::PropertyInteger","BasePoint","Arch","Crossing point of the path on the profile.")
        obj.addProperty("App::PropertyAngle","Rotation","Arch","The rotation of the profile around its extrusion axis")
        self.Type = "Frame"
        obj.Role = Roles

    def execute(self,obj):
        if not obj.Base:
            return
        if not obj.Base.Shape:
            return
        if not obj.Base.Shape.Wires:
            return

        pl = obj.Placement
        if obj.Base.Shape.Solids:
            obj.Shape = obj.Base.Shape.copy()
            if not pl.isNull():
                obj.Placement = obj.Shape.Placement.multiply(pl)
        else:
            if not obj.Profile:
                return
            if not obj.Profile.isDerivedFrom("Part::Part2DObject"):
                return
            if not obj.Profile.Shape:
                return
            if not obj.Profile.Shape.Wires:
                return
            if not obj.Profile.Shape.Faces:
                for w in obj.Profile.Shape.Wires:
                    if not w.isClosed():
                        return
            import DraftGeomUtils, Part, math
            baseprofile = obj.Profile.Shape.copy()
            if not baseprofile.Faces:
                f = []
                for w in baseprofile.Wires:
                    f.append(Part.Face(w))
                if len(f) == 1:
                    baseprofile = f[0]
                else:
                    baseprofile = Part.makeCompound(f)
            shapes = []
            normal = DraftGeomUtils.getNormal(obj.Base.Shape)
            #for wire in obj.Base.Shape.Wires:
            for e in obj.Base.Shape.Edges:
                #e = wire.Edges[0]
                bvec = DraftGeomUtils.vec(e)
                bpoint = e.Vertexes[0].Point
                profile = baseprofile.copy()
                #basepoint = profile.Placement.Base
                if hasattr(obj,"BasePoint"):
                    edges = DraftGeomUtils.sortEdges(profile.Edges)
                    basepointliste = [profile.CenterOfMass]
                    for edge in edges:
                        basepointliste.append(DraftGeomUtils.findMidpoint(edge))
                        basepointliste.append(edge.Vertexes[-1].Point)
                    try:
                        basepoint = basepointliste[obj.BasePoint]
                    except IndexError:
                        FreeCAD.Console.PrintMessage(translate("Arch","Crossing point not found in profile.\n"))
                        basepoint = basepointliste[0]
                else :
                    basepoint = profile.CenterOfMass
                profile.translate(bpoint.sub(basepoint))
                if obj.Align:
                    axis = profile.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))
                    angle = bvec.getAngle(axis)
                    if round(angle,Draft.precision()) != 0:
                        if round(angle,Draft.precision()) != round(math.pi,Draft.precision()):
                            rotaxis = axis.cross(bvec)
                            profile.rotate(DraftVecUtils.tup(bpoint), DraftVecUtils.tup(rotaxis), math.degrees(angle))
                if obj.Rotation:
                    profile.rotate(DraftVecUtils.tup(bpoint), DraftVecUtils.tup(FreeCAD.Vector(bvec).normalize()), obj.Rotation)
                #profile = wire.makePipeShell([profile],True,False,2) TODO buggy
                profile = profile.extrude(bvec)
                if obj.Offset:
                    if not DraftVecUtils.isNull(obj.Offset):
                        profile.translate(obj.Offset)
                shapes.append(profile)
            if shapes:
                obj.Shape = Part.makeCompound(shapes)
                obj.Placement = pl


class _ViewProviderFrame(ArchComponent.ViewProviderComponent):
    "A View Provider for the Frame object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Frame_Tree.svg"

    def claimChildren(self):
        p = []
        if hasattr(self,"Object"):
            if self.Object.Profile:
                p = [self.Object.Profile]
        return ArchComponent.ViewProviderComponent.claimChildren(self)+p

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Frame',_CommandFrame())
