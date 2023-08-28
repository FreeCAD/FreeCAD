# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2013 WandererFan <wandererfan@gmail.com>                *
# *   Copyright (c) 2019 Zheng, Lei (realthunder)<realthunder.dev@gmail.com>*
# *   Copyright (c) 2020 Carlo Pavan <carlopav@gmail.com>                   *
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
"""Provides GUI tools to create PathArray objects.

The copies will be created along a path, like a polyline, B-spline,
or Bezier curve.
"""
## @package gui_patharray
# \ingroup draftguitools
# \brief Provides GUI tools to create PathArray objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftVecUtils
import draftguitools.gui_base_original as gui_base_original

from draftutils.messages import _err
from draftutils.translate import translate

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
        self.call = None

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_PathArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PathArray", "Path array"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PathArray", "Creates copies of the selected object along a selected path.\nFirst select the object, and then select the path.\nThe path can be a polyline, B-spline, Bezier curve, or even edges from other objects.")}

    def Activated(self, name="Path array"):
        """Execute when the command is called."""
        super(PathArray, self).Activated(name=name)
        self.name = name
        # This was deactivated because it doesn't work correctly;
        # the selection needs to be made on two objects, but currently
        # it only selects one.

        # if not Gui.Selection.getSelectionEx():
        #     if self.ui:
        #         self.ui.selectUi()
        #         _msg(translate("draft",
        #                        "Please select exactly two objects, "
        #                        "the base object and the path object, "
        #                        "before calling this command."))
        #         self.call = \
        #             self.view.addEventCallback("SoEvent",
        #                                        gui_tool_utils.selectObject)
        # else:
        #     self.proceed()
        self.proceed()

    def proceed(self):
        """Proceed with the command if one object was selected."""
        sel = Gui.Selection.getSelectionEx()
        if len(sel) != 2:
            _err(translate("draft","Please select exactly two objects, the base object and the path object, before calling this command."))
        else:
            base_object = sel[0].Object
            path_object = sel[1].Object

            count = 4
            extra = App.Vector(0, 0, 0)
            subelements = list(sel[1].SubElementNames)
            align = False
            align_mode = "Original"
            tan_vector = App.Vector(1, 0, 0)
            force_vertical = False
            vertical_vector = App.Vector(0, 0, 1)
            start_offset = 0.0
            end_offset = 0.0
            use_link = self.use_link

            _edge_list_str = list()
            _edge_list_str = ["'" + edge + "'" for edge in subelements]
            _sub_str = ", ".join(_edge_list_str)
            subelements_list_str = "[" + _sub_str + "]"

            vertical_vector_str = DraftVecUtils.toString(vertical_vector)

            Gui.addModule("Draft")
            _cmd = "Draft.make_path_array"
            _cmd += "("
            _cmd += "App.ActiveDocument." + base_object.Name + ", "
            _cmd += "App.ActiveDocument." + path_object.Name + ", "
            _cmd += "count=" + str(count) + ", "
            _cmd += "extra=" + DraftVecUtils.toString(extra) + ", "
            _cmd += "subelements=" + subelements_list_str + ", "
            _cmd += "align=" + str(align) + ", "
            _cmd += "align_mode=" + "'" + align_mode + "', "
            _cmd += "tan_vector=" + DraftVecUtils.toString(tan_vector) + ", "
            _cmd += "force_vertical=" + str(force_vertical) + ", "
            _cmd += "vertical_vector=" + vertical_vector_str + ", "
            _cmd += "start_offset=" + str(start_offset) + ", "
            _cmd += "end_offset=" + str(end_offset) + ", "
            _cmd += "use_link=" + str(use_link)
            _cmd += ")"

            _cmd_list = ["_obj_ = " + _cmd,
                         "Draft.autogroup(_obj_)",
                         "App.ActiveDocument.recompute()"]
            self.commit(translate("draft","Path array"), _cmd_list)

        # Commit the transaction and execute the commands
        # through the parent class
        self.finish()


Gui.addCommand('Draft_PathArray', PathArray())


class PathLinkArray(PathArray):
    """Gui Command for the PathLinkArray tool based on the PathArray tool."""

    def __init__(self):
        super(PathLinkArray, self).__init__(use_link=True)

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_PathLinkArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PathLinkArray", "Path Link array"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PathLinkArray", "Like the PathArray tool, but creates a 'Link array' instead.\nA 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.")}

    def Activated(self):
        """Execute when the command is called."""
        super(PathLinkArray, self).Activated(name="Path link array")


Gui.addCommand('Draft_PathLinkArray', PathLinkArray())

## @}
