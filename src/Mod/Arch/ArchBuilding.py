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

import ArchCell,FreeCAD,FreeCADGui,Draft,ArchCommands
from PyQt4 import QtCore

__title__="FreeCAD Building"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeBuilding(objectslist=None,join=False,name="Building"):
    '''makeBuilding(objectslist,[joinmode]): creates a building including the
    objects from the given list. If joinmode is True, components will be joined.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Building(obj)
    _ViewProviderBuilding(obj.ViewObject)
    if objectslist:
        obj.Components = objectslist
    for comp in obj.Components:
        comp.ViewObject.hide()
    obj.JoinMode = join
    return obj

class _CommandBuilding:
    "the Arch Building command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Building',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Building","Building"),
                'Accel': "B, U",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Building","Creates a building object including selected objects.")}
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        ok = False
        if (len(sel) == 1):
            if Draft.getType(sel[0]) in ["Cell","Site","Floor"]:
                FreeCAD.ActiveDocument.openTransaction("Type conversion")
                nobj = makeBuilding()
                ArchCommands.copyProperties(sel[0],nobj)
                FreeCAD.ActiveDocument.removeObject(sel[0].Name)
                FreeCAD.ActiveDocument.commitTransaction()
                ok = True
        if not ok:
            FreeCAD.ActiveDocument.openTransaction("Building")
            makeBuilding(sel)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        
class _Building(ArchCell._Cell):
    "The Building object"
    def __init__(self,obj):
        ArchCell._Cell.__init__(self,obj)
        self.Type = "Building"
        
class _ViewProviderBuilding(ArchCell._ViewProviderCell):
    "A View Provider for the Building object"
    def __init__(self,vobj):
        ArchCell._ViewProviderCell.__init__(self,vobj)

    def getIcon(self):
        return ":/icons/Arch_Building_Tree.svg"

FreeCADGui.addCommand('Arch_Building',_CommandBuilding())
