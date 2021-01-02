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

The copies will be created where various points are located.

The points need to be grouped under a compound before using this tool.
To create this compound, select various points and then use the Draft Upgrade
tool to create a `Block`, or use a `Part::Compound`.
You can also create a Sketch, and place explicit points.

Other geometrical objects may be contained inside the compounds but only
the explicit point and vertex objects will be used when creating
the point array.
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
from draftutils.translate import _tr

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
        _menu = "Point array"
        _tip = ("Creates copies of the selected object, "
                "and places the copies at the position of various points.\n"
                "\n"
                "The points need to be grouped under a compound of points "
                "before using this tool.\n"
                "To create this compound, select various points "
                "and then use the Part Compound tool,\n"
                "or use the Draft Upgrade tool to create a 'Block', "
                "or create a Sketch and add simple points to it.\n"
                "\n"
                "Select the base object, and then select the compound "
                "or the sketch to create the point array.")

        return {'Pixmap': 'Draft_PointArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PointArray", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PointArray", _tip)}

    def Activated(self, name="Point array"):
        """Execute when the command is called."""
        self.name = name
        super(PointArray, self).Activated(name=_tr(self.name))
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
            _err(_tr("Please select exactly two objects, "
                     "the base object and the point object, "
                     "before calling this command."))
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
            self.commit(_tr(self.name), _cmd_list)

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
        _tip = ("Like the PointArray tool, but creates a 'Point link array' instead.\n"
                "A 'Point link array' is more efficient when handling many copies.")

        return {'Pixmap': 'Draft_PointLinkArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_PointLinkArray", "PointLinkArray"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_PointLinkArray", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(PointLinkArray, self).Activated(name="Point link array")


Gui.addCommand('Draft_PointLinkArray', PointLinkArray())

## @}
