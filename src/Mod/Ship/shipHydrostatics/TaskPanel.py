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

import math
# FreeCAD modules
import FreeCAD as App
import FreeCADGui as Gui
# Qt library
from PyQt4 import QtGui,QtCore
# Module
import PlotAux
import Instance
from shipUtils import Paths
import Tools

class TaskPanel:
	def __init__(self):
		self.ui = Paths.modulePath() + "/shipHydrostatics/TaskPanel.ui"
		self.ship = None

	def accept(self):
		if not self.ship:
			return False
		self.save()
		draft  = self.form.minDraft.value()
		drafts = [draft]
		dDraft = (self.form.maxDraft.value() - self.form.minDraft.value())/(self.form.nDraft.value()-1)
		for i in range(1,self.form.nDraft.value()):
			draft = draft + dDraft
			drafts.append(draft)
		PlotAux.Plot(self.ship, self.form.trim.value(), drafts)
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
		form.trim = form.findChild(QtGui.QDoubleSpinBox, "Trim")
		form.minDraft = form.findChild(QtGui.QDoubleSpinBox, "MinDraft")
		form.maxDraft = form.findChild(QtGui.QDoubleSpinBox, "MaxDraft")
		form.nDraft = form.findChild(QtGui.QSpinBox, "NDraft")
		self.form = form
		# Initial values
		if self.initValues():
			return True
		self.retranslateUi()
		# Connect Signals and Slots
		QtCore.QObject.connect(form.trim, QtCore.SIGNAL("valueChanged(double)"), self.onData)
		QtCore.QObject.connect(form.minDraft, QtCore.SIGNAL("valueChanged(double)"), self.onData)
		QtCore.QObject.connect(form.maxDraft, QtCore.SIGNAL("valueChanged(double)"), self.onData)

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
		""" Set initial values for fields
		"""
		# Get objects
		selObjs  = Gui.Selection.getSelection()
		if not selObjs:
			msg = QtGui.QApplication.translate("ship_console", "Ship instance must be selected (no object selected)",
                                       None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg + '\n')
			return True
		for i in range(0,len(selObjs)):
			obj = selObjs[i]
			# Test if is a ship instance
			props = obj.PropertiesList
			try:
				props.index("IsShip")
			except ValueError:
				continue
			if obj.IsShip:
				# Test if another ship already selected
				if self.ship:
					msg = QtGui.QApplication.translate("ship_console", "More than one ship selected (extra ships will be neglected)",
                                       None,QtGui.QApplication.UnicodeUTF8)
					App.Console.PrintWarning(msg + '\n')
					break
				self.ship = obj
		# Test if any valid ship was selected
		if not self.ship:
			msg = QtGui.QApplication.translate("ship_console",
                                       "Ship instance must be selected (no valid ship found at selected objects)",
                                       None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg + '\n')
			return True
		# Get bounds
		bbox = self.ship.Shape.BoundBox
		# Set trim
		flag = True
		try:
			props.index("HydrostaticsTrim")
		except ValueError:
			flag = False
		if flag:
			self.form.trim.setValue(self.ship.HydrostaticsTrim)
		# Set drafts
		self.form.maxDraft.setValue(1.1*self.ship.Draft)
		self.form.minDraft.setValue(0.9*self.ship.Draft)
		# Try to use saved values
		props = self.ship.PropertiesList
		flag = True
		try:
			props.index("HydrostaticsMinDraft")
		except ValueError:
			flag = False
		if flag:
			self.form.minDraft.setValue(self.ship.HydrostaticsMinDraft)
		flag = True
		try:
			props.index("HydrostaticsMaxDraft")
		except ValueError:
			flag = False
		if flag:
			self.form.maxDraft.setValue(self.ship.HydrostaticsMaxDraft)
		self.form.maxDraft.setMaximum(bbox.ZMax)
		self.form.minDraft.setMinimum(bbox.ZMin)
		self.form.maxDraft.setMinimum(self.form.minDraft.value())
		self.form.minDraft.setMaximum(self.form.maxDraft.value())		
		flag = True
		try:
			props.index("HydrostaticsNDraft")
		except ValueError:
			flag = False
		if flag:
			self.form.nDraft.setValue(self.ship.HydrostaticsNDraft)
		# Update GUI
		return False

	def retranslateUi(self):
		""" Set user interface locale strings. 
		"""
		self.form.setWindowTitle(QtGui.QApplication.translate("ship_hydrostatic","Plot hydrostatics",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "TrimLabel").setText(QtGui.QApplication.translate("ship_hydrostatic","Trim",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "MinDraftLabel").setText(QtGui.QApplication.translate("ship_hydrostatic","Minimum draft",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "MaxDraftLabel").setText(QtGui.QApplication.translate("ship_hydrostatic","Maximum draft",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "NDraftLabel").setText(QtGui.QApplication.translate("ship_hydrostatic","Number of points",
                                 None,QtGui.QApplication.UnicodeUTF8))

	def onData(self, value):
		""" Method called when input data is changed.
		 @param value Changed value.
		"""
		if not self.ship:
			return
		self.form.maxDraft.setMinimum(self.form.minDraft.value())
		self.form.minDraft.setMaximum(self.form.maxDraft.value())		

	def save(self):
		""" Saves data into ship instance.
		"""
		props = self.ship.PropertiesList
		try:
			props.index("HydrostaticsTrim")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("ship_hydrostatic","Hydrostatics tool trim selected",
                                 None,QtGui.QApplication.UnicodeUTF8))
			self.ship.addProperty("App::PropertyFloat","HydrostaticsTrim","Ship", tooltip)
		self.ship.HydrostaticsTrim = self.form.trim.value()
		try:
			props.index("HydrostaticsMinDraft")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("ship_hydrostatic","Hydrostatics tool minimum draft selected [m]",
                                 None,QtGui.QApplication.UnicodeUTF8))
			self.ship.addProperty("App::PropertyFloat","HydrostaticsMinDraft","Ship", tooltip)
		self.ship.HydrostaticsMinDraft = self.form.minDraft.value()
		try:
			props.index("HydrostaticsMaxDraft")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("ship_hydrostatic","Hydrostatics tool maximum draft selected [m]",
                                 None,QtGui.QApplication.UnicodeUTF8))
			self.ship.addProperty("App::PropertyFloat","HydrostaticsMaxDraft","Ship", tooltip)
		self.ship.HydrostaticsMaxDraft = self.form.maxDraft.value()
		try:
			props.index("HydrostaticsNDraft")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("ship_hydrostatic","Hydrostatics tool number of points selected",
                                 None,QtGui.QApplication.UnicodeUTF8))
			self.ship.addProperty("App::PropertyInteger","HydrostaticsNDraft","Ship", tooltip)
		self.ship.HydrostaticsNDraft = self.form.nDraft.value()

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
