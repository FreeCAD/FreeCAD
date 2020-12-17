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

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import draftutils.utils as utils
import draftutils.groups as groups
import draftguitools.gui_base as gui_base

from draftutils.translate import _tr, translate

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
        super(AddToGroup, self).__init__(name=_tr("Add to group"))
        self.ungroup = QT_TRANSLATE_NOOP("Draft_AddToGroup",
                                         "Ungroup")

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tooltip = ("Moves the selected objects to an existing group, "
                    "or removes them from any group.\n"
                    "Create a group first to use this tool.")

        d = {'Pixmap': 'Draft_AddToGroup',
             'MenuText': QT_TRANSLATE_NOOP("Draft_AddToGroup",
                                           "Move to group")+"...",
             'ToolTip': QT_TRANSLATE_NOOP("Draft_AddToGroup",
                                          _tooltip)}
        return d

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
        # Deactivate the source command of the `DraftToolBar` class
        # so that it doesn't do more with this command.
        self.ui.sourceCmd = None

        # If the selected group matches the ungroup label,
        # remove the selection from all groups.
        if labelname == self.ungroup:
            for obj in Gui.Selection.getSelection():
                try:
                    groups.ungroup(obj)
                except Exception:
                    pass
        else:
            # Otherwise try to add all selected objects to the chosen group
            if labelname in self.labels:
                i = self.labels.index(labelname)
                g = self.doc.getObject(self.groups[i])
                for obj in Gui.Selection.getSelection():
                    try:
                        g.addObject(obj)
                    except Exception:
                        pass


Gui.addCommand('Draft_AddToGroup', AddToGroup())


class SelectGroup(gui_base.GuiCommandNeedsSelection):
    """GuiCommand for the Draft_SelectGroup tool.

    If the selection is a group, it selects all objects
    with the same "parents" as this object. This means all objects
    that are inside this group, including those in nested sub-groups.

    If the selection is a simple object inside a group,
    it will select the "brother" objects, that is, those objects that are
    at the same level as this object, including the upper group
    that contains them all.

    NOTE: the second functionality is a bit strange, as it produces results
    that are not very intuitive. Maybe we should change it and restrict
    this command to only groups (`App::DocumentObjectGroup`) because
    in this case it works in an intuitive manner, selecting
    only the objects under the group.

    It inherits `GuiCommandNeedsSelection` to only be available
    when there is a document and a selection.
    See this class for more information.
    """

    def __init__(self):
        super(SelectGroup, self).__init__(name=_tr("Select group"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tooltip = ("If the selection is a group, it selects all objects "
                    "that are inside this group, including those in "
                    "nested sub-groups.\n"
                    "\n"
                    "If the selection is a simple object inside a group, "
                    'it will select the "brother" objects, that is,\n'
                    "those that are at the same level as this object, "
                    "including the upper group that contains them all.")

        d = {'Pixmap': 'Draft_SelectGroup',
             'MenuText': QT_TRANSLATE_NOOP("Draft_SelectGroup",
                                           "Select group"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_SelectGroup",
                                          _tooltip)}
        return d

    def Activated(self):
        """Execute when the command is called.

        If the selection is a single group, it selects all objects
        inside this group.

        In other cases it selects all objects (children)
        in the OutList of this object, and also all objects (parents)
        in the InList of this object.
        For all parents, it also selects the children of these.
        """
        super(SelectGroup, self).Activated()

        sel = Gui.Selection.getSelection()
        if len(sel) == 1:
            if sel[0].isDerivedFrom("App::DocumentObjectGroup"):
                cts = groups.get_group_contents(Gui.Selection.getSelection())
                for o in cts:
                    Gui.Selection.addSelection(o)
                return
        for obj in sel:
            # This selects the objects in the `OutList`
            # which are actually `parents` but appear below in the tree.
            # Regular objects usually have an empty `OutList`
            # so this is skipped.
            # But for groups, it selects the objects
            # that it contains under it.
            for child in obj.OutList:
                Gui.Selection.addSelection(child)

            # This selects the upper group that contains `obj`.
            # Then for this group, it selects the objects in its `OutList`,
            # which are at the same level as `obj` (brothers).
            for parent in obj.InList:
                Gui.Selection.addSelection(parent)
                for child in parent.OutList:
                    Gui.Selection.addSelection(child)
        # -------------------------------------------------------------------
        # NOTE: the terminology here may be confusing.
        # Those in the `InList` are actually `children` (dependents)
        # but appear above in the tree view,
        # and this is the reason they are called `parents`.
        #
        # Those in the `OutList` are actually `parents` (suppliers)
        # but appear below in the tree, and this is the reason
        # they are called `children`.
        #
        # InList
        #  |
        #  - object
        #     |
        #     - OutList
        #
        # -------------------------------------------------------------------


Gui.addCommand('Draft_SelectGroup', SelectGroup())


class SetAutoGroup(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_AutoGroup tool."""

    def __init__(self):
        super(SetAutoGroup, self).__init__(name=_tr("Autogroup"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = "Select a group to add all Draft and Arch objects to."

        return {'Pixmap': 'Draft_AutoGroup',
                'MenuText': QT_TRANSLATE_NOOP("Draft_AutoGroup", "Autogroup"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_AutoGroup", _tip)}

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
        self.ui = Gui.draftToolBar
        s = Gui.Selection.getSelection()
        if len(s) == 1:
            if (utils.get_type(s[0]) == "Layer") or \
-               (App.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM").GetBool("AutogroupAddGroups", False)
             and (s[0].isDerivedFrom("App::DocumentObjectGroup")
                  or utils.get_type(s[0]) in ["Site", "Building",
                                              "Floor", "BuildingPart"])):
                self.ui.setAutoGroup(s[0].Name)
                return

        # Otherwise it builds a list of layers, with names and icons,
        # including the options "None" and "Add new layer".
        self.groups = ["None"]
        gn = [o.Name for o in self.doc.Objects if utils.get_type(o) == "Layer"]
        if App.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM").GetBool("AutogroupAddGroups", False):
            gn.extend(groups.get_group_names())
        if gn:
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


class AddToConstruction(gui_base.GuiCommandSimplest):
    """Gui Command for the AddToConstruction tool.

    It adds the selected objects to the construction group
    defined in the `DraftToolBar` class which is initialized
    in the `Gui` namespace when the workbench loads.

    It adds a construction group if it doesn't exist.

    Added objects are also given the visual properties of the construction
    group.
    """

    def __init__(self):
        super(AddToConstruction, self).__init__(name=_tr("Add to construction group"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Add to Construction group"
        _tip = ("Adds the selected objects to the construction group,\n"
                "and changes their appearance to the construction style.\n"
                "It creates a construction group if it doesn't exist.")

        d = {'Pixmap': 'Draft_AddConstruction',
             'MenuText': QT_TRANSLATE_NOOP("Draft_AddConstruction", _menu),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_AddConstruction", _tip)}
        return d

    def Activated(self):
        """Execute when the command is called."""
        super(AddToConstruction, self).Activated()

        if not hasattr(Gui, "draftToolBar"):
            return

        col = Gui.draftToolBar.getDefaultColor("constr")
        col = (float(col[0]), float(col[1]), float(col[2]), 0.0)

        # Get the construction group or create it if it doesn't exist
        gname = utils.get_param("constructiongroupname", "Construction")
        grp = self.doc.getObject(gname)
        if not grp:
            grp = self.doc.addObject("App::DocumentObjectGroup", gname)

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

## @}
