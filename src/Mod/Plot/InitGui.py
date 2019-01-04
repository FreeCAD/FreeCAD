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

import FreeCAD


class PlotWorkbench(Workbench):
    """Workbench of Plot module."""
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Plot/resources/icons/PlotWorkbench.svg"
        self.__class__.MenuText = "Plot"
        self.__class__.ToolTip = "The Plot module is used to edit/save output plots performed by other tools"

    from plotUtils import Paths
    import PlotGui

    def Initialize(self):
        from PySide import QtCore, QtGui
        cmdlst = ["Plot_SaveFig",
                  "Plot_Axes",
                  "Plot_Series",
                  "Plot_Grid",
                  "Plot_Legend",
                  "Plot_Labels",
                  "Plot_Positions"]
        self.appendToolbar(str(QtCore.QT_TRANSLATE_NOOP(
            "Plot",
            "Plot edition tools")), cmdlst)
        self.appendMenu(str(QtCore.QT_TRANSLATE_NOOP(
            "Plot",
            "Plot")), cmdlst)
        try:
            import matplotlib
        except ImportError:
            from PySide import QtCore, QtGui
            msg = QtGui.QApplication.translate(
                "plot_console",
                "matplotlib not found, Plot module will be disabled",
                None)
            FreeCAD.Console.PrintMessage(msg + '\n')


Gui.addWorkbench(PlotWorkbench())
