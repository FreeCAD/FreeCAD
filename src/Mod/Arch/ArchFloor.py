# -*- coding: utf8 -*-
#***************************************************************************
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

"""This module provides tools to build Floor objects. Floors are used to group
different Arch objects situated at a same level.

The _Floor object and this module as a whole is now obsolete. It has been
superseded by the use of the BuildingPart class, set to the "Building Storey"
IfcType.
"""

import FreeCAD
import ArchCommands
import ArchIFC
import Draft
import DraftVecUtils
if FreeCAD.GuiUp:
    import FreeCADGui
    from draftutils.translate import translate
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

__title__  = "FreeCAD Arch Floor"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

def makeFloor(objectslist=None,baseobj=None,name=None):
    """Obsolete, superseded by ArchBuildingPart.makeFloor.

    Create a floor.

    Create a new floor based on a group, and then adds the objects in
    objectslist to the new floor.

    Parameters
    ----------
    objectslist: list of <App::DocumentObject>, optional
        The objects to add to the new floor.
    baseobj:
        Unused.
    name: str, optional
        The Label for the new floor.

    Returns
    -------
    <App::DocumentObjectGroupPython>
        The created floor.
    """


    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroupPython","Floor")
    obj.Label = name if name else translate("Arch","Floor")
    _Floor(obj)
    if FreeCAD.GuiUp:
        _ViewProviderFloor(obj.ViewObject)
    if objectslist:
        obj.Group = objectslist
    return obj


class _CommandFloor:
    """The command definition for the Arch workbench's gui tool, Arch Floor.

    A tool for creating Arch floors.

    Create a floor from the objects selected by the user, if any. Exclude
    objects that appear higher in the object hierarchy, such as sites or
    buildings. If free linking is enabled in the Arch preferences, allow higher
    hierarchy objects to be part of floors.

    Find documentation on the end user usage of Arch Floor here:
    https://wiki.freecad.org/Arch_Floor
    """


    def GetResources(self):
        """Return a dictionary with the visual aspects of the Arch Floor tool."""

        return {'Pixmap'  : 'Arch_Floor',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Floor","Level"),
                'Accel': "L, V",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Floor","Creates a Building Part object that represents a level, including selected objects")}

    def IsActive(self):
        """Determine whether or not the Arch Floor tool is active.

        Inactive commands are indicated by a greyed-out icon in the menus and toolbars.
        """

        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        """Executed when Arch Floor is called.

        Create a floor from the objects selected by the user, if any. Exclude
        objects that appear higher in the object hierarchy, such as sites or
        buildings. If free linking is enabled in the Arch preferences, allow
        higher hierarchy objects to be part of floors.
        """

        sel = FreeCADGui.Selection.getSelection()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        link = p.GetBool("FreeLinking",False)
        floorobj = []
        warning = False
        for obj in sel :
            if not Draft.getType(obj) in ["Site", "Building"] :
                floorobj.append(obj)
            else :
                if link:
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


class _Floor(ArchIFC.IfcProduct):
    """Obsolete, superseded by the BuildingPart class, with IfcType set to "Building Storey".

    The Floor object.

    Turns a <App::DocumentObjectGroupPython> into a floor object, then
    takes a list of objects to own as its children.

    The floor can be based off either a group, or a python feature. Learn more
    about groups here: https://wiki.freecad.org/Std_Group

    Adds the properties of a floor, and sets its IFC type.

    Parameters
    ----------
    obj: <App::DocumentObjectGroupPython> or <App::FeaturePython>
        The object to turn into a Floor.
    """

    def __init__(self,obj):
        obj.Proxy = self
        self.Object = obj
        _Floor.setProperties(self,obj)
        self.IfcType = "Building Storey"

    def setProperties(self,obj):
        """Give the object properties unique to floors.

        Add the IFC product properties, and the floor's height and area.
        """

        ArchIFC.IfcProduct.setProperties(self, obj)
        pl = obj.PropertiesList
        if not "Height" in pl:
            obj.addProperty("App::PropertyLength","Height","Floor",QT_TRANSLATE_NOOP("App::Property","The height of this object"))
        if not "Area" in pl:
            obj.addProperty("App::PropertyArea","Area", "Floor",QT_TRANSLATE_NOOP("App::Property","The computed floor area of this floor"))
        if not hasattr(obj,"Placement"):
            # obj can be a Part Feature and already has a placement
            obj.addProperty("App::PropertyPlacement","Placement","Base",QT_TRANSLATE_NOOP("App::Property","The placement of this object"))
        self.Type = "Floor"

    def onDocumentRestored(self,obj):
        """Method run when the document is restored. Re-adds the properties."""

        _Floor.setProperties(self,obj)

    def dumps(self):

        return None

    def loads(self,state):

        return None

    def onChanged(self,obj,prop):
        """Method called when the object has a property changed.

        If the objects grouped under the floor object changes, recompute
        the Area property.

        Also calls ArchIFC.IfcProduct.onChanged().

        Parameters
        ----------
        prop: string
            The name of the property that has changed.
        """
        ArchIFC.IfcProduct.onChanged(self, obj, prop)

        if not hasattr(self,"Object"):
            # on restore, self.Object is not there anymore
            self.Object = obj
        if (prop == "Group") and hasattr(obj,"Area"):
            a = 0
            for o in Draft.getObjectsOfType(Draft.get_group_contents(obj.Group, addgroups=True),
                                            "Space"):
                if hasattr(o,"Area"):
                    if hasattr(o.Area,"Value"):
                        a += o.Area.Value
                        if obj.Area.Value != a:
                            obj.Area = a

    def execute(self,obj):
        """Method run when the object is recomputed.

        Move its children if its placement has changed since the previous
        recompute. Set any child Walls and Structures to have the height of
        the floor if they have not Height value set.
        """

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
        """Add the object to the floor's group.

        Parameters
        ----------
        child: <App::DocumentObject>
            The object to add to the floor's group.
        """

        if hasattr(self,"Object"):
            g = self.Object.Group
            if not child in g:
                g.append(child)
                self.Object.Group = g

    def removeObject(self,child):
        """Remove the object from the floor's group, if it's present.

        Parameters
        ----------
        child: <App::DocumentObject>
            The object to remove from the floor's group.
        """

        if hasattr(self,"Object"):
            g = self.Object.Group
            if child in g:
                g.remove(child)
                self.Object.Group = g


class _ViewProviderFloor:
    """Obsolete, superseded by the ViewProviderBuildingPart class.

    A View Provider for the Floor object.

    Parameters
    ----------
    vobj: <Gui.ViewProviderDocumentObject>
        The view provider to turn into a floor view provider.
    """

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        """Return the path to the appropriate icon.

        Returns
        -------
        str
            Path to the appropriate icon .svg file.
        """

        import Arch_rc
        return ":/icons/Arch_Floor_Tree.svg"

    def attach(self,vobj):
        """Add display modes' data to the coin scenegraph.

        Add each display mode as a coin node, whose parent is this view
        provider.

        Each display mode's node includes the data needed to display the object
        in that mode. This might include colors of faces, or the draw style of
        lines. This data is stored as additional coin nodes which are children
        of the display mode node.

        Do not add any new display modes.
        """

        self.Object = vobj.Object
        return

    def claimChildren(self):
        """Define which objects will appear as children in the tree view.

        Claim all the objects that appear in the floor's group.

        Returns
        -------
        list of <App::DocumentObject>s:
            The objects claimed as children.
        """

        if hasattr(self,"Object"):
            if self.Object:
                return self.Object.Group
        return []

    def dumps(self):

        return None

    def loads(self,state):

        return None

    def setupContextMenu(self,vobj,menu):
        """Add the floor specific options to the context menu.

        The context menu is the drop down menu that opens when the user right
        clicks on the floor in the tree view.

        Add a menu choice to convert the floor to an Arch Building Part with
        the ArchBuildingPart.convertFloors function.

        Parameters
        ----------
        menu: <PySide2.QtWidgets.QMenu>
            The context menu already assembled prior to this method being
            called.
        """

        from PySide import QtCore,QtGui
        import Arch_rc
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Arch_BuildingPart.svg"),"Convert to BuildingPart",menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.convertToBuildingPart)
        menu.addAction(action1)

    def convertToBuildingPart(self):
        """Converts the floor into an Arch Building Part.

        TODO: May be depreciated?
        """

        if hasattr(self,"Object"):
            import ArchBuildingPart
            from DraftGui import todo
            todo.delay(ArchBuildingPart.convertFloors,self.Object)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Floor',_CommandFloor())
