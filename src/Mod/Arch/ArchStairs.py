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

import FreeCAD,FreeCADGui,ArchComponent,ArchCommands,Draft,DraftVecUtils
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
            
    def makeTread(self,basepoint,depthvec,widthvec,nosing=0,thickness=0,widthvec2=None):
        "returns a base face of a tread and a tread object"
        import Part
        if thickness:
            basepoint = basepoint.add(Vector(0,0,-abs(thickness)))
        p1 = basepoint
        p2 = p1.add(depthvec)
        p3 = p2.add(widthvec)
        p4 = p3.add(DraftVecUtils.neg(depthvec))
        baseface = Part.Face(Part.makePolygon([p1,p2,p3,p4,p1]))
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
        return baseface,step
            
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
            stepfaces = []
            lstep = float(obj.Length)/(obj.NumberOfSteps-1)
            lheight = float(obj.Height)/(obj.NumberOfSteps)
            for i in range(obj.NumberOfSteps-1):
                basepoint = Vector(i*lstep,0,(i+1)*lheight)
                if obj.Align == "Center":
                    basepoint = basepoint.add(Vector(0,obj.Width/2,0))
                elif obj.Align == "Right":
                    basepoint = basepoint.add(Vector(0,obj.Width,0))
                depthvec = Vector(lstep,0,0)
                widthvec = Vector(0,-obj.Width,0)
                f,s = self.makeTread(basepoint,depthvec,widthvec,obj.Nosing,obj.TreadThickness)
                stepfaces.append(f)
                steps.append(s)
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
