#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *
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


class ShipWorkbench(Workbench):
    """Ships design workbench."""
    from shipUtils import Paths
    import ShipGui

    Icon = "Ship_Module.svg"
    MenuText = "Ship"
    ToolTip = ("Ship module provides some of the commonly used tool to design"
               " ship forms")

    def Initialize(self):
        from PySide import QtCore, QtGui

        try:
            import Plot
        except ImportError:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Plot module is disabled, tools cannot graph output curves",
                None,
                QtGui.QApplication.UnicodeUTF8)
            FreeCAD.Console.PrintMessage(msg + '\n')
        # ToolBar
        """
        shiplist = ["Ship_LoadExample",
                    "Ship_CreateShip",
                    "Ship_OutlineDraw",
                    "Ship_AreasCurve",
                    "Ship_Hydrostatics"]
        weightslist = ["Ship_Weights",
                       "Ship_CreateTank",
                       "Ship_GZ"]
        """
        shiplist = ["Ship_LoadExample",
                    "Ship_CreateShip",
                    "Ship_OutlineDraw",
                    "Ship_AreasCurve"]
        weightslist = []
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

        # Simulation tools will be added only if pyOpenCL & numpy are present
        # Simulations are in development, so keep it disabled except for
        # development purposes
        hasSim = False
        if hasSim:
            hasOpenCL = True
            hasNumpy = True
            try:
                import pyopencl
            except ImportError:
                hasOpenCL = False
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "pyOpenCL not installed, simulation tools will disabled"
                    " therefore",
                    None,
                    QtGui.QApplication.UnicodeUTF8)
                FreeCAD.Console.PrintMessage(msg + '\n')
            try:
                import numpy
            except ImportError:
                hasNumpy = False
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "numpy not installed, simulation tools will disabled"
                    " therefore",
                    None,
                    QtGui.QApplication.UnicodeUTF8)
                FreeCAD.Console.PrintMessage(msg + '\n')
            if hasOpenCL and hasNumpy:
                simlist = ["Ship_CreateSim",
                           "Ship_RunSim",
                           "Ship_StopSim",
                           "Ship_TrackSim"]
                self.appendToolbar(
                    str(QtCore.QT_TRANSLATE_NOOP("Ship", "Simulation")),
                    simlist)
                self.appendMenu(
                    str(QtCore.QT_TRANSLATE_NOOP("Ship", "Simulation")),
                    simlist)


Gui.addWorkbench(ShipWorkbench())
