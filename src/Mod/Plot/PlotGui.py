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

from PyQt4 import QtCore, QtGui
import FreeCAD, FreeCADGui, os

# Setup tranlations
from plotUtils import Paths
path = Paths.translationsPath()
FreeCADGui.addLanguagePath(path)
import os
import FreeCAD
translator = QtCore.QTranslator()
dirList=os.listdir(path)
for fname in dirList:
	valid = translator.load(os.path.join(path, fname))
	if valid:
		QtGui.QApplication.installTranslator(translator)

class Save: 
	def Activated(self):
		import plotSave
		plotSave.load()

	def GetResources(self):
		from plotUtils import Paths
		IconPath = Paths.iconsPath() + "/Save.svg"
		MenuText = str(QtGui.QApplication.translate("plot", "Save plot").toAscii())
		ToolTip  = str(QtGui.QApplication.translate("plot", "Save plot as image file").toAscii())
		return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class Axes: 
	def Activated(self):
		import plotAxes
		plotAxes.load()

	def GetResources(self):
		from plotUtils import Paths
		IconPath = Paths.iconsPath() + "/Axes.svg"
		MenuText = str(QtGui.QApplication.translate("plot", "Configure axes").toAscii())
		ToolTip  = str(QtGui.QApplication.translate("plot", "Configure axes parameters").toAscii())
		return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class Series: 
	def Activated(self):
		import plotSeries
		plotSeries.load()

	def GetResources(self):
		from plotUtils import Paths
		IconPath = Paths.iconsPath() + "/Series.svg"
		MenuText = str(QtGui.QApplication.translate("plot", "Configure series").toAscii())
		ToolTip  = str(QtGui.QApplication.translate("plot", "Configure series drawing style and label").toAscii())
		return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class Grid: 
	def Activated(self):
		import Plot
		plt = Plot.getPlot()
		if not plt:
			msg = str(QtGui.QApplication.translate("plot", "Grid must be activated on top of a plot document").toAscii())
			FreeCAD.Console.PrintError(msg+"\n")
			return
		flag = plt.isGrid()
		Plot.grid(not flag)

	def GetResources(self):
		from plotUtils import Paths
		IconPath = Paths.iconsPath() + "/Grid.svg"
		MenuText = str(QtGui.QApplication.translate("plot", "Show/Hide grid").toAscii())
		ToolTip  = str(QtGui.QApplication.translate("plot", "Show/Hide grid on selected plot").toAscii())
		return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class Legend: 
	def Activated(self):
		import Plot
		plt = Plot.getPlot()
		if not plt:
			msg = str(QtGui.QApplication.translate("plot", "Legend must be activated on top of a plot document").toAscii())
			FreeCAD.Console.PrintError(msg+"\n")
			return
		flag = plt.isLegend()
		Plot.legend(not flag)

	def GetResources(self):
		from plotUtils import Paths
		IconPath = Paths.iconsPath() + "/Legend.svg"
		MenuText = str(QtGui.QApplication.translate("plot", "Show/Hide legend").toAscii())
		ToolTip  = str(QtGui.QApplication.translate("plot", "Show/Hide legend on selected plot").toAscii())
		return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class Labels: 
	def Activated(self):
		import plotLabels
		plotLabels.load()

	def GetResources(self):
		from plotUtils import Paths
		IconPath = Paths.iconsPath() + "/Labels.svg"
		MenuText = str(QtGui.QApplication.translate("plot", "Set labels").toAscii())
		ToolTip  = str(QtGui.QApplication.translate("plot", "Set title and axes labels").toAscii())
		return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

class Positions: 
	def Activated(self):
		import plotPositions
		plotPositions.load()

	def GetResources(self):
		from plotUtils import Paths
		IconPath = Paths.iconsPath() + "/Positions.svg"
		MenuText = str(QtGui.QApplication.translate("plot", "Set positions and sizes").toAscii())
		ToolTip  = str(QtGui.QApplication.translate("plot", "Set labels and legend positions and sizes").toAscii())
		return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip} 

FreeCADGui.addCommand('Plot_SaveFig', Save())
FreeCADGui.addCommand('Plot_Axes', Axes())
FreeCADGui.addCommand('Plot_Series', Series())
FreeCADGui.addCommand('Plot_Grid', Grid())
FreeCADGui.addCommand('Plot_Legend', Legend())
FreeCADGui.addCommand('Plot_Labels', Labels())
FreeCADGui.addCommand('Plot_Positions', Positions())

