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
"""Provides tools for creating point arrays with the Draft Workbench.

The copies will be created where various points are located.

The points need to be grouped under a compound of points
before using this tool.
To create this compound, select various points and then use the Upgrade tool
to create a `Block`.
"""
## @package gui_patharray
# \ingroup DRAFT
# \brief Provides tools for creating path arrays with the Draft Workbench.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft
import Draft_rc
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
from draftutils.messages import _msg
from draftutils.translate import translate, _tr

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class PointArray(gui_base_original.Modifier):
    """Gui Command for the Point array tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Point array"
        _tip = ("Creates copies of a selected object at the position "
                "of various points.\n"
                "The points need to be grouped under a compound of points "
                "before using this tool.\n"
                "To create this compound, select various points "
                "and then use the Upgrade tool to create a 'Block'.\n"
                "Select the base object, and then select the compound "
                "to create the point array.")

        return {'Pixmap': 'Draft_PointArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PointArray", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PointArray", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(PointArray, self).Activated(name=_tr("Point array"))
        if not Gui.Selection.getSelectionEx():
            if self.ui:
                self.ui.selectUi()
                _msg(translate("draft",
                               "Please select base and pointlist objects."))
                self.call = \
                    self.view.addEventCallback("SoEvent",
                                               gui_tool_utils.selectObject)
        else:
            self.proceed()

    def proceed(self):
        """Proceed with the command if one object was selected."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)

        sel = Gui.Selection.getSelectionEx()
        if sel:
            base = sel[0].Object
            ptlst = sel[1].Object

            App.ActiveDocument.openTransaction("PointArray")
            Draft.makePointArray(base, ptlst)
            App.ActiveDocument.commitTransaction()
            App.ActiveDocument.recompute()
        self.finish()


Gui.addCommand('Draft_PointArray', PointArray())
