# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2010 Ken Cline <cline@frii.com>                                   *
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
"""Provides GUI tools to apply styles to objects."""
## @package gui_styles
# \ingroup draftguitools
# \brief Provides GUI tools to apply styles to objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import draftguitools.gui_base_original as gui_base_original

from draftutils.translate import translate


class ApplyStyle(gui_base_original.Modifier):
    """Gui Command for the ApplyStyle tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Apply',
                'MenuText': QT_TRANSLATE_NOOP("Draft_ApplyStyle", "Apply current style"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_ApplyStyle", "Applies the current style defined in the toolbar (line width and colors) to the selected objects and groups.")}

    def Activated(self):
        """Execute when the command is called.

        Activate the specific BSpline tracker.
        """
        super(ApplyStyle, self).Activated(name="Apply style")
        if self.ui:
            self.sel = Gui.Selection.getSelection()
            if len(self.sel) > 0:
                Gui.addModule("Draft")
                _cmd_list = []
                for obj in self.sel:
                    # TODO: instead of `TypeId`, use `utils.get_type`
                    # to get the type of the object and apply different
                    # formatting information depending on the type of object.
                    # The groups may also be things like `App::Parts`
                    # or `Arch_BuildingParts`.
                    if obj.TypeId == "App::DocumentObjectGroup":
                        _cmd_list.extend(self.formatGroup(obj))
                    else:
                        _cmd = 'Draft.formatObject'
                        _cmd += '('
                        _cmd += 'FreeCAD.ActiveDocument.' + obj.Name
                        _cmd += ')'
                        _cmd_list.append(_cmd)
                self.commit(translate("draft", "Change Style"),
                            _cmd_list)
            self.finish()

    def formatGroup(self, group):
        """Format a group instead of simple object."""
        Gui.addModule("Draft")
        _cmd_list = []
        for obj in group.Group:
            if obj.TypeId == "App::DocumentObjectGroup":
                _cmd_list.extend(self.formatGroup(obj))
            else:
                _cmd = 'Draft.formatObject'
                _cmd += '('
                _cmd += 'FreeCAD.ActiveDocument.' + obj.Name
                _cmd += ')'
                _cmd_list.append(_cmd)
        return _cmd_list


Gui.addCommand('Draft_ApplyStyle', ApplyStyle())

## @}
