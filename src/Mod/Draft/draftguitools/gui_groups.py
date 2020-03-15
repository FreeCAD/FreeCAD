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
"""Provides tools to do various operations with groups.

For example, add objects to groups.
"""
## @package gui_groups
# \ingroup DRAFT
# \brief Provides tools to do various operations with groups.
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import draftutils.utils as utils
import draftguitools.gui_base as gui_base
from draftutils.translate import _tr

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class AddToGroup(gui_base.GuiCommandNeedsSelection):
    """GuiCommand for the Draft_AddToGroup tool.

    It adds selected objects to a group, or removes them from any group.

    It inherits `GuiCommandNeedsSelection` to only be availbale
    when there is a document and a selection.
    See this class for more information.
    """

    def __init__(self):
        super().__init__(name=_tr("Add to group"))
        self.ungroup = QT_TRANSLATE_NOOP("Draft_AddToGroup",
                                         "Ungroup")

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tooltip = ("Moves the selected objects to an existing group, "
                    "or removes them from any group.\n"
                    "Create a group first to use this tool.")

        d = {'Pixmap': 'Draft_AddToGroup',
             'MenuText': QT_TRANSLATE_NOOP("Draft_AddToGroup",
                                           "Move to group"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_AddToGroup",
                                          _tooltip)}
        return d

    def Activated(self):
        """Execute when the command is called."""
        super().Activated()

        self.groups = [self.ungroup]
        self.groups.extend(utils.get_group_names())

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
                    utils.ungroup(obj)
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
