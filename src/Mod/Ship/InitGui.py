#***************************************************************************
#*																		 *
#*   Copyright (c) 2011, 2012											  *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>							*  
#*																		 *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)	*
#*   as published by the Free Software Foundation; either version 2 of	 *
#*   the License, or (at your option) any later version.				   *
#*   for detail see the LICENCE text file.								 *
#*																		 *
#*   This program is distributed in the hope that it will be useful,	   *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of		*
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the		 *
#*   GNU Library General Public License for more details.				  *
#*																		 *
#*   You should have received a copy of the GNU Library General Public	 *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA																   *
#*																		 *
#***************************************************************************

class ShipWorkbench ( Workbench ):
	""" @brief Workbench of Ship design module. Here toolbars & icons are append. """
	from PyQt4 import QtCore, QtGui
	from shipUtils import Paths
	import ShipGui

	Icon = "Ico"
	MenuText = str(QtCore.QT_TRANSLATE_NOOP("Ship", "Ship"))
	ToolTip = str(QtCore.QT_TRANSLATE_NOOP("Ship", "Ship module provides some of the commonly used tool to design ship forms"))

	def Initialize(self):
		from PyQt4 import QtCore, QtGui

		# Print a warning if Plot module can't be used
		try:
			import Plot
		except ImportError:
			msg = QtGui.QApplication.translate("ship_console", "Plot module is disabled, tools can't graph output curves",
                                       None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintMessage(msg + '\n')
		# ToolBar
		shiplist    = ["Ship_LoadExample", "Ship_CreateShip", "Ship_OutlineDraw", "Ship_AreasCurve", "Ship_Hydrostatics"]
		weightslist = ["Ship_Weights", "Ship_CreateTank", "Ship_GZ"]
		self.appendToolbar(str(QtCore.QT_TRANSLATE_NOOP("Ship", "Ship design")),shiplist)
		self.appendToolbar(str(QtCore.QT_TRANSLATE_NOOP("Ship", "Weights")),weightslist)
		self.appendMenu(str(QtCore.QT_TRANSLATE_NOOP("Ship", "Ship design")),shiplist)
		self.appendMenu(str(QtCore.QT_TRANSLATE_NOOP("Ship", "Weights")),weightslist)
		# Simulation stuff only if pyOpenCL & numpy are present
		hasOpenCL = True
		hasNumpy  = True
		hasSim    = False  # In development, activate it only for development purposes
		try:
			import pyopencl
		except ImportError:
			hasOpenCL = False
			msg = QtGui.QApplication.translate("ship_console", "pyOpenCL not installed, simulations stuff will disabled therefore",
                                       None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintMessage(msg + '\n')
		try:
			import numpy
		except ImportError:
			hasNumpy = False
			msg = QtGui.QApplication.translate("ship_console", "numpy not installed, simulations stuff will disabled therefore",
                                       None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintMessage(msg + '\n')
		if hasOpenCL and hasNumpy and hasSim:
			simlist = ["Ship_CreateSim", "Ship_RunSim", "Ship_StopSim", "Ship_TrackSim"]
			self.appendToolbar(str(QtCore.QT_TRANSLATE_NOOP("Ship", "Simulation")),simlist)
			self.appendMenu(str(QtCore.QT_TRANSLATE_NOOP("Ship", "Simulation")),simlist)

Gui.addWorkbench(ShipWorkbench())
