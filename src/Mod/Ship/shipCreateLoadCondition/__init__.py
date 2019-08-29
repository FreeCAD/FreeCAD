#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2016                                              *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtGui
from . import Tools


READ_ONLY_FOREGROUND = (0.5, 0.5, 0.5)
READ_ONLY_BACKGROUND = (0.9, 0.9, 0.9)


def load():
    """Directly create the load condition"""
    # Check that a ship has been selected
    ship = None
    selObjs = Gui.Selection.getSelection()
    if not selObjs:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "A ship instance must be selected before using this tool (no"
            " objects selected)",
            None)
        App.Console.PrintError(msg + '\n')
        return
    for i in range(len(selObjs)):
        obj = selObjs[i]
        props = obj.PropertiesList
        try:
            props.index("IsShip")
        except ValueError:
            continue
        if obj.IsShip:
            if ship:
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "More than one ship have been selected (the extra"
                    " ships will be ignored)",
                    None)
                App.Console.PrintWarning(msg + '\n')
                break
            ship = obj

    if not ship:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "A ship instance must be selected before using this tool (no"
            " valid ship found at the selected objects)",
            None)
        App.Console.PrintError(msg + '\n')
        return

    Tools.createLoadCondition(ship)
