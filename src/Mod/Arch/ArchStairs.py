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

import FreeCAD,FreeCADGui,ArchComponent,ArchCommands,Draft,DraftVecUtils,math
from FreeCAD import Vector
from DraftTools import translate
from PyQt4 import QtCore

def makeStairs(base=None,length=None,width=None,height=None):
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Stairs")
    _Stairs(obj)
    _ViewProviderStairs(obj.ViewObject)
        
class _CommandStairs:
    "the Arch Stairs command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Stairs',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Stairs","Stairs"),
                'Accel': "S, R",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Space","Creates a stairs objects")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Space")))
        FreeCADGui.doCommand("import Arch")
        FreeCADGui.doCommand("Arch.makeStairs()")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

class _Stairs(ArchComponent.Component):
    "A stairs object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        
        # http://en.wikipedia.org/wiki/Stairs
        
        # base properties
        obj.addProperty("App::PropertyLength","Length","Base",
                        str(translate("Arch","The length of these stairs, if no baseline is defined")))
        obj.addProperty("App::PropertyLength","Width","Base",
                        str(translate("Arch","The width of these stairs")))
        obj.addProperty("App::PropertyLength","Height","Base",
                        str(translate("Arch","The total height of these stairs")))
        obj.addProperty("App::PropertyEnumeration","Align","Base",
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
                        str(translate("Arch","The thickness of the massive structure or of the stingers")))
                        
        obj.Align = ['Left','Right','Center']
        obj.Landings = ["None","At each corner","On central segment"]
        obj.Winders = ["None","All","Corners strict","Corners relaxed"]
        obj.Structure = ["None","Massive","One stinger","Two stingers"]
        obj.setEditorMode("TreadDepth",1)
        obj.setEditorMode("RiserHeight",1)
        self.Type = "Stairs"

    def execute(self,obj):
        self.getShape(obj)

    def onChanged(self,obj,prop):
        if prop in ["Base"]:
            self.getShape(obj)
        elif prop in ["NumberOfSteps","Length","Height"]:
            self.setStepData(obj)
        
    def setStepData(self,obj):
        "sets step depth and height values"
        if obj.NumberOfSteps > 1:
            obj.TreadDepth = float(obj.Length)/(obj.NumberOfSteps-1)
            obj.RiserHeight = float(obj.Height)/obj.NumberOfSteps

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

    def makeStairsStructure(self,mode,nsteps,basepoint,depthvec,widthvec,heightvec,thickness,sthickness):
        "returns the shape of the structure of a stair"
        import Part
        struct = None
        if thickness:
            if mode == "Massive":
                points = [basepoint]
                for i in range(nsteps-1):
                    last = points[-1]
                    if i == 0:
                        last = last.add(Vector(0,0,-abs(sthickness)))
                    p1 = last.add(heightvec)
                    p2 = p1.add(depthvec)
                    points.extend([p1,p2])
                resHeight1 = thickness*((math.sqrt(heightvec.Length**2 + depthvec.Length**2))/depthvec.Length)
                #print "lheight = ",heightvec.Length," ldepth = ",depthvec.Length," resheight1 = ",resHeight1
                last = points[-1]
                p1 = last.add(Vector(0,0,-resHeight1+abs(sthickness)))
                resHeight2 = ((nsteps-1)*heightvec.Length)-resHeight1
                resLength = (depthvec.Length/heightvec.Length)*resHeight2
                p2 = p1.add(Vector(-resLength,0,-resHeight2))
                points.extend([p1,p2,basepoint])
                struct = Part.Face(Part.makePolygon(points))
                struct = struct.extrude(widthvec)
        return struct
        
    def align(self,basepoint,align,width):
        "moves a given basepoint according to the alignment"
        if align == "Center":
            basepoint = basepoint.add(Vector(0,width/2,0))
        elif align == "Right":
            basepoint = basepoint.add(Vector(0,width,0))
        return basepoint

    def getShape(self,obj):
        "constructs the shape of the stairs"
        steps = []
        structure = []
        pl = obj.Placement
        if not obj.Width:
            return
        if not obj.Height:
            return
        if obj.NumberOfSteps < 2:
            return
        if obj.Base:
            pass
        else:
            if not obj.Length:
                return
            
            # definitions
            lstep = float(obj.Length)/(obj.NumberOfSteps-1)
            lheight = float(obj.Height)/obj.NumberOfSteps
            depthvec = Vector(lstep,0,0)
            widthvec = Vector(0,-obj.Width,0)
            heightvec = Vector(0,0,lheight)
            
            # making structure
            basepoint = self.align(Vector(0,0,0),obj.Align,obj.Width)
            s = self.makeStairsStructure(obj.Structure,obj.NumberOfSteps,basepoint,depthvec,
                                         widthvec,heightvec,obj.StructureThickness,obj.TreadThickness)
            if s:
                structure.append(s)
            
            # making steps
            for i in range(obj.NumberOfSteps-1):
                basepoint = self.align(Vector(i*lstep,0,(i+1)*lheight),obj.Align,obj.Width)
                s = self.makeStairsTread(basepoint,depthvec,widthvec,obj.Nosing,obj.TreadThickness)
                if s:
                    steps.append(s)
                
        # joining everything
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
