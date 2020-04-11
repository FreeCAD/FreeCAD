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
"""Provides tools for creating path arrays with the Draft Workbench.

The copies will be created along a path, like a polyline, B-spline,
or Bezier curve.
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


class PathArray(gui_base_original.Modifier):
    """Gui Command for the Path array tool.

    Parameters
    ----------
    use_link: bool, optional
        It defaults to `False`. If it is `True`, the created object
        will be a `Link array`.
    """

    def __init__(self, use_link=False):
        super(PathArray, self).__init__()
        self.use_link = use_link

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Path array"
        _tip = ("Creates copies of a selected object along a selected path.\n"
                "First select the object, and then select the path.\n"
                "The path can be a polyline, B-spline or Bezier curve.")

        return {'Pixmap': 'Draft_PathArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PathArray", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PathArray", _tip)}

    def Activated(self, name=_tr("Path array")):
        """Execute when the command is called."""
        super(PathArray, self).Activated(name=name)
        if not Gui.Selection.getSelectionEx():
            if self.ui:
                self.ui.selectUi()
                _msg(translate("draft", "Please select base and path objects"))
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
            path = sel[1].Object

            defCount = 4
            defXlate = App.Vector(0, 0, 0)
            defAlign = False
            pathsubs = list(sel[1].SubElementNames)

            App.ActiveDocument.openTransaction("PathArray")
            Draft.makePathArray(base, path,
                                defCount, defXlate, defAlign, pathsubs,
                                use_link=self.use_link)
            App.ActiveDocument.commitTransaction()
            App.ActiveDocument.recompute()
        self.finish()


Gui.addCommand('Draft_PathArray', PathArray())


class PathLinkArray(PathArray):
    """Gui Command for the PathLinkArray tool based on the PathArray tool."""

    def __init__(self):
        super(PathLinkArray, self).__init__(use_link=True)

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Path Link array"
        _tip = ("Like the PathArray tool, but creates a 'Link array' "
                "instead.\n"
                "A 'Link array' is more efficient when handling many copies "
                "but the 'Fuse' option cannot be used.")

        return {'Pixmap': 'Draft_PathLinkArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PathLinkArray", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PathLinkArray", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(PathLinkArray, self).Activated(name=_tr("Link path array"))


Gui.addCommand('Draft_PathLinkArray', PathLinkArray())
