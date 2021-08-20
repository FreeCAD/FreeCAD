# ***************************************************************************
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides GUI tools to create PathTwistedArray objects.

The copies will be created along a path, like a polyline, B-spline,
or Bezier curve.
"""
## @package gui_pathtwistedarray
# \ingroup draftguitools
# \brief Provides GUI tools to create PathTwistedArray objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_base_original as gui_base_original

from draftutils.messages import _err
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class PathTwistedArray(gui_base_original.Modifier):
    """Gui Command for the Path twisted array tool.

    Parameters
    ----------
    use_link: bool, optional
        It defaults to `False`. If it is `True`, the created object
        will be a `Link array`.
    """

    def __init__(self, use_link=False):
        super(PathTwistedArray, self).__init__()
        self.use_link = use_link
        self.call = None

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_PathTwistedArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PathTwistedArray", "Path twisted array"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PathTwistedArray", "Creates copies of the selected object along a selected path, and twists the copies.\nFirst select the object, and then select the path.\nThe path can be a polyline, B-spline, Bezier curve, or even edges from other objects.")}

    def Activated(self, name="Path twisted array"):
        """Execute when the command is called."""
        super(PathTwistedArray, self).Activated(name=name)
        self.name = name
        self.proceed()

    def proceed(self):
        """Proceed with the command if one object was selected."""
        sel = Gui.Selection.getSelectionEx()
        if len(sel) != 2:
            _err(translate("draft","Please select exactly two objects, the base object and the path object, before calling this command."))
        else:
            base_object = sel[0].Object
            path_object = sel[1].Object

            count = 15
            rot_factor = 0.25
            use_link = self.use_link

            Gui.addModule("Draft")
            _cmd = "Draft.make_path_twisted_array"
            _cmd += "("
            _cmd += "App.ActiveDocument." + base_object.Name + ", "
            _cmd += "App.ActiveDocument." + path_object.Name + ", "
            _cmd += "count=" + str(count) + ", "
            _cmd += "rot_factor=" + str(rot_factor) + ", "
            _cmd += "use_link=" + str(use_link)
            _cmd += ")"

            _cmd_list = ["_obj_ = " + _cmd,
                         "Draft.autogroup(_obj_)",
                         "App.ActiveDocument.recompute()"]
            self.commit(translate("draft","Path twisted array"), _cmd_list)

        # Commit the transaction and execute the commands
        # through the parent class
        self.finish()


Gui.addCommand('Draft_PathTwistedArray', PathTwistedArray())


class PathTwistedLinkArray(PathTwistedArray):
    """Gui Command for the PathTwistedLinkArray tool."""

    def __init__(self):
        super(PathTwistedLinkArray, self).__init__(use_link=True)

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_PathTwistedLinkArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PathTwistedLinkArray","Path twisted Link array"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PathTwistedLinkArray","Like the PathTwistedArray tool, but creates a 'Link array' instead.\nA 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.")}

    def Activated(self):
        """Execute when the command is called."""
        super(PathTwistedLinkArray,
              self).Activated(name="Path twisted link array")


Gui.addCommand('Draft_PathTwistedLinkArray', PathTwistedLinkArray())

## @}
