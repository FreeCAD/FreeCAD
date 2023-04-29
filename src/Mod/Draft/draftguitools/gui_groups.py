# ***************************************************************************
# *   (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>                  *
# *   (c) 2009, 2010 Ken Cline <cline@frii.com>                             *
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
import PySide.QtCore as QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtGui

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import draftutils.utils as utils
import draftutils.groups as groups
import draftguitools.gui_base as gui_base
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
        super(AddToGroup, self).__init__(name=translate("draft", "Add to group"))
        self.ungroup = translate("draft", "Ungroup")
        #add new group string option
        self.addNewGroupStr = "+ " + translate("draft", "Add new group")

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'Pixmap': 'Draft_AddToGroup',
                'MenuText': QT_TRANSLATE_NOOP("Draft_AddToGroup", "Move to group..."),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_AddToGroup", "Moves the selected objects to an existing group, or removes them from any group.\nCreate a group first to use this tool.")}

    def Activated(self):
        """Execute when the command is called."""
        super(AddToGroup, self).Activated()

        self.groups = [self.ungroup]
        self.groups.extend(groups.get_group_names())

        self.labels = [self.ungroup]
        for group in self.groups:
            obj = self.doc.getObject(group)
            if obj:
                self.labels.append(obj.Label)
        #add new group option
        self.labels.append(self.addNewGroupStr)

        # It uses the `DraftToolBar` class defined in the `DraftGui` module
        # and globally initialized in the `Gui` namespace,
        # in order to pop up a menu with group labels
        # or the default `Ungroup` text.
        # Once the desired option is chosen
        # it launches the `proceed` method.
        self.ui = Gui.draftToolBar
        self.ui.sourceCmd = self
        self.ui.popupMenu(self.labels)


    def proceed(self, labelname):
        """Place the selected objects in the chosen group or ungroup them.
        Parameters
        ----------
        labelname: str
            The passed string with the name of the group.
            It puts the selected objects inside this group.
        """
        # If the selected group matches the ungroup label,
        # remove the selection from all groups.
        if labelname == self.ungroup:
            for obj in Gui.Selection.getSelection():
                try:
                    groups.ungroup(obj)
                except Exception:
                    pass
        else:
            # Deactivate the source command of the `DraftToolBar` class
            # so that it doesn't do more with this command.
            self.ui.sourceCmd = None

            #if new group is selected then launch AddNamedGroup
            if labelname == self.addNewGroupStr:
                add=AddNamedGroup()
                add.Activated()
            else:
            #else add selection to the selected group
                if labelname in self.labels :
                    i = self.labels.index(labelname)
                    g = self.doc.getObject(self.groups[i])
                    moveToGroup(g)


Gui.addCommand('Draft_AddToGroup', AddToGroup())


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

    App.activeDocument().recompute(None, True, True)


class SelectGroup(gui_base.GuiCommandNeedsSelection):
    """GuiCommand for the Draft_SelectGroup tool."""

    def __init__(self):
        super(SelectGroup, self).__init__(name=translate("draft","Select group"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'Pixmap': 'Draft_SelectGroup',
                'MenuText': QT_TRANSLATE_NOOP("Draft_SelectGroup", "Select group"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_SelectGroup", "Selects the contents of selected groups. For selected non-group objects, the contents of the group they are in is selected.")}

    def Activated(self):
        """Execute when the command is called."""
        super(SelectGroup, self).Activated()

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


Gui.addCommand('Draft_SelectGroup', SelectGroup())


class SetAutoGroup(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_AutoGroup tool."""

    def __init__(self):
        super(SetAutoGroup, self).__init__(name=translate("draft","Autogroup"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'Pixmap': 'Draft_AutoGroup',
                'MenuText': QT_TRANSLATE_NOOP("Draft_AutoGroup", "Autogroup"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_AutoGroup", "Select a group to add all Draft and Arch objects to.")}

    def Activated(self):
        """Execute when the command is called.

        It calls the `setAutogroup` method of the `DraftToolBar` class
        installed inside the global `Gui` namespace.
        """
        super(SetAutoGroup, self).Activated()

        if not hasattr(Gui, "draftToolBar"):
            return

        # It uses the `DraftToolBar` class defined in the `DraftGui` module
        # and globally initialized in the `Gui` namespace to run
        # some actions.
        # If there is only a group selected, it runs the `AutoGroup` method.
        params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        self.ui = Gui.draftToolBar
        s = Gui.Selection.getSelection()
        if len(s) == 1:
            if (utils.get_type(s[0]) == "Layer"
                or (params.GetBool("AutogroupAddGroups", False)
                    and groups.is_group(s[0]))):
                self.ui.setAutoGroup(s[0].Name)
                return

        # Otherwise it builds a list of layers, with names and icons,
        # including the options "None" and "Add new layer".
        self.groups = [translate("draft", "None")]
        gn = [o.Name for o in self.doc.Objects if utils.get_type(o) == "Layer"]
        if params.GetBool("AutogroupAddGroups", False):
            gn.extend(groups.get_group_names())
        self.groups.extend(gn)
        self.labels = [translate("draft", "None")]
        self.icons = [self.ui.getIcon(":/icons/button_invalid.svg")]
        for g in gn:
            o = self.doc.getObject(g)
            if o:
                self.labels.append(o.Label)
                self.icons.append(o.ViewObject.Icon)
        self.labels.append(translate("draft", "Add new Layer"))
        self.icons.append(self.ui.getIcon(":/icons/document-new.svg"))

        # With the lists created is uses the interface
        # to pop up a menu with layer options.
        # Once the desired option is chosen
        # it launches the `proceed` method.
        self.ui.sourceCmd = self
        pos = self.ui.autoGroupButton.mapToGlobal(QtCore.QPoint(0, self.ui.autoGroupButton.geometry().height()))
        self.ui.popupMenu(self.labels, self.icons, pos)

    def proceed(self, labelname):
        """Set the defined autogroup, or create a new layer.

        Parameters
        ----------
        labelname: str
            The passed string with the name of the group or layer.
        """
        # Deactivate the source command of the `DraftToolBar` class
        # so that it doesn't do more with this command
        # when it finishes.
        self.ui.sourceCmd = None

        if labelname in self.labels:
            if labelname == self.labels[0]:
                # First option "None" deactivates autogrouping
                self.ui.setAutoGroup(None)
            elif labelname == self.labels[-1]:
                # Last option "Add new layer" creates new layer
                Gui.runCommand("Draft_Layer")
            else:
                # Set autogroup to the chosen layer
                i = self.labels.index(labelname)
                self.ui.setAutoGroup(self.groups[i])


Gui.addCommand('Draft_AutoGroup', SetAutoGroup())


class AddToConstruction(gui_base.GuiCommandNeedsSelection):
    """Gui Command for the AddToConstruction tool.

    It adds the selected objects to the construction group
    defined in the `DraftToolBar` class which is initialized
    in the `Gui` namespace when the workbench loads.

    It adds a construction group if it doesn't exist.

    Added objects are also given the visual properties of the construction
    group.
    """

    def __init__(self):
        super(AddToConstruction, self).__init__(name=translate("draft","Add to construction group"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'Pixmap': 'Draft_AddConstruction',
                'MenuText': QT_TRANSLATE_NOOP("Draft_AddConstruction", "Add to Construction group"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_AddConstruction", "Adds the selected objects to the construction group,\nand changes their appearance to the construction style.\nIt creates a construction group if it doesn't exist.")}

    def Activated(self):
        """Execute when the command is called."""
        super(AddToConstruction, self).Activated()

        if not hasattr(Gui, "draftToolBar"):
            return

        col = Gui.draftToolBar.getDefaultColor("constr")
        col = (float(col[0]), float(col[1]), float(col[2]), 0.0)

        # Get the construction group or create it if it doesn't exist
        grp = self.doc.getObject("Draft_Construction")
        if not grp:
            grp = self.doc.addObject("App::DocumentObjectGroup", "Draft_Construction")
            grp.Label = utils.get_param("constructiongroupname", "Construction")

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


Draft_AddConstruction = AddToConstruction
Gui.addCommand('Draft_AddConstruction', AddToConstruction())


class AddNamedGroup(gui_base.GuiCommandSimplest):

    """Gui Command for the addGroup tool.
        It adds a new named group
    """
    def __init__(self):
        super().__init__(name=translate("draft", "Add a new group with a given name"))


    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'Pixmap': 'Draft_AddNamedGroup',
                'MenuText': QT_TRANSLATE_NOOP("Draft_AddNamedGroup", "Add a new named group"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_AddNamedGroup", "Add a new group with a given name.")}


    def Activated(self):
        super().Activated()
        panel = Ui_AddNamedGroup()
        Gui.Control.showDialog(panel)
        panel.name.setFocus()


Draft_AddNamedGroup = AddNamedGroup
Gui.addCommand('Draft_AddNamedGroup', AddNamedGroup())


class Ui_AddNamedGroup():
    """
    User interface for addgroup tool
    simple label and line edit in dialogbox
    """
    def __init__(self):
        self.form = QtGui.QWidget()
        self.form.setWindowTitle(translate("draft", "Add group"))
        row = QtGui.QHBoxLayout(self.form)
        lbl = QtGui.QLabel(translate("draft", "Group name") + ":")
        self.name = QtGui.QLineEdit()
        row.addWidget(lbl)
        row.addWidget(self.name)


    def accept(self):
        group = App.activeDocument().addObject("App::DocumentObjectGroup",translate("draft", "Group"))
        group.Label=self.name.text()
        moveToGroup(group)
        Gui.Control.closeDialog()


## @}
