# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchFloor
#  \ingroup ARCH
#  \brief The Floor object and tools
#
#  This module provides tools to build Floor objects.
#  Floors are used to group different Arch objects situated
#  at a same level

__title__="FreeCAD Arch Floor"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


def makeFloor(objectslist=None,baseobj=None,name="Floor"):

    '''makeFloor(objectslist): creates a floor including the
    objects from the given list.'''

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
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
                'MenuText': QT_TRANSLATE_NOOP("Arch_Floor","Level"),
                'Accel': "L, V",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Floor","Creates a Building Part object that represents a level, including selected objects")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        link = p.GetBool("FreeLinking",False)
        floorobj = []
        warning = False
        for obj in sel :
            if not Draft.getType(obj) in ["Site", "Building"] :
                floorobj.append(obj)
            else :
                if link == True :
                    floorobj.append(obj)
                else:
                    warning = True
        if warning :
            message = translate( "Arch" , "You can put anything but the following objects: Site, Building, and Floor - in a Floor object.\n\
Floor object is not allowed to accept Site, Building, or Floor objects.\n\
Site, Building, and Floor objects will be removed from the selection.\n\
You can change that in the preferences.") + "\n"
            ArchCommands.printMessage( message )
        if sel and len(floorobj) == 0:
            message = translate( "Arch" , "There is no valid object in the selection.\n\
Floor creation aborted.") + "\n"
            ArchCommands.printMessage( message )
        else :
            ss = "[ "
            for o in floorobj:
                ss += "FreeCAD.ActiveDocument." + o.Name + ", "
            ss += "]"
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Floor"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("obj = Arch.makeFloor("+ss+")")
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


class _Floor:

    "The Floor object"

    def __init__(self,obj):

        obj.Proxy = self
        self.Object = obj
        _Floor.setProperties(self,obj)
        self.IfcType = "Building Storey"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Height" in pl:
            obj.addProperty("App::PropertyLength","Height","Floor",QT_TRANSLATE_NOOP("App::Property","The height of this object"))
        if not "Area" in pl:
            obj.addProperty("App::PropertyArea","Area", "Floor",QT_TRANSLATE_NOOP("App::Property","The computed floor area of this floor"))
        if not hasattr(obj,"Placement"):
            # obj can be a Part Feature and already has a placement
            obj.addProperty("App::PropertyPlacement","Placement","Base",QT_TRANSLATE_NOOP("App::Property","The placement of this object"))
        if not "IfcType" in pl:
            obj.addProperty("App::PropertyEnumeration","IfcType","IFC",QT_TRANSLATE_NOOP("App::Property","The type of this object"))
            import ArchIFC
            obj.IfcType = ArchIFC.IfcTypes
        self.Type = "Floor"

    def onDocumentRestored(self,obj):

        _Floor.setProperties(self,obj)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def onChanged(self,obj,prop):

        if not hasattr(self,"Object"):
            # on restore, self.Object is not there anymore
            self.Object = obj
        if (prop == "Group") and hasattr(obj,"Area"):
            a = 0
            for o in Draft.getObjectsOfType(Draft.getGroupContents(obj.Group,addgroups=True),"Space"):
                if hasattr(o,"Area"):
                    if hasattr(o.Area,"Value"):
                        a += o.Area.Value
                        if obj.Area.Value != a:
                            obj.Area = a

    def execute(self,obj):

        # move children with this floor
        if hasattr(obj,"Placement"):
            if not hasattr(self,"OldPlacement"):
                self.OldPlacement = obj.Placement.copy()
            else:
                pl = obj.Placement.copy()
                if not DraftVecUtils.equals(pl.Base,self.OldPlacement.Base):
                    print("placement moved")
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

        if hasattr(self,"Object"):
            if self.Object:
                return self.Object.Group
        return []

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def setupContextMenu(self,vobj,menu):
        from PySide import QtCore,QtGui
        import Arch_rc
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Arch_BuildingPart.svg"),"Convert to BuildingPart",menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.convertToBuildingPart)
        menu.addAction(action1)

    def convertToBuildingPart(self):
        if hasattr(self,"Object"):
            import ArchBuildingPart
            from DraftGui import todo
            todo.delay(ArchBuildingPart.convertFloors,self.Object)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Floor',_CommandFloor())
