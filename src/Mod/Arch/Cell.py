#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
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

import FreeCAD,FreeCADGui,Part,Draft,Component,Commands
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
        transmode = True
        if not sel:
            transmode = False
        for obj in sel:
            if not (Draft.getType(obj) in ["Floor","Site","Building"]):
                transmode = False
        if transmode:
            FreeCAD.ActiveDocument.openTransaction("Type conversion")
            for obj in sel:
                nobj = makeCell()
                Commands.copyProperties(obj,nobj)
                FreeCAD.ActiveDocument.removeObject(obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.ActiveDocument.openTransaction("Cell")
            makeCell(sel)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

class _Cell(Component.Component):
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
        pl = obj.Placement
        if obj.Components:
            if obj.JoinMode:
                components = obj.Components[:]
                f = components.pop(0)
                baseShape = f.Shape
                for comp in components:
                    if Draft.getType(comp) in ["Wall","Cell","Shape"]:
                        baseShape = baseShape.oldFuse(comp.Shape)
            else:
                compshapes = []
                for o in obj.Components:
                    compshapes.append(o.Shape)
                baseShape = Part.makeCompound(compshapes)
            obj.Shape = baseShape
            obj.Placement = pl

class _ViewProviderCell(Component.ViewProviderComponent):
    "A View Provider for the Cell object"
    def __init__(self,vobj):
        Component.ViewProviderComponent.__init__(self,vobj)
        vobj.Proxy = self
        self.Object = vobj.Object

    def getIcon(self):
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
