# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2025 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides GUI tools to do various operations with groups.

For example, add objects to groups, select objects inside groups,
set the automatic group in which to create objects, and add objects
to the construction group.
"""
## @package gui_groups
# \ingroup draftguitools
# \brief Provides GUI tools to do various operations with groups.

## \addtogroup draftguitools
# @{
from PySide import QtCore
from PySide import QtWidgets
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
from draftguitools import gui_base
from draftmake import make_layer
from draftutils import groups
from draftutils import params
from draftutils import utils
from draftutils.translate import translate


# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class AddToGroup(gui_base.GuiCommandNeedsSelection):
    """GuiCommand for the Draft_AddToGroup tool.

    It adds selected objects to a group, or removes them from any group.

    It inherits `GuiCommandNeedsSelection` to only be available
    when there is a document and a selection.
    See this class for more information.
    """

    def __init__(self):
        super().__init__(name="Draft_AddToGroup")

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_AddToGroup",
                "MenuText": QT_TRANSLATE_NOOP("Draft_AddToGroup", "Add to group..."),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_AddToGroup", "Adds the selected objects to a group, or removes them from any group.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated()

        if not hasattr(Gui, "draftToolBar"):
            return

        self.ui = Gui.draftToolBar
        objs = [obj for obj in self.doc.Objects if groups.is_group(obj)]
        objs.sort(key=lambda obj: obj.Label)
        self.objects = [None] \
                       + [None] \
                       + objs
        self.labels  = [translate("draft", "Ungroup")] \
                       + ["---"] \
                       + [obj.Label for obj in objs] \
                       + ["---"] \
                       + [translate("draft", "Add to new group...")]
        self.icons   = [self.ui.getIcon(":/icons/list-remove.svg")] \
                       + [None] \
                       + [obj.ViewObject.Icon for obj in objs] \
                       + [None] \
                       + [self.ui.getIcon(":/icons/list-add.svg")]

        # It uses the `DraftToolBar` class defined in the `DraftGui` module and
        # globally initialized in the `Gui` namespace to pop up a menu.
        # Once the desired option is chosen it launches the `proceed` method.
        self.ui.sourceCmd = self
        self.ui.popupMenu(self.labels, self.icons)

    def proceed(self, option):
        """Place the selected objects in the chosen group or ungroup them.
        Parameters
        ----------
        option: str
            The passed string.
        """
        self.ui.sourceCmd = None

        if option == self.labels[0]:
            # "Ungroup"
            self.doc.openTransaction(translate("draft", "Ungroup"))
            for obj in Gui.Selection.getSelection():
                try:
                    groups.ungroup(obj)
                except Exception:
                    pass
            self.doc.commitTransaction()
            self.doc.recompute()
            return

        if option == self.labels[-1]:
            # "Add to new group..."
            grp = AddNamedGroup()  # handles transaction
            grp.Activated()
            return

        # Group has been selected
        self.doc.openTransaction(translate("draft", "Add to group"))
        i = self.labels.index(option)
        grp = self.objects[i]
        moveToGroup(grp)
        self.doc.commitTransaction()
        self.doc.recompute()


Gui.addCommand("Draft_AddToGroup", AddToGroup())


def moveToGroup(group):
    """
    Place the selected objects in the chosen group.
    """

    for obj in Gui.Selection.getSelection():
        try:
            #retrieve group's visibility
            obj.ViewObject.Visibility = group.ViewObject.Visibility
            group.addObject(obj)

        except Exception:
            pass


class SelectGroup(gui_base.GuiCommandNeedsSelection):
    """GuiCommand for the Draft_SelectGroup tool."""

    def __init__(self):
        super().__init__(name="Draft_SelectGroup")

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_SelectGroup",
                "MenuText": QT_TRANSLATE_NOOP("Draft_SelectGroup", "Select group"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_SelectGroup", "Selects the contents of selected groups. For selected non-group objects, the contents of the group they are in is selected.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated()

        sel = Gui.Selection.getSelection()
        subs = []
        for obj in sel:
            if groups.is_group(obj):
                for sub in obj.Group:
                    subs.append(sub)
            else:
                for parent in obj.InList:
                    if groups.is_group(parent):
                        for sub in parent.Group:
                            subs.append(sub)

        # Always clear the selection:
        Gui.Selection.clearSelection()

        # Create a new selection from the sub objects:
        for sub in subs:
            Gui.Selection.addSelection(sub)

        # Inform the user if there is no new selection:
        if not Gui.Selection.hasSelection():
            msg = translate("draft", "No new selection. You must select non-empty groups or objects inside groups.")
            App.Console.PrintMessage(msg + "\n")


Gui.addCommand("Draft_SelectGroup", SelectGroup())


class SetAutoGroup(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_AutoGroup tool."""

    def __init__(self):
        super().__init__(name="Draft_AutoGroup")

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_AutoGroup",
                "MenuText": QT_TRANSLATE_NOOP("Draft_AutoGroup", "Autogroup"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_AutoGroup", "Select a layer or group to add new Draft and BIM objects to.")}

    def Activated(self):
        """Execute when the command is called.

        It calls the `setAutogroup` method of the `DraftToolBar` class
        installed inside the global `Gui` namespace.
        """
        super().Activated()

        if not hasattr(Gui, "draftToolBar"):
            return

        # It uses the `DraftToolBar` class defined in the `DraftGui` module and
        # globally initialized in the `Gui` namespace to run some actions.
        # If a layer or group is selected, it runs the `AutoGroup` method.
        self.ui = Gui.draftToolBar
        sel = Gui.Selection.getSelection()
        if len(sel) == 1:
            if (utils.get_type(sel[0]) == "Layer"
                or (params.get_param("AutogroupAddGroups")
                    and groups.is_group(sel[0]))):
                self.ui.setAutoGroup(sel[0].Name)
                return

        # Otherwise it builds a list of layers and groups, with labels and icons,
        # including the options "None" and "Add new layer".
        if params.get_param("AutogroupAddGroups"):
            grps = [obj for obj in self.doc.Objects if groups.is_group(obj)]
            grps.sort(key=lambda obj: obj.Label)
        else:
            grps = []
        lyrs = [obj for obj in self.doc.Objects if utils.get_type(obj) == "Layer"]
        lyrs.sort(key=lambda obj: obj.Label)
        self.names  = [None] \
                      + [None] \
                      + [obj.Name for obj in grps] \
                      + [None] \
                      + [obj.Name for obj in lyrs]
        self.labels = [translate("draft", "None")] \
                      + ["---"] \
                      + [obj.Label for obj in grps] \
                      + ["---"] \
                      + [obj.Label for obj in lyrs] \
                      + ["---"] \
                      + [translate("draft", "New layer...")]
        self.icons  = [self.ui.getIcon(":/icons/button_invalid.svg")] \
                      + [None] \
                      + [obj.ViewObject.Icon for obj in grps] \
                      + [None] \
                      + [obj.ViewObject.Icon for obj in lyrs] \
                      + [None] \
                      + [self.ui.getIcon(":/icons/Draft_NewLayer.svg")]

        # With the created lists it uses the interface to pop up a menu with options.
        # Once the desired option is chosen it launches the `proceed` method.
        self.ui.sourceCmd = self
        pos = self.ui.autoGroupButton.mapToGlobal(QtCore.QPoint(0, self.ui.autoGroupButton.geometry().height()))
        self.ui.popupMenu(self.labels, self.icons, pos)

    def proceed(self, option):
        """Set the defined autogroup, or create a new layer.

        Parameters
        ----------
        option: str
            The passed string.
        """
        self.ui.sourceCmd = None

        if option == self.labels[0]:
            # "None"
            self.ui.setAutoGroup(None)
            return

        if option == self.labels[-1]:
            # "New layer..."
            txt, ok = QtWidgets.QInputDialog.getText(
                None,
                translate("draft", "Create new layer"),
                translate("draft", "Layer name:"),
                text=translate("draft", "Layer", "Object label")
            )
            if not ok:
                return
            if not txt:
                return
            self.doc.openTransaction(translate("draft", "New layer"))
            lyr = make_layer.make_layer(name=txt, line_color=None, shape_color=None,
                                        line_width=None, draw_style=None, transparency=None)
            self.doc.commitTransaction()
            self.doc.recompute()
            self.ui.setAutoGroup(lyr.Name)           # this...
            # self.ui.autoGroupButton.setDown(False) # or this?
            return

        # Layer or group has been selected
        i = self.labels.index(option)
        self.ui.setAutoGroup(self.names[i])


Gui.addCommand("Draft_AutoGroup", SetAutoGroup())


class AddToConstruction(gui_base.GuiCommandNeedsSelection):
    """GuiCommand for the Draft_AddConstruction tool.

    It adds the selected objects to the construction group
    defined in the `DraftToolBar` class which is initialized
    in the `Gui` namespace when the workbench loads.

    It adds a construction group if it doesn't exist.

    Added objects are also given the visual properties of the construction
    group.
    """

    def __init__(self):
        super().__init__(name="Draft_AddConstruction")

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_AddConstruction",
                "MenuText": QT_TRANSLATE_NOOP("Draft_AddConstruction", "Add to construction group"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_AddConstruction", "Adds the selected objects to the construction group,\nand changes their appearance to the construction style.\nThe construction group is created if it doesn't exist.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated()

        if not hasattr(Gui, "draftToolBar"):
            return

        self.doc.openTransaction(translate("draft", "Add to construction group"))
        col = params.get_param("constructioncolor") | 0x000000FF

        # Get the construction group or create it if it doesn't exist
        grp = self.doc.getObject("Draft_Construction")
        if not grp:
            grp = self.doc.addObject("App::DocumentObjectGroup", "Draft_Construction")
            grp.Label = params.get_param("constructiongroupname")

        for obj in Gui.Selection.getSelection():
            grp.addObject(obj)
            # Change the appearance to the construction colors
            vobj = obj.ViewObject
            if "TextColor" in vobj.PropertiesList:
                vobj.TextColor = col
            if "PointColor" in vobj.PropertiesList:
                vobj.PointColor = col
            if "LineColor" in vobj.PropertiesList:
                vobj.LineColor = col
            if "ShapeColor" in vobj.PropertiesList:
                vobj.ShapeColor = col
            if hasattr(vobj, "Transparency"):
                vobj.Transparency = 80

        self.doc.commitTransaction()
        self.doc.recompute()


Draft_AddConstruction = AddToConstruction
Gui.addCommand("Draft_AddConstruction", AddToConstruction())


class AddNamedGroup(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_AddNamedGroup tool.

    It adds a new named group.
    """

    def __init__(self):
        super().__init__(name="Draft_AddNamedGroup")

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_AddNamedGroup",
                "MenuText": QT_TRANSLATE_NOOP("Draft_AddNamedGroup", "New named group"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_AddNamedGroup", "Adds a group with a given name.")}

    def Activated(self):
        super().Activated()

        txt, ok = QtWidgets.QInputDialog.getText(
            None,
            translate("draft", "Create new group"),
            translate("draft", "Group name:"),
            text=translate("draft", "Group", "Object label")
        )
        if not ok:
            return
        if not txt:
            return
        self.doc.openTransaction(translate("draft", "New named group"))
        grp = self.doc.addObject("App::DocumentObjectGroup", translate("draft", "Group"))
        grp.Label = txt
        moveToGroup(grp)
        self.doc.commitTransaction()
        self.doc.recompute()


Gui.addCommand("Draft_AddNamedGroup", AddNamedGroup())

## @}
