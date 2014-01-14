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

__title__="FreeCAD Arch Stairs"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

import FreeCAD,FreeCADGui,ArchComponent,ArchCommands,Draft,DraftVecUtils,math
from FreeCAD import Vector
from DraftTools import translate
from PySide import QtCore


def makeStairs(base=None,length=4.5,width=1,height=3,steps=17,name=translate("Arch","Stairs")):
    """makeStairs([base,length,width,height,steps]): creates a Stairs
    objects with given attributes."""
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Stairs(obj)
    _ViewProviderStairs(obj.ViewObject)
    if base:
        obj.Base = base
    obj.Length = length
    obj.Width = width
    obj.Height = height
    obj.NumberOfSteps = steps


class _CommandStairs:
    "the Arch Stairs command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Stairs',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Stairs","Stairs"),
                'Accel': "S, R",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Space","Creates a stairs objects")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Stairs"))
        FreeCADGui.doCommand("import Arch")
        if len(FreeCADGui.Selection.getSelection()) == 1:
            n = FreeCADGui.Selection.getSelection()[0].Name
            FreeCADGui.doCommand("Arch.makeStairs(base=FreeCAD.ActiveDocument."+n+")")
        else:
            FreeCADGui.doCommand("Arch.makeStairs()")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _Stairs(ArchComponent.Component):
    "A stairs object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        
        # http://en.wikipedia.org/wiki/Stairs
        
        # base properties
        obj.addProperty("App::PropertyLength","Length","Arch",
                        translate("Arch","The length of these stairs, if no baseline is defined"))
        obj.addProperty("App::PropertyLength","Width","Arch",
                        translate("Arch","The width of these stairs"))
        obj.addProperty("App::PropertyLength","Height","Arch",
                        translate("Arch","The total height of these stairs"))
        obj.addProperty("App::PropertyEnumeration","Align","Arch",
                        translate("Arch","The alignment of these stairs on their baseline, if applicable"))
                        
        # steps properties
        obj.addProperty("App::PropertyInteger","NumberOfSteps","Steps",
                        translate("Arch","The number of risers in these stairs"))
        obj.addProperty("App::PropertyLength","TreadDepth","Steps",
                        translate("Arch","The depth of the treads of these stairs"))
        obj.addProperty("App::PropertyLength","RiserHeight","Steps",
                        translate("Arch","The height of the risers of these stairs"))
        obj.addProperty("App::PropertyLength","Nosing","Steps",
                        translate("Arch","The size of the nosing"))
        obj.addProperty("App::PropertyLength","TreadThickness","Steps",
                        translate("Arch","The thickness of the treads"))
                        
        # structural properties
        obj.addProperty("App::PropertyEnumeration","Landings","Structure",
                        translate("Arch","The type of landings of these stairs"))
        obj.addProperty("App::PropertyEnumeration","Winders","Structure",
                        translate("Arch","The type of winders in these stairs"))
        obj.addProperty("App::PropertyEnumeration","Structure","Structure",
                        translate("Arch","The type of structure of these stairs"))
        obj.addProperty("App::PropertyLength","StructureThickness","Structure",
                        translate("Arch","The thickness of the massive structure or of the stringers"))
        obj.addProperty("App::PropertyLength","StringerWidth","Structure",
                        translate("Arch","The width of the stringers"))
        obj.addProperty("App::PropertyLength","StringerOffset","Structure",
                        translate("Arch","The offset between the border of the stairs and the stringers"))
                        
        obj.Align = ['Left','Right','Center']
        obj.Landings = ["None","At center","At each corner"]
        obj.Winders = ["None","All","Corners strict","Corners relaxed"]
        obj.Structure = ["None","Massive","One stringer","Two stringers"]
        obj.setEditorMode("TreadDepth",1)
        obj.setEditorMode("RiserHeight",1)
        self.Type = "Stairs"


    def execute(self,obj):

        "constructs the shape of the stairs"

        import Part
        self.steps = []
        self.structures = []
        pl = obj.Placement

        # base tests
        if not obj.Width:
            return
        if not obj.Height:
            if not obj.Base:
                return
        if obj.NumberOfSteps < 2:
            return
        if obj.Base:
            if not obj.Base.isDerivedFrom("Part::Feature"):
                return
            if obj.Base.Shape.Solids:
                obj.Shape = obj.Base.Shape.copy()
                obj.Placement = FreeCAD.Placement(obj.Base.Placement).multiply(pl)
                obj.TreadDepth = 0.0
                obj.RiserHeight = 0.0
                return
            if not obj.Base.Shape.Edges:
                return
            if obj.Base.Shape.Faces:
                return
            if (len(obj.Base.Shape.Edges) == 1): 
                edge = obj.Base.Shape.Edges[0]
                if isinstance(edge.Curve,Part.Line):
                    if obj.Landings == "At center":
                        self.makeStraightStairsWithLanding(obj,edge)                
                    else:
                        self.makeStraightStairs(obj,edge)
                else:
                    if obj.Landings == "At center":
                        self.makeCurvedStairsWithLandings(obj,edge)
                    else:
                        self.makeCurvedStairs(obj,edge)
        else:
            if not obj.Length:
                return
            edge = Part.Line(Vector(0,0,0),Vector(obj.Length,0,0)).toShape()
            if obj.Landings == "At center":
                self.makeStraightStairsWithLanding(obj,edge)
            else:
                self.makeStraightStairs(obj,edge)

        if self.structures or self.steps:
            shape = Part.makeCompound(self.structures + self.steps)
            shape = self.processSubShapes(obj,shape,pl)
            obj.Shape = shape
            obj.Placement = pl

        # compute step data
        if obj.NumberOfSteps > 1:
            l = obj.Length
            h = obj.Height
            if obj.Base:
                if obj.Base.isDerivedFrom("Part::Feature"):
                    l = obj.Base.Shape.Length
                    if obj.Base.Shape.BoundBox.ZLength:
                        h = obj.Base.Shape.BoundBox.ZLength
            obj.TreadDepth = float(l)/(obj.NumberOfSteps-1)
            obj.RiserHeight = float(h)/obj.NumberOfSteps


    def align(self,basepoint,align,widthvec):
        "moves a given basepoint according to the alignment"
        if align == "Center":
            basepoint = basepoint.add(DraftVecUtils.scale(widthvec,-0.5))
        elif align == "Right":
            basepoint = basepoint.add(DraftVecUtils.scale(widthvec,-1))
        return basepoint


    def makeStraightStairs(self,obj,edge):

        "builds a simple, straight staircase from a straight edge"

        # general data
        import Part,DraftGeomUtils
        v = DraftGeomUtils.vec(edge)
        vLength = DraftVecUtils.scaleTo(v,float(edge.Length)/(obj.NumberOfSteps-1))
        vLength = Vector(vLength.x,vLength.y,0)
        if round(v.z,Draft.precision()) != 0:
            h = v.z
        else:
            h = obj.Height
        vHeight = Vector(0,0,float(h)/obj.NumberOfSteps)
        vWidth = DraftVecUtils.scaleTo(vLength.cross(Vector(0,0,1)),obj.Width)
        vBase = edge.Vertexes[0].Point
        vNose = DraftVecUtils.scaleTo(vLength,-abs(obj.Nosing))

        # steps
        for i in range(obj.NumberOfSteps-1):
            p1 = vBase.add((Vector(vLength).multiply(i)).add(Vector(vHeight).multiply(i+1)))
            p1 = self.align(p1,obj.Align,vWidth)
            p1 = p1.add(vNose).add(Vector(0,0,-abs(obj.TreadThickness)))
            p2 = p1.add(DraftVecUtils.neg(vNose)).add(vLength)
            p3 = p2.add(vWidth)
            p4 = p3.add(DraftVecUtils.neg(vLength)).add(vNose)
            step = Part.Face(Part.makePolygon([p1,p2,p3,p4,p1]))
            if obj.TreadThickness:
                step = step.extrude(Vector(0,0,abs(obj.TreadThickness)))
            self.steps.append(step)

        # structure
        lProfile = []
        struct = None
        if obj.Structure == "Massive":
            if obj.StructureThickness:
                for i in range(obj.NumberOfSteps-1):
                    if not lProfile:
                        lProfile.append(vBase)
                    last = lProfile[-1]
                    if len(lProfile) == 1:
                        last = last.add(Vector(0,0,-abs(obj.TreadThickness)))
                    lProfile.append(last.add(vHeight))
                    lProfile.append(lProfile[-1].add(vLength))
                resHeight1 = obj.StructureThickness*((math.sqrt(vHeight.Length**2 + vLength.Length**2))/vLength.Length)
                lProfile.append(lProfile[-1].add(Vector(0,0,-resHeight1+abs(obj.TreadThickness))))
                resHeight2 = ((obj.NumberOfSteps-1)*vHeight.Length)-resHeight1
                resLength = (vLength.Length/vHeight.Length)*resHeight2
                h = DraftVecUtils.scaleTo(vLength,-resLength)
                lProfile.append(lProfile[-1].add(Vector(h.x,h.y,-resHeight2)))
                lProfile.append(vBase)
                #print lProfile
                pol = Part.makePolygon(lProfile)
                struct = Part.Face(pol)
                struct = struct.extrude(vWidth)
        elif obj.Structure in ["One stringer","Two stringers"]:
            if obj.StringerWidth and obj.StructureThickness:
                hyp = math.sqrt(vHeight.Length**2 + vLength.Length**2)
                l1 = Vector(vLength).multiply(obj.NumberOfSteps-1)
                h1 = Vector(vHeight).multiply(obj.NumberOfSteps-1).add(Vector(0,0,-abs(obj.TreadThickness)))
                lProfile.append(vBase.add(l1).add(h1))
                h2 = (obj.StructureThickness/vLength.Length)*hyp
                lProfile.append(lProfile[-1].add(Vector(0,0,-abs(h2))))
                h3 = lProfile[-1].z-vBase.z
                l3 = (h3/vHeight.Length)*vLength.Length
                v3 = DraftVecUtils.scaleTo(vLength,-l3)
                lProfile.append(lProfile[-1].add(Vector(0,0,-abs(h3))).add(v3))
                l4 = (obj.StructureThickness/vHeight.Length)*hyp
                v4 = DraftVecUtils.scaleTo(vLength,-l4)
                lProfile.append(lProfile[-1].add(v4))
                lProfile.append(lProfile[0])
                print lProfile
                pol = Part.makePolygon(lProfile)
                pol = Part.Face(pol)
                evec = DraftVecUtils.scaleTo(vWidth,obj.StringerWidth)
                if obj.Structure == "One stringer":
                    if obj.StringerOffset:
                        mvec = DraftVecUtils.scaleTo(vWidth,obj.StringerOffset)
                    else:
                        mvec = DraftVecUtils.scaleTo(vWidth,(vWidth.Length/2)-obj.StringerWidth/2)
                    pol.translate(mvec)
                    struct = pol.extrude(evec)
                elif obj.Structure == "Two stringers":
                    pol2 = pol.copy()
                    if obj.StringerOffset:
                        mvec = DraftVecUtils.scaleTo(vWidth,obj.StringerOffset)
                        pol.translate(mvec)
                        mvec = vWidth.add(mvec.negative())
                        pol2.translate(mvec)
                    else:
                        pol2.translate(vWidth)
                    s1 = pol.extrude(evec)
                    s2 = pol2.extrude(evec.negative())
                    struct = Part.makeCompound([s1,s2])
        if struct:
            self.structures.append(struct)


    def makeStraightStairsWithLanding(self,obj,edge):
        print "Not yet implemented!"

    def makeCurvedStairs(self,obj,edge):
        print "Not yet implemented!"

    def makeCurvedStairsWithLanding(self,obj,edge):
        print "Not yet implemented!"



class _ViewProviderStairs(ArchComponent.ViewProviderComponent):
    "A View Provider for Stairs"
    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        
    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Stairs_Tree.svg"


FreeCADGui.addCommand('Arch_Stairs',_CommandStairs())
