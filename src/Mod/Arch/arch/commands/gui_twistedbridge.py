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
"""Provides GUI tools to create TwistedBridge objects."""
## @package gui_twistedbridge
# \ingroup archguitools
# \brief Provides GUI tools to create TwistedBridge objects.

## \addtogroup archguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_base_original as gui_base_original

from draftutils.messages import _err
from draftutils.translate import _tr

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class TwistedBridge(gui_base_original.Modifier):
    """Gui Command for the Twisted bridge tool."""

    def __init__(self, use_link=False):
        super(TwistedBridge, self).__init__()
        self.call = None

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Twisted bridge"
        _tip = ("Creates a twisted bridge from a selected frame "
                "and a path.\n"
                "First select the object, and then select the path.\n"
                "The path can be a polyline, B-spline, Bezier curve, "
                "or even edges from other objects.")

        return {'Pixmap': 'Arch_TwistedBridge',
                'MenuText': QT_TRANSLATE_NOOP("Arch_TwistedBridge", _menu),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_TwistedBridge", _tip)}

    def Activated(self, name=_tr("Twisted bridge")):
        """Execute when the command is called."""
        super(TwistedBridge, self).Activated(name=name)
        self.name = name
        self.proceed()

    def proceed(self):
        """Proceed with the command if one object was selected."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)

        sel = Gui.Selection.getSelectionEx()
        if len(sel) != 2:
            _err(_tr("Please select exactly two objects, "
                     "the base object and the path object, "
                     "before calling this command."))
        else:
            base_object = sel[0].Object
            path_object = sel[1].Object

            count = 15
            rot_factor = 0.25
            width = 100
            thickness = 10

            Gui.addModule("Arch")
            Gui.addModule("Draft")
            _cmd = "Arch.make_twisted_bridge"
            _cmd += "("
            _cmd += "App.ActiveDocument." + base_object.Name + ", "
            _cmd += "App.ActiveDocument." + path_object.Name + ", "
            _cmd += "count=" + str(count) + ", "
            _cmd += "rot_factor=" + str(rot_factor) + ", "
            _cmd += "width=" + str(width) + ", "
            _cmd += "thickness=" + str(thickness)
            _cmd += ")"

            _cmd_list = ["_obj_ = " + _cmd,
                         "Draft.autogroup(_obj_)",
                         "App.ActiveDocument.recompute()"]
            self.commit(_tr(self.name), _cmd_list)

        # Commit the transaction and execute the commands
        # through the parent class
        self.finish()


Gui.addCommand('Arch_TwistedBridge', TwistedBridge())

## @}
