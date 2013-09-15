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
from PyQt4 import QtCore


def makeStairs(base=None,length=4.5,width=1,height=3,steps=17):
    """makeStairs([base,length,width,height,steps]): creates a Stairs
    objects with given attributes."""
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Stairs")
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
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Stairs")))
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
                        str(translate("Arch","The length of these stairs, if no baseline is defined")))
        obj.addProperty("App::PropertyLength","Width","Arch",
                        str(translate("Arch","The width of these stairs")))
        obj.addProperty("App::PropertyLength","Height","Arch",
                        str(translate("Arch","The total height of these stairs")))
        obj.addProperty("App::PropertyEnumeration","Align","Arch",
                        str(translate("Arch","The alignment of these stairs on their baseline, if applicable")))
                        
        # steps properties
        obj.addProperty("App::PropertyInteger","NumberOfSteps","Steps",
                        str(translate("Arch","The number of risers in these stairs")))
        obj.addProperty("App::PropertyLength","TreadDepth","Steps",
                        str(translate("Arch","The depth of the treads of these stairs")))
        obj.addProperty("App::PropertyLength","RiserHeight","Steps",
                        str(translate("Arch","The height of the risers of these stairs")))
        obj.addProperty("App::PropertyLength","Nosing","Steps",
                        str(translate("Arch","The size of the nosing")))
        obj.addProperty("App::PropertyLength","TreadThickness","Steps",
                        str(translate("Arch","The thickness of the treads")))
                        
        # structural properties
        obj.addProperty("App::PropertyEnumeration","Landings","Structure",
                        str(translate("Arch","The type of landings of these stairs")))
        obj.addProperty("App::PropertyEnumeration","Winders","Structure",
                        str(translate("Arch","The type of winders in these stairs")))
        obj.addProperty("App::PropertyEnumeration","Structure","Structure",
                        str(translate("Arch","The type of structure of these stairs")))
        obj.addProperty("App::PropertyLength","StructureThickness","Structure",
                        str(translate("Arch","The thickness of the massive structure or of the stringers")))
        obj.addProperty("App::PropertyLength","StringerWidth","Structure",
                        str(translate("Arch","The width of the stringers")))
        obj.addProperty("App::PropertyLength","StringerOffset","Structure",
                        str(translate("Arch","The offset between the border of the stairs and the stringers")))
                        
        obj.Align = ['Left','Right','Center']
        obj.Landings = ["None","At center","At each corner"]
        obj.Winders = ["None","All","Corners strict","Corners relaxed"]
        obj.Structure = ["None","Massive","One stringer","Two stringers"]
        obj.setEditorMode("TreadDepth",1)
        obj.setEditorMode("RiserHeight",1)
        self.Type = "Stairs"

    def execute(self,obj):
        self.getShape(obj)

    def onChanged(self,obj,prop):
        if prop in ["Base"]:
            self.getShape(obj)
        elif prop in ["NumberOfSteps","Length","Height"]:
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

    def makeStairsTread(self,basepoint,depthvec,widthvec,nosing=0,thickness=0):
        "returns the shape of a single tread"
        import Part
        if thickness:
            basepoint = basepoint.add(Vector(0,0,-abs(thickness)))
        if nosing:
            nosevec = DraftVecUtils.scaleTo(DraftVecUtils.neg(depthvec),nosing)
        else:
            nosevec = Vector(0,0,0)
        p1 = basepoint.add(nosevec)
        p2 = p1.add(DraftVecUtils.neg(nosevec)).add(depthvec)
        p3 = p2.add(widthvec)
        p4 = p3.add(DraftVecUtils.neg(depthvec)).add(nosevec)
        step = Part.Face(Part.makePolygon([p1,p2,p3,p4,p1]))
        if thickness:
            step = step.extrude(Vector(0,0,abs(thickness)))
        return step

    def makeStairsStructure(self,mode,nsteps,basepoint,depthvec,widthvec,heightvec,thickness,sthickness,stwidth,stoffset):
        "returns the shape of the structure of a stair"
        import Part
        struct = None
        
        if thickness:
            
            if mode == "Massive":
                points = [basepoint]
                
                #adding thread points
                for i in range(nsteps-1):
                    last = points[-1]
                    if i == 0:
                        last = last.add(Vector(0,0,-abs(sthickness)))
                    p1 = last.add(heightvec)
                    p2 = p1.add(depthvec)
                    points.extend([p1,p2])
                resHeight1 = thickness*((math.sqrt(heightvec.Length**2 + depthvec.Length**2))/depthvec.Length)
                #print "lheight = ",heightvec.Length," ldepth = ",depthvec.Length," resheight1 = ",resHeight1
                
                # adding closing points
                last = points[-1]
                p1 = last.add(Vector(0,0,-resHeight1+abs(sthickness)))
                resHeight2 = ((nsteps-1)*heightvec.Length)-resHeight1
                resLength = (depthvec.Length/heightvec.Length)*resHeight2
                hvec = DraftVecUtils.scaleTo(depthvec,-resLength)
                p2 = p1.add(Vector(hvec.x,hvec.y,-resHeight2))
                
                # making shape
                points.extend([p1,p2,basepoint])
                pol = Part.makePolygon(points)
                #print points
                struct = Part.Face(pol)
                struct = struct.extrude(widthvec)
                
            elif mode != "None":
                if stwidth:
                    # calculating polygon
                    hyp = math.sqrt(heightvec.Length**2 + depthvec.Length**2)
                    l1 = Vector(depthvec).multiply(nsteps-1)
                    h1 = Vector(heightvec).multiply(nsteps-1).add(Vector(0,0,-abs(sthickness)))
                    p1 = basepoint.add(l1).add(h1)
                    h2 = (thickness/depthvec.Length)*hyp
                    p2 = p1.add(Vector(0,0,-abs(h2)))
                    h3 = p2.z-basepoint.z
                    l3 = (h3/heightvec.Length)*depthvec.Length
                    v3 = DraftVecUtils.scaleTo(depthvec,-l3)
                    p3 = p2.add(Vector(0,0,-abs(h3))).add(v3)
                    l4 = (thickness/heightvec.Length)*hyp
                    v4 = DraftVecUtils.scaleTo(depthvec,-l4)
                    p4 = p3.add(v4)
                    pol = Part.makePolygon([p1,p2,p3,p4,p1])
                    pol = Part.Face(pol)
                    evec = DraftVecUtils.scaleTo(widthvec,stwidth)
                    
                    # placing
                    if mode == "One stringer":
                        if stoffset:
                            mvec = DraftVecUtils.scaleTo(widthvec,stoffset)
                        else:
                            mvec = DraftVecUtils.scaleTo(widthvec,(widthvec.Length/2)-stwidth/2)
                        pol.translate(mvec)
                        struct = pol.extrude(evec)
                    elif mode == "Two stringers":
                        pol2 = pol.copy()
                        if stoffset:
                            mvec = DraftVecUtils.scaleTo(widthvec,stoffset)
                            pol.translate(mvec)
                            mvec = widthvec.add(mvec.negative())
                            pol2.translate(mvec)
                        else:
                            pol2.translate(widthvec)
                        s1 = pol.extrude(evec)
                        s2 = pol2.extrude(evec.negative())
                        struct = Part.makeCompound([s1,s2])
        return struct

    def align(self,basepoint,align,width,widthvec):
        "moves a given basepoint according to the alignment"
        if align == "Center":
            basepoint = basepoint.add(DraftVecUtils.scaleTo(widthvec,-width/2))
        elif align == "Right":
            basepoint = basepoint.add(DraftVecUtils.scaleTo(widthvec,-width))
        return basepoint

    def getShape(self,obj):
        "constructs the shape of the stairs"
        steps = []
        structure = []
        pl = obj.Placement
        pieces = []

        # base tests
        if not obj.Width:
            return
        if not obj.Height:
            if not obj.Base:
                return
        if obj.NumberOfSteps < 2:
            return
            
        # 1. Calculate stairs data

        if obj.Base:

            import Part,DraftGeomUtils
            
            # we have a baseline, check it is valid
            if not obj.Base.isDerivedFrom("Part::Feature"):
                return
            if not obj.Base.Shape.Edges:
                return
            if obj.Base.Shape.Faces:
                return

            if (len(obj.Base.Shape.Edges) == 1): 
                edge = obj.Base.Shape.Edges[0]
                if isinstance(edge.Curve,Part.Line):
                    # case 1: only one straight edge
                    p = {}
                    v = DraftGeomUtils.vec(edge)
                    if round(v.z,Draft.precision()) != 0:
                        height = v.z
                        v = Vector(v.x,v.y,0)
                    else:
                        height = obj.Height
                    p['type'] = "straight"
                    p['lstep'] = float(v.Length)/(obj.NumberOfSteps-1)
                    p['lheight'] = float(height)/obj.NumberOfSteps
                    p['depthvec'] = DraftVecUtils.scaleTo(v,p['lstep'])
                    p['widthvec'] = DraftVecUtils.scaleTo(p['depthvec'].cross(Vector(0,0,1)),obj.Width)
                    p['heightvec'] = Vector(0,0,p['lheight'])
                    p['basepoint'] = edge.Vertexes[0].Point
                    pieces.append(p)
                else:
                    print "Not implemented yet!"
            else:
                print "Not implemented yet!"

        else:

            # no baseline, we calculate a simple, straight staircase
            if not obj.Length:
                return
                
            if obj.Landings == "At center":
                print "Not implemented yet!"
            else:
                p = {}
                p['type'] = "straight"
                p['lstep'] = float(obj.Length)/(obj.NumberOfSteps-1)
                p['lheight'] = float(obj.Height)/obj.NumberOfSteps
                p['depthvec'] = Vector(p['lstep'],0,0)
                p['widthvec'] = Vector(0,-obj.Width,0)
                p['heightvec'] = Vector(0,0,p['lheight'])
                p['basepoint'] = Vector(0,0,0)
                pieces.append(p)
            
        # 2. Create stairs components

        for p in pieces:

            if p['type'] == "straight":
                # making structure
                sbasepoint = self.align(p['basepoint'],obj.Align,obj.Width,p['widthvec'])
                s = self.makeStairsStructure(obj.Structure,obj.NumberOfSteps,sbasepoint,p['depthvec'],
                                             p['widthvec'],p['heightvec'],obj.StructureThickness,
                                             obj.TreadThickness,obj.StringerWidth,obj.StringerOffset)
                if s:
                    structure.append(s)
    
                # making steps
                for i in range(obj.NumberOfSteps-1):
                    tpoint = (Vector(p['depthvec']).multiply(i)).add(Vector(p['heightvec']).multiply(i+1))
                    tbasepoint = self.align(p['basepoint'].add(tpoint),obj.Align,obj.Width,p['widthvec'])
                    s = self.makeStairsTread(tbasepoint,p['depthvec'],p['widthvec'],obj.Nosing,obj.TreadThickness)
                    if s:
                        steps.append(s)
                
        # joining everything
        if structure or steps:
            import Part
            shape = Part.makeCompound(structure + steps)
            obj.Shape = shape
            obj.Placement = pl


class _ViewProviderStairs(ArchComponent.ViewProviderComponent):
    "A View Provider for Stairs"
    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        
    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Stairs_Tree.svg"


FreeCADGui.addCommand('Arch_Stairs',_CommandStairs())
