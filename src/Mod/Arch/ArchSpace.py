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

import FreeCAD,FreeCADGui,ArchComponent,ArchCommands,math
from DraftTools import translate
from PyQt4 import QtCore

def makeSpace(objects):
    """makeSpace(objects): Creates a space objects from the given boundary objects"""
    if not objects:
        return
    if not isinstance(objects,list):
        objects = [objects]
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Space")
    _Space(obj)
    _ViewProviderSpace(obj.ViewObject)
    if len(objects) == 1:
        obj.Base = objects[0]
        objects[0].ViewObject.hide()
    else:
        obj.Objects = objects


class _CommandSpace:
    "the Arch Space command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Space',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Space","Space"),
                'Accel': "S, P",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Space","Creates a space object from selected boundary objects")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Space")))
        FreeCADGui.doCommand("import Arch")
        FreeCADGui.doCommand("Arch.makeSpace(FreeCADGui.Selection.getSelection())")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _Space(ArchComponent.Component):
    "A space object"
    def __init__(self,obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyLink","Base","Base",
                        str(translate("Arch","A base shape defining this space")))
        obj.addProperty("App::PropertyLinkList","Objects","Base",
                        str(translate("Arch","The objects that make the boundaries of this space object")))
        self.Type = "Space"

    def execute(self,obj):
        self.getShape(obj)

    def onChanged(self,obj,prop):
        if prop in ["Objects","Base"]:
            self.getShape(obj)

    def getShape(self,obj):
        "computes a shape"
        import Part
        shape = None
        faces = []
        
        # 1: if we have a base shape, we use it
        
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.Solids:
                    shape = obj.Base.Shape.Solids[0]

        # 2: if not, add all bounding boxes of considered objects and build a first shape
        if shape:
            bb = shape.BoundBox
        else:
            bb = None
            for obj in obj.Objects:
                if obj.isDerivedFrom("Part::Feature"):
                    if not bb:
                        bb = obj.Shape.BoundBox
                    else:
                        bb.add(obj.Shape.BoundBox)
            if not bb:
                return
            shape = Part.makeBox(bb.XLength,bb.YLength,bb.ZLength,FreeCAD.Vector(bb.XMin,bb.YMin,bb.ZMin))
        
        # 3: identify all faces pointing towards the center of our shape
        goodfaces = []
        for obj in obj.Objects:
                if obj.isDerivedFrom("Part::Feature"):
                    faces.extend(obj.Shape.Faces)
        for face in faces:
            pt = face.CenterOfMass
            norm = face.normalAt(1,1) #TODO calculate for curved faces
            v1 = bb.Center.sub(pt)
            if v1.getAngle(norm) < math.pi/2:
                goodfaces.append(face)
        faces = goodfaces
        
        # 4: get cutvolumes from faces
        cutvolumes = []
        for f in faces:
            cutface,cutvolume,invcutvolume = ArchCommands.getCutVolume(f,shape)
            if cutvolume:
                cutvolumes.append(cutvolume)
        for v in cutvolumes:
            shape = shape.cut(v)
            
        # 5: get the final shape
        if shape:
            if shape.Solids:
                shape = shape.Solids[0]
                obj.Shape = shape


class _ViewProviderSpace(ArchComponent.ViewProviderComponent):
    "A View Provider for Section Planes"
    def __init__(self,vobj):
        vobj.Transparency = 85
        vobj.LineWidth = 1
        vobj.LineColor = (1.0,0.0,0.0,1.0)
        vobj.DrawStyle = "Dotted"
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        
    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Space_Tree.svg"

    def claimChildren(self):
        if self.Object.Base:
            return [self.Object.Base]
        else:
            return []


FreeCADGui.addCommand('Arch_Space',_CommandSpace())
