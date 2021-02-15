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
"""Provides GUI tools to repair objects created with older versions."""
## @package gui_heal
# \ingroup draftguitools
# \brief Provides GUI tools to repair objects created with older versions.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft
import draftguitools.gui_base as gui_base

from draftutils.translate import translate


class Heal(gui_base.GuiCommandSimplest):
    """The Draft Heal command definition.

    Heal faulty Draft objects saved with an earlier version of the program.

    It inherits `GuiCommandSimplest` to set up the document
    and other behavior. See this class for more information.
    """

    def __init__(self):
        super(Heal, self).__init__(name=translate("draft","Heal"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = ()

        return {'Pixmap': 'Draft_Heal',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Heal", "Heal"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Heal", "Heal faulty Draft objects saved with an earlier version of the program.\nIf an object is selected it will try to heal that object in particular,\notherwise it will try to heal all objects in the active document.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Heal, self).Activated()

        s = Gui.Selection.getSelection()
        self.doc.openTransaction("Heal")
        if s:
            Draft.heal(s)
        else:
            Draft.heal()
        self.doc.commitTransaction()


Gui.addCommand('Draft_Heal', Heal())

## @}
