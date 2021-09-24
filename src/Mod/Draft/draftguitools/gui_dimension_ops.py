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
"""Provides GUI tools to modify dimension objects.

For example, a tool to flip the direction of the text in the dimension
as the normal is sometimes not correctly calculated automatically.
"""
## @package gui_dimension_ops
# \ingroup draftguitools
# \brief Provides GUI tools to modify dimension objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import draftutils.utils as utils
import draftguitools.gui_base as gui_base

from draftutils.translate import translate


class FlipDimension(gui_base.GuiCommandNeedsSelection):
    """The Draft FlipDimension command definition.

    Flip the normal direction of the selected dimensions.

    It inherits `GuiCommandNeedsSelection` to set up the document
    and other behavior. See this class for more information.
    """

    def __init__(self):
        super(Draft_FlipDimension, self).__init__(name=translate("draft","Flip dimension"))

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_FlipDimension',
                'MenuText': QT_TRANSLATE_NOOP("Draft_FlipDimension",
                                              "Flip dimension"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_FlipDimension",
                                             "Flip the normal direction of the selected dimensions (linear, radial, angular).\nIf other objects are selected they are ignored.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft_FlipDimension, self).Activated()

        for o in Gui.Selection.getSelection():
            if utils.get_type(o) in ("Dimension",
                                     "LinearDimension", "AngularDimension"):
                self.doc.openTransaction("Flip dimension")
                _cmd = "App.activeDocument()." + o.Name + ".Normal"
                _cmd += " = "
                _cmd += "App.activeDocument()." + o.Name + ".Normal.negative()"
                Gui.doCommand(_cmd)
                self.doc.commitTransaction()
                self.doc.recompute()


Draft_FlipDimension = FlipDimension
Gui.addCommand('Draft_FlipDimension', FlipDimension())

## @}
