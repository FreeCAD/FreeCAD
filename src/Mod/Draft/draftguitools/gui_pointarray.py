# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2018 Benjamin Alterauge (ageeye)                        *
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
"""Provides GUI tools to create PointArray objects.

The copies will be created at the points of a point object.
"""
## @package gui_pointarray
# \ingroup draftguitools
# \brief Provides GUI tools to create PointArray objects.

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


class PointArray(gui_base_original.Modifier):
    """Gui Command for the Point array tool.

    Parameters
    ----------
    use_link: bool, optional
        It defaults to `False`. If it is `True`, the created object
        will be a `Point link array`.
    """

    def __init__(self, use_link=False):
        super(PointArray, self).__init__()
        self.use_link = use_link

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_PointArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PointArray", "Point array"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PointArray", "Creates copies of the selected base object at the points of a point object.\nFirst select the base object, and then select the point object.")}

    def Activated(self, name="Point array"):
        """Execute when the command is called."""
        super(PointArray, self).Activated(name=name)
        # This was deactivated because it doesn't work correctly;
        # the selection needs to be made on two objects, but currently
        # it only selects one.

        # if not Gui.Selection.getSelectionEx():
        #     if self.ui:
        #         self.ui.selectUi()
        #         _msg(translate("draft",
        #                        "Please select exactly two objects, "
        #                        "the base object and the point object, "
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
            _err(translate("draft","Please select exactly two objects, the base object and the point object, before calling this command."))
        else:
            base_object = sel[0].Object
            point_object = sel[1].Object
            extra = None

            Gui.addModule('Draft')
            _cmd = "Draft.make_point_array"
            _cmd += "("
            _cmd += "App.ActiveDocument." + base_object.Name + ", "
            _cmd += "App.ActiveDocument." + point_object.Name + ", "
            _cmd += "extra=" + str(extra) + ", "
            _cmd += 'use_link=' + str(self.use_link)
            _cmd += ")"

            _cmd_list = ["_obj_ = " + _cmd,
                         "Draft.autogroup(_obj_)",
                         "App.ActiveDocument.recompute()"]
            self.commit(translate("draft","Point array"), _cmd_list)

        # Commit the transaction and execute the commands
        # through the parent class
        self.finish()


Gui.addCommand('Draft_PointArray', PointArray())

class PointLinkArray(PointArray):
    """Gui Command for the PointLinkArray tool based on the PointArray tool."""

    def __init__(self):
        super(PointLinkArray, self).__init__(use_link=True)

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_PointLinkArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PointLinkArray", "PointLinkArray"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PointLinkArray", "Like the PointArray tool, but creates a 'Point link array' instead.\nA 'Point link array' is more efficient when handling many copies.")}

    def Activated(self):
        """Execute when the command is called."""
        super(PointLinkArray, self).Activated(name="Point link array")


Gui.addCommand('Draft_PointLinkArray', PointLinkArray())

## @}
