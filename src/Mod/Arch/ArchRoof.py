#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012                                                    *  
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

import FreeCAD,FreeCADGui,Draft,ArchComponent
from draftlibs import fcvec
from FreeCAD import Vector
from PyQt4 import QtCore

__title__="FreeCAD Roof"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeRoof(baseobj,facenr=1,angle=45,name="Roof"):
    '''makeRoof(baseobj,[facenr],[angle],[name]) : Makes a roof based on a
    face from an existing object. You can provide the number of the face
    to build the roof on (default = 1), the angle (default=45) and a name (default
    = roof).'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Roof(obj)
    _ViewProviderRoof(obj.ViewObject)
    obj.Base = baseobj
    obj.Face = facenr
    obj.Angle = angle
    return obj

class _CommandRoof:
    "the Arch Roof command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Roof',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Roof","Roof"),
                'Accel': "R, F",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Roof","Creates a roof object from the selected face of an object")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            sel = sel[0]
            obj = sel.Object
            if sel.HasSubObjects:
                if "Face" in sel.SubElementNames[0]:
                    idx = int(sel.SubElementNames[0][4:])
                    FreeCAD.ActiveDocument.openTransaction("Create Roof")
                    makeRoof(obj,idx)
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                elif obj.isDerivedFrom("Part::Feature"):
                    if len(obj.Shape.Faces) == 1:
                        FreeCAD.ActiveDocument.openTransaction("Create Roof")
                        makeRoof(obj,1)
                        FreeCAD.ActiveDocument.commitTransaction()
                        FreeCAD.ActiveDocument.recompute()
            elif obj.isDerivedFrom("Part::Feature"):
                if len(obj.Shape.Faces) == 1:
                    FreeCAD.ActiveDocument.openTransaction("Create Roof")
                    makeRoof(obj,1)
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
            else:
                FreeCAD.Console.PrintMessage("Unable to create a roof")
        else:
            FreeCAD.Console.PrintMessage("No object selected")
       
class _Roof(ArchComponent.Component):
    "The Roof object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyAngle","Angle","Base",
                        "The angle of this roof")
        obj.addProperty("App::PropertyInteger","Face","Base",
                        "The face number of the base object used to build this roof")
        self.Type = "Structure"
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        if prop in ["Base","Face","Angle","Additions","Subtractions"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        import Part,math
        from draftlibs import fcgeo
        pl = obj.Placement

        if obj.Base and obj.Face and obj.Angle:
            if len(obj.Base.Shape.Faces) >= obj.Face:
                f = obj.Base.Shape.Faces[obj.Face-1]
                if len(f.Wires) == 1:
                    if f.Wires[0].isClosed():
                        c = round(math.tan(math.radians(obj.Angle)),Draft.precision())
                        norm = f.normalAt(0,0)
                        d = f.BoundBox.DiagonalLength
                        edges = fcgeo.sortEdges(f.Edges)
                        l = len(edges)
                        edges.append(edges[0])
                        shps = []
                        for i in range(l):
                            v = fcgeo.vec(fcgeo.angleBisection(edges[i],edges[i+1]))
                            v.normalize()
                            bis = v.getAngle(fcgeo.vec(edges[i]))
                            delta = 1/math.cos(bis)
                            v.multiply(delta)
                            n = (FreeCAD.Vector(norm)).multiply(c)
                            dv = v.add(n)
                            dv.normalize()
                            dv.scale(d,d,d)
                            shps.append(f.extrude(dv))
                        c = shps.pop()
                        for s in shps:
                            c = c.common(s)
                        c = c.removeSplitter()
                        if not c.isNull():
                            obj.Shape = c        
                            if not fcgeo.isNull(pl):
                                obj.Placement = pl

class _ViewProviderRoof(ArchComponent.ViewProviderComponent):
    "A View Provider for the Roof object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):          
        return ":/icons/Arch_Roof_Tree.svg"

FreeCADGui.addCommand('Arch_Roof',_CommandRoof())
