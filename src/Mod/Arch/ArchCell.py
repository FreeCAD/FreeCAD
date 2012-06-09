#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
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

import FreeCAD,FreeCADGui,Draft,ArchComponent,ArchCommands
from FreeCAD import Vector
from PyQt4 import QtCore

__title__="FreeCAD Cell"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeCell(objectslist=None,join=True,name="Cell"):
    '''makeCell([objectslist],[joinmode]): creates a cell including the
    objects from the given list. If joinmode is False, contents will
    not be joined.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Cell(obj)
    _ViewProviderCell(obj.ViewObject)
    if objectslist:
        obj.Components = objectslist
    for comp in obj.Components:
        comp.ViewObject.hide()
    obj.JoinMode = join
    return obj

class _CommandCell:
    "the Arch Cell command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Cell',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Cell","Cell"),
                'Accel': "C, E",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Cell","Creates a cell object including selected objects")}
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        ok = False
        if (len(sel) == 1):
            if Draft.getType(sel[0]) in ["Floor","Site","Building"]:
                FreeCAD.ActiveDocument.openTransaction("Type conversion")
                nobj = makeCell()
                ArchCommands.copyProperties(sel[0],nobj)
                FreeCAD.ActiveDocument.removeObject(sel[0].Name)
                FreeCAD.ActiveDocument.commitTransaction()
                ok = True
        if not ok:
            FreeCAD.ActiveDocument.openTransaction("Cell")
            makeCell(sel)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()

class _Cell(ArchComponent.Component):
    "The Cell object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkList","Components","Base",
                        "The objects that make part of this cell")
        obj.addProperty("App::PropertyBool","JoinMode","Base",
                        "If True, underlying geometry will be joined")
        obj.Proxy = self
        self.Type = "Cell"
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        if prop in ["Components","JoinMode"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        import Part
        pl = obj.Placement
        if obj.Components:
            shapes = []
            if obj.JoinMode:
                walls = []
                structs = []
                for c in obj.Components:
                    if Draft.getType(c) == "Wall":
                        walls.append(c.Shape)
                    elif Draft.getType(c) == "Structure":
                        structs.append(c.Shape)
                    else:
                        shapes.append(c.Shape)
                for group in [walls,structs]:
                    if group:
                        sh = group.pop(0).copy()
                        for subsh in group:
                            sh = sh.oldFuse(subsh)
                        shapes.append(sh)
            else:
                for c in obj.Components:
                    shapes.append(c.Shape)
            if shapes:
                obj.Shape = Part.makeCompound(shapes)
            obj.Placement = pl

class _ViewProviderCell(ArchComponent.ViewProviderComponent):
    "A View Provider for the Cell object"
    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        vobj.Proxy = self
        self.Object = vobj.Object

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Cell_Tree.svg"
       
    def updateData(self,obj,prop):
        return

    def onChanged(self,vobj,prop):
        return

    def claimChildren(self):
        return self.Object.Components

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def getDisplayModes(self,obj):
        modes=[]
        return modes

    def setDisplayMode(self,mode):
        return mode

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

FreeCADGui.addCommand('Arch_Cell',_CommandCell())
