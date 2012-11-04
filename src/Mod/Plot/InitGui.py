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

class PlotWorkbench ( Workbench ):
	""" @brief Workbench of Plot module. Here toolbars & icons are append. """
	from plotUtils import Paths
	import PlotGui

	Icon = Paths.iconsPath() + "/Icon.svg"
	MenuText = "Plot"
	ToolTip = "The Plot module is used to edit/save output plots performed by other tools"

	def Initialize(self):
		# ToolBar
		list = ["Plot_SaveFig", "Plot_Axes", "Plot_Series", "Plot_Grid", "Plot_Legend", "Plot_Labels", "Plot_Positions"]
		self.appendToolbar("Plot",list)

		# Menu
		list = ["Plot_SaveFig", "Plot_Axes", "Plot_Series", "Plot_Grid", "Plot_Legend", "Plot_Labels", "Plot_Positions"]
		self.appendMenu("Plot",list)

try:
	import matplotlib
	Gui.addWorkbench(PlotWorkbench())
except ImportError:
	msg = QtCore.QT_TRANSLATE_NOOP("plot", "matplotlib not found, Plot module will be disabled")
	FreeCAD.Console.PrintMessage(msg + '\n')

