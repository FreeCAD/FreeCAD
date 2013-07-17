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
    """makeSpace(objects): Creates a space object from the given objects. Objects can be one
    document object, in which case it becomes the base shape of the space object, or a list of
    selection objects as got from getSelectionEx(), or a list of tuples (object, subobjectname)"""
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
        obj.Proxy.addSubobjects(objects)
        
def addSpaceBoundary(space,subobjects):
    """addSpaceBoundary(space,subobjects): adds the given subobjects to the given space"""
    import Draft
    if Draft.getType(space) == "Space":
        space.Proxy.addSubobjects(space,subobjects)

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
        obj.addProperty("App::PropertyLinkSubList","Boundaries","Base",
                        str(translate("Arch","The objects that make the boundaries of this space object")))
        self.Type = "Space"

    def execute(self,obj):
        self.getShape(obj)

    def onChanged(self,obj,prop):
        print prop
        if prop in ["Boundaries","Base"]:
            self.getShape(obj)
            
    def addSubobjects(self,obj,subobjects):
        "adds subobjects to this space"
        objs = []
        for o in subobjects:
            print o
            if isinstance(o,tuple) or isinstance(o,list):
                objs.append(tuple(o))
            else:
                for el in o.SubElementNames:
                    if "Face" in el:
                        print "adding ",el
                        objs.append((o.Object,el))
        print "boundaries to add: ",objs
        obj.Boundaries = objs

    def getShape(self,obj):
        "computes a shape"
        import Part
        shape = None
        faces = []
        
        print "starting compute"
        # 1: if we have a base shape, we use it
        
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.Solids:
                    shape = obj.Base.Shape.Solids[0].copy()

        # 2: if not, add all bounding boxes of considered objects and build a first shape
        if shape:
            print "got shape from base object"
            bb = shape.BoundBox
        else:
            bb = None
            for b in obj.Boundaries:
                if b[0].isDerivedFrom("Part::Feature"):
                    if not bb:
                        bb = b[0].Shape.BoundBox
                    else:
                        bb.add(b[0].Shape.BoundBox)
            if not bb:
                return
            shape = Part.makeBox(bb.XLength,bb.YLength,bb.ZLength,FreeCAD.Vector(bb.XMin,bb.YMin,bb.ZMin))
            print "created shape from boundbox"
        
        # 3: identifing boundary faces
        goodfaces = []
        for b in obj.Boundaries:
                if b[0].isDerivedFrom("Part::Feature"):
                    if "Face" in b[1]:
                        fn = int(b[1][4:])-1
                        faces.append(b[0].Shape.Faces[fn])
                        print "adding face ",fn," of object ",b[0].Name

        print "total: ", len(faces), " faces"
        
        # 4: get cutvolumes from faces
        cutvolumes = []
        for f in faces:
            f = f.copy()
            f.reverse()
            cutface,cutvolume,invcutvolume = ArchCommands.getCutVolume(f,shape)
            if cutvolume:
                print "generated 1 cutvolume"
                cutvolumes.append(cutvolume.copy())
                #Part.show(cutvolume)
        for v in cutvolumes:
            print "cutting"
            shape = shape.cut(v)
            
        # 5: get the final shape
        if shape:
            if shape.Solids:
                print "setting objects shape"
                shape = shape.Solids[0]
                obj.Shape = shape
                return
                
        print "something went wrong, bailing out"


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
