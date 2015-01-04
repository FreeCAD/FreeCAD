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

import FreeCAD,Draft,ArchCommands,ArchFloor
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Building"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

def makeBuilding(objectslist=None,baseobj=None,name="Building"):
    '''makeBuilding(objectslist): creates a building including the
    objects from the given list.'''
    obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroupPython",name)
    _Building(obj)
    if FreeCAD.GuiUp:
        _ViewProviderBuilding(obj.ViewObject)
    if objectslist:
        obj.Group = objectslist
    obj.Label = translate("Arch",name)
    return obj

class _CommandBuilding:
    "the Arch Building command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Building',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Building","Building"),
                'Accel': "B, U",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Building","Creates a building object including selected objects.")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        ok = False
        if (len(sel) == 1):
            if Draft.getType(sel[0]) in ["Cell","Site","Floor"]:
                FreeCAD.ActiveDocument.openTransaction(translate("Arch","Type conversion"))
                FreeCADGui.addModule("Arch")
                FreeCADGui.doCommand("obj = Arch.makeBuilding()")
                FreeCADGui.doCommand("Arch.copyProperties(FreeCAD.ActiveDocument."+sel[0].Name+",obj)")
                FreeCADGui.doCommand('FreeCAD.ActiveDocument.removeObject("'+sel[0].Name+'")')
                FreeCAD.ActiveDocument.commitTransaction()
                ok = True
        if not ok:
            FreeCAD.ActiveDocument.openTransaction(translate("Arch"," Create Building"))
            ss = "["
            for o in sel:
                if len(ss) > 1:
                    ss += ","
                ss += "FreeCAD.ActiveDocument."+o.Name
            ss += "]"
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Floor"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.makeBuilding("+ss+")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()

class _Building(ArchFloor._Floor):
    "The Building object"
    def __init__(self,obj):
        ArchFloor._Floor.__init__(self,obj)
        self.Type = "Building"
        obj.setEditorMode('Height',2)

class _ViewProviderBuilding(ArchFloor._ViewProviderFloor):
    "A View Provider for the Building object"
    def __init__(self,vobj):
        ArchFloor._ViewProviderFloor.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Building_Tree.svg"

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Building',_CommandBuilding())
