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
"""Provides GUI tools to create parametric Array objects (OBSOLETE).

These commands were replaced by individual commands `Draft_OrthoArray`,
`Draft_PolarArray`, and `Draft_CircularArray` which launch their own
task panel, and provide a more useful way of creating the desired array.
"""
## @package gui_array_simple
# \ingroup draftguitools
# \brief Provides GUI tools to create parametric Array objects (OBSOLETE).

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
from draftutils.messages import _msg
from draftutils.translate import translate, _tr

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Array(gui_base_original.Modifier):
    """Gui Command for the original simple Array tool.

    Parameters
    ----------
    use_link: bool, optional
        It defaults to `False`. If it is `True`, the created object
        will be a `Link array`.
    """

    def __init__(self, use_link=False):
        super(Array, self).__init__()
        self.use_link = use_link

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = ("Creates an array from a selected object. "
                "By default, it is a 2x2 orthogonal array.\n"
                "Once the array is created its type can be changed "
                "to polar or circular, and its properties can be modified.")

        return {'Pixmap': 'Draft_Array',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Array", "Array"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Array", _tip)}

    def Activated(self, name=_tr("Array")):
        """Execute when the command is called."""
        super(Array, self).Activated(name=name)
        if not Gui.Selection.getSelection():
            if self.ui:
                self.ui.selectUi()
                _msg(translate("draft", "Select an object to array"))
                self.call = self.view.addEventCallback(
                    "SoEvent",
                     gui_tool_utils.selectObject)
        else:
            self.proceed()

    def proceed(self):
        """Proceed with the command if one object was selected."""
        if Gui.Selection.getSelection():
            obj = Gui.Selection.getSelection()[0]
            Gui.addModule("Draft")
            _cmd = 'Draft.makeArray'
            _cmd += '('
            _cmd += 'FreeCAD.ActiveDocument.' + obj.Name + ', '
            _cmd += 'FreeCAD.Vector(1, 0, 0), '
            _cmd += 'FreeCAD.Vector(0, 1, 0), '
            _cmd += '2, '
            _cmd += '2, '
            _cmd += 'use_link=' + str(self.use_link)
            _cmd += ')'
            _cmd_list = ['obj = ' + _cmd,
                         'Draft.autogroup(obj)',
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Array"),
                        _cmd_list)
        self.finish()


Gui.addCommand('Draft_Array', Array())


class LinkArray(Array):
    """Gui Command for the LinkArray tool based on the simple Array tool."""

    def __init__(self):
        super(LinkArray, self).__init__(use_link=True)

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = ("Like the Array tool, but creates a 'Link array' instead.\n"
                "A 'Link array' is more efficient when handling many copies "
                "but the 'Fuse' option cannot be used.")

        return {'Pixmap': 'Draft_LinkArray',
                'MenuText': QT_TRANSLATE_NOOP("Draft_LinkArray", "LinkArray"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_LinkArray", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        super(LinkArray, self).Activated(name=_tr("Link array"))


Gui.addCommand('Draft_LinkArray', LinkArray())

## @}
