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

import FreeCAD


class ShipWorkbench(Workbench):
    """Ships design workbench."""
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Ship/resources/icons/ShipWorkbench.svg"
        self.__class__.MenuText = "Ship"
        self.__class__.ToolTip = "Ship module provides some of the commonly used tool to design ship forms"

    from shipUtils import Paths
    import ShipGui

    def Initialize(self):
        from PySide import QtCore, QtGui

        try:
            import Plot
        except ImportError:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Plot module is disabled, tools cannot graph output curves",
                None)
            FreeCAD.Console.PrintMessage(msg + '\n')
        # ToolBar
        shiplist = ["Ship_LoadExample",
                    "Ship_CreateShip",
                    "Ship_OutlineDraw",
                    "Ship_AreasCurve",
                    "Ship_Hydrostatics"]
        weightslist = ["Ship_Weight",
                       "Ship_Tank",
                       "Ship_Capacity",
                       "Ship_LoadCondition",
                       "Ship_GZ"]

        self.appendToolbar(
            str(QtCore.QT_TRANSLATE_NOOP("Ship", "Ship design")),
            shiplist)
        self.appendToolbar(
            str(QtCore.QT_TRANSLATE_NOOP("Ship", "Weights")),
            weightslist)
        self.appendMenu(
            str(QtCore.QT_TRANSLATE_NOOP("Ship", "Ship design")),
            shiplist)
        self.appendMenu(
            str(QtCore.QT_TRANSLATE_NOOP("Ship", "Weights")),
            weightslist)

Gui.addWorkbench(ShipWorkbench())
