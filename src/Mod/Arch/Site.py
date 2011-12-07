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

import Cell,FreeCAD,FreeCADGui,Draft,Commands
from PyQt4 import QtCore

__title__="FreeCAD Site"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeSite(objectslist=None,name="Site"):
    '''makeBuilding(objectslist): creates a site including the
    objects from the given list.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Site(obj)
    _ViewProviderSite(obj.ViewObject)
    if objectslist:
        obj.Components = objectslist
    obj.JoinMode = False
    return obj

class _CommandSite:
    "the Arch Site command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Site',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Site","Site"),
                'Accel': "S, I",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Site","Creates a site object including selected objects.")}
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        transmode = True
        if not sel:
            transmode = False
        for obj in sel:
            if not (Draft.getType(obj) in ["Cell","Building","Floor"]):
                transmode = False
        if transmode:
            FreeCAD.ActiveDocument.openTransaction("Type conversion")
            for obj in sel:
                nobj = makeSite()
                Commands.copyProperties(obj,nobj)
                FreeCAD.ActiveDocument.removeObject(obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.ActiveDocument.openTransaction("Site")
            makeSite(sel)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

class _Site(Cell._Cell):
    "The Site object"
    def __init__(self,obj):
        Cell._Cell.__init__(self,obj)
        self.Type = "Site"
        
    def execute(self,obj):
        pass
        
    def onChanged(self,obj,prop):
        pass
    
class _ViewProviderSite(Cell._ViewProviderCell):
    "A View Provider for the Site object"
    def __init__(self,vobj):
        Cell._ViewProviderCell.__init__(self,vobj)

    def getIcon(self):
        return ":/icons/Arch_Site_Tree.svg"

FreeCADGui.addCommand('Arch_Site',_CommandSite())
