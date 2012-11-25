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

# FreeCAD modules
import FreeCAD as App
import FreeCADGui as Gui
import Part
# Qt library
from PyQt4 import QtGui,QtCore
# Module
from TankInstance import *
from shipUtils import Paths

class TaskPanel:
	def __init__(self):
		self.ui = Paths.modulePath() + "/tankCreateTank/TaskPanel.ui"

	def accept(self):
		# Create new ship instance
		obj = App.ActiveDocument.addObject("Part::FeaturePython","Tank")
		ShipTank(obj, self.solid, self.form.level.value(), self.form.dens.value())
		if not obj.IsShipTank:
			msg = QtGui.QApplication.translate("ship_console", "Tank has not been created",
                                       None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg + '\n')
		ViewProviderShipTank(obj.ViewObject)
		App.ActiveDocument.recompute()
		return True

	def reject(self):
		return True

	def clicked(self, index):
		pass

	def open(self):
		pass

	def needsFullSpace(self):
		return True

	def isAllowedAlterSelection(self):
		return False

	def isAllowedAlterView(self):
		return True

	def isAllowedAlterDocument(self):
		return False

	def helpRequested(self):
		pass

	def setupUi(self):
		mw = self.getMainWindow()
		form = mw.findChild(QtGui.QWidget, "TaskPanel")
		form.level = form.findChild(QtGui.QDoubleSpinBox, "Level")
		form.dens  = form.findChild(QtGui.QDoubleSpinBox, "Density")
		self.form = form
		# Initial values
		if self.initValues():
			return True
		self.retranslateUi()
		# Connect Signals and Slots
		QtCore.QObject.connect(form.level, QtCore.SIGNAL("valueChanged(double)"), self.onLevel)
		QtCore.QObject.connect(form.dens , QtCore.SIGNAL("valueChanged(double)"), self.onDens)

	def getMainWindow(self):
		"returns the main window"
		# using QtGui.qApp.activeWindow() isn't very reliable because if another
		# widget than the mainwindow is active (e.g. a dialog) the wrong widget is
		# returned
		toplevel = QtGui.qApp.topLevelWidgets()
		for i in toplevel:
			if i.metaObject().className() == "Gui::MainWindow":
				return i
		raise Exception("No main window found")

	def initValues(self):
		""" Get selected geometry.
		@return False if sucessfully find valid geometry.
		"""
		self.solid = None
		solids	 = []
		selObjs	= Gui.Selection.getSelection()
		if not selObjs:
			msg = QtGui.QApplication.translate("ship_console",
                                       "Tank objects can only be created on top of structure geometry (no object selected)",
                                       None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg + '\n')
			msg = QtGui.QApplication.translate("ship_console",
                                       "Please create a tank geometry before using this tool",
                                       None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg + '\n')
			return True
		for i in range(0, len(selObjs)):
			solid = selObjs[i]
			if solid.isDerivedFrom('Part::Feature'):
				# Get shape
				shape = solid.Shape
				if not shape:
					continue
				solid = shape
			if not solid.isDerivedFrom('Part::TopoShape'):
				return None
			# Get shells
			shells = solid.Shells
			if not shells:
				continue
			# Build solids
			for s in shells:
				solids.append(Part.Solid(s))
		if not solids:
			msg = QtGui.QApplication.translate("ship_console",
                                       "Tank objects can only be created on top of structure geometry (no solids can't be computed)",
                                       None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg + '\n')
			msg = QtGui.QApplication.translate("ship_console",
                                       "Please create a tank geometry before using this tool",
                                       None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg + '\n')
			return True
		self.solid = Part.CompSolid(solids)
		return False

	def retranslateUi(self):
		""" Set user interface locale strings. 
		"""
		self.form.setWindowTitle(QtGui.QApplication.translate("shiptank_create","Create a new tank",
                                 None,QtGui.QApplication.UnicodeUTF8))
		name = QtGui.QApplication.translate("shiptank_create","Filling level", None,QtGui.QApplication.UnicodeUTF8) + " (%)"
		self.form.findChild(QtGui.QLabel, "LevelLabel").setText(name)
		name = '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd">\n<html><body>'
		name = name + QtGui.QApplication.translate("shiptank_create","Fluid density", None,QtGui.QApplication.UnicodeUTF8)
		name = name + '(kg/m<span style=" vertical-align:super;">3</span>)</body></html>'		
		self.form.findChild(QtGui.QLabel, "DensityLabel").setText(name)

	def onLevel(self, value):
		""" Method called when tank filling level has been modified.
		@param value Changed value.
		"""
		pass

	def onDens(self, value):
		""" Method called when fluid density has been modified.
		@param value Changed value.
		"""
		pass

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
