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

import FreeCAD,FreeCADGui,Draft,ArchCommands
from PyQt4 import QtCore

__title__="FreeCAD Site"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeSite(objectslist=None,name="Site"):
    '''makeBuilding(objectslist): creates a site including the
    objects from the given list.'''
    obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroupPython",name)
    _Site(obj)
    _ViewProviderSite(obj.ViewObject)
    if objectslist:
        obj.Group = objectslist
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
        ok = False
        if (len(sel) == 1):
            if Draft.getType(sel[0]) in ["Cell","Building","Floor"]:
                FreeCAD.ActiveDocument.openTransaction("Type conversion")
                nobj = makeSite()
                ArchCommands.copyProperties(sel[0],nobj)
                FreeCAD.ActiveDocument.removeObject(sel[0].Name)
                FreeCAD.ActiveDocument.commitTransaction()
                ok = True
        if not ok:
            FreeCAD.ActiveDocument.openTransaction("Site")
            makeSite(sel)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        
class _Site:
    "The Site object"
    def __init__(self,obj):
        self.Type = "Site"
        obj.Proxy = self
        
    def execute(self,obj):
        pass
        
    def onChanged(self,obj,prop):
        pass
    
class _ViewProviderSite:
    "A View Provider for the Site object"
    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Arch_Site_Tree.svg"

    def attach(self,vobj):
        self.Object = vobj.Object
        return    
    
    def claimChildren(self):
        return self.Object.Group
    
FreeCADGui.addCommand('Arch_Site',_CommandSite())
