# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
"""Provides GUI tools to create Layer objects."""
## @package gui_layers
# \ingroup draftguitools
# \brief Provides GUI tools to create Layer objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_base as gui_base

from draftutils.translate import _tr

# The module is used to prevent complaints from code checkers (flake8)
bool(Draft_rc.__name__)


class Layer(gui_base.GuiCommandSimplest):
    """GuiCommand to create a Layer object in the document."""

    def __init__(self):
        super(Layer, self).__init__(name=_tr("Layer"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = QT_TRANSLATE_NOOP("Draft_Layer",
                                 "Adds a layer to the document.\n"
                                 "Objects added to this layer can share "
                                 "the same visual properties such as "
                                 "line color, line width, and shape color.")
        return {'Pixmap': 'Draft_Layer',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Layer", "Layer"),
                'ToolTip': _tip}

    def Activated(self):
        """Execute when the command is called.

        It calls the `finish(False)` method of the active Draft command.
        """
        super(Layer, self).Activated()

        self.doc.openTransaction("Create Layer")
        Gui.addModule("Draft")
        Gui.doCommand('_layer_ = Draft.make_layer()')
        Gui.doCommand('FreeCAD.ActiveDocument.recompute()')
        self.doc.commitTransaction()


Gui.addCommand('Draft_Layer', Layer())

## @}
