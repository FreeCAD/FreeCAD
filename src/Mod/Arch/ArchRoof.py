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

import FreeCAD,FreeCADGui,Draft,ArchComponent, DraftVecUtils
from FreeCAD import Vector
from PyQt4 import QtCore
from DraftTools import translate

__title__="FreeCAD Roof"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

def makeRoof(baseobj=None,facenr=1,angle=45,name=str(translate("Arch","Roof"))):
    '''makeRoof(baseobj,[facenr],[angle],[name]) : Makes a roof based on a
    face from an existing object. You can provide the number of the face
    to build the roof on (default = 1), the angle (default=45) and a name (default
    = roof).'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Roof(obj)
    _ViewProviderRoof(obj.ViewObject)
    if baseobj:
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

    def Activated(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            sel = sel[0]
            obj = sel.Object
            FreeCADGui.Control.closeDialog()
            if sel.HasSubObjects:
                if "Face" in sel.SubElementNames[0]:
                    idx = int(sel.SubElementNames[0][4:])
                    FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Roof")))
                    FreeCADGui.doCommand("import Arch")
                    FreeCADGui.doCommand("Arch.makeRoof(FreeCAD.ActiveDocument."+obj.Name+","+str(idx)+")")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return
            if obj.isDerivedFrom("Part::Feature"):
                if obj.Shape.Wires:
                    FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Roof")))
                    FreeCADGui.doCommand("import Arch")
                    FreeCADGui.doCommand("Arch.makeRoof(FreeCAD.ActiveDocument."+obj.Name+")")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return
            else:
                FreeCAD.Console.PrintMessage(str(translate("Arch","Unable to create a roof")))
        else:
            FreeCAD.Console.PrintMessage(str(translate("Arch","Please select a base object\n")))
            FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
            FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Roof")
            FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)
       
class _Roof(ArchComponent.Component):
    "The Roof object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyAngle","Angle","Base",
                        str(translate("Arch","The angle of this roof")))
        obj.addProperty("App::PropertyInteger","Face","Base",
                        str(translate("Arch","The face number of the base object used to build this roof")))
        self.Type = "Roof"
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        self.hideSubobjects(obj,prop)
        if prop in ["Base","Face","Angle","Additions","Subtractions"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        import Part, math, DraftGeomUtils
        pl = obj.Placement

        base = None
        if obj.Base and obj.Angle:
            w = None
            if obj.Base.isDerivedFrom("Part::Feature"):
                if (obj.Base.Shape.Faces and obj.Face):
                    w = obj.Base.Shape.Faces[obj.Face-1].Wires[0]
                elif obj.Base.Shape.Wires:
                    w = obj.Base.Shape.Wires[0]
            if w:
                if w.isClosed():
                    f = Part.Face(w)
                    norm = f.normalAt(0,0)
                    c = round(math.tan(math.radians(obj.Angle)),Draft.precision())
                    d = f.BoundBox.DiagonalLength
                    edges = DraftGeomUtils.sortEdges(f.Edges)
                    l = len(edges)
                    edges.append(edges[0])
                    shps = []
                    for i in range(l):
                        v = DraftGeomUtils.vec(DraftGeomUtils.angleBisection(edges[i],edges[i+1]))
                        v.normalize()
                        bis = v.getAngle(DraftGeomUtils.vec(edges[i]))
                        delta = 1/math.cos(bis)
                        v.multiply(delta)
                        n = (FreeCAD.Vector(norm)).multiply(c)
                        dv = v.add(n)
                        dv.normalize()
                        dv.scale(d,d,d)
                        shps.append(f.extrude(dv))
                    base = shps.pop()
                    for s in shps:
                        base = base.common(s)
                    base = base.removeSplitter()
                    if not base.isNull():
                        if not DraftGeomUtils.isNull(pl):
                            base.Placement = pl
                            
        base = self.processSubShapes(obj,base)
        if base:
            if not base.isNull():
                obj.Shape = base

class _ViewProviderRoof(ArchComponent.ViewProviderComponent):
    "A View Provider for the Roof object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Roof_Tree.svg"

FreeCADGui.addCommand('Arch_Roof',_CommandRoof())
