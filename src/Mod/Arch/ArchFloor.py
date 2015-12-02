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

import FreeCAD,Draft,ArchCommands, DraftVecUtils
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Arch Floor"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

def makeFloor(objectslist=None,baseobj=None,name="Floor"):
    '''makeFloor(objectslist): creates a floor including the
    objects from the given list.'''
    obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroupPython",name)
    obj.Label = translate("Arch",name)
    _Floor(obj)
    if FreeCAD.GuiUp:
        _ViewProviderFloor(obj.ViewObject)
    if objectslist:
        obj.Group = objectslist
    return obj

class _CommandFloor:
    "the Arch Cell command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Floor',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Floor","Floor"),
                'Accel': "F, L",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Floor","Creates a floor object including selected objects")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        ok = False
        if (len(sel) == 1):
            if Draft.getType(sel[0]) in ["Cell","Site","Building"]:
                FreeCAD.ActiveDocument.openTransaction(translate("Arch","Type conversion"))
                FreeCADGui.addModule("Arch")
                FreeCADGui.doCommand("obj = Arch.makeFloor()")
                FreeCADGui.doCommand("Arch.copyProperties(FreeCAD.ActiveDocument."+sel[0].Name+",obj)")
                FreeCADGui.doCommand('FreeCAD.ActiveDocument.removeObject("'+sel[0].Name+'")')
                FreeCAD.ActiveDocument.commitTransaction()
                ok = True
        if not ok:
            ss = "["
            for o in sel:
                if len(ss) > 1:
                    ss += ","
                ss += "FreeCAD.ActiveDocument."+o.Name
            ss += "]"
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Floor"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.makeFloor("+ss+")")
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

class _Floor:
    "The Floor object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyLength","Height","Arch",translate("Arch","The height of this floor"))
        obj.addProperty("App::PropertyPlacement","Placement","Arch",translate("Arch","The placement of this group"))
        self.Type = "Floor"
        obj.Proxy = self
        self.Object = obj

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state

    def execute(self,obj):
        # move children with this floor
        if hasattr(obj,"Placement"):
            if not hasattr(self,"OldPlacement"):
                self.OldPlacement = obj.Placement.copy()
            else:
                pl = obj.Placement.copy()
                if not DraftVecUtils.equals(pl.Base,self.OldPlacement.Base):
                    print "placement moved"
                    delta = pl.Base.sub(self.OldPlacement.Base)
                    for o in obj.Group:
                        if hasattr(o,"Placement"):
                            o.Placement.move(delta)
                    self.OldPlacement = pl
        # adjust childrens heights
        if obj.Height.Value:
            for o in obj.Group:
                if Draft.getType(o) in ["Wall","Structure"]:
                    if not o.Height.Value:
                        o.Proxy.execute(o)

    def addObject(self,child):
        if hasattr(self,"Object"):
            g = self.Object.Group
            if not child in g:
                g.append(child)
                self.Object.Group = g

    def removeObject(self,child):
        if hasattr(self,"Object"):
            g = self.Object.Group
            if child in g:
                g.remove(child)
                self.Object.Group = g

class _ViewProviderFloor:
    "A View Provider for the Floor object"
    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Floor_Tree.svg"

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def claimChildren(self):
        return self.Object.Group

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Floor',_CommandFloor())
