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
import Preview, PlotAux
import Instance
from shipUtils import Paths
from shipHydrostatics import Tools as Hydrostatics

class TaskPanel:
	def __init__(self):
		self.ui = Paths.modulePath() + "/shipAreasCurve/TaskPanel.ui"
		self.preview = Preview.Preview()
		self.ship = None

	def accept(self):
		if not self.ship:
			return False
		self.save()
		# Plot data
		data = Hydrostatics.displacement(self.ship,self.form.draft.value(),0.0,self.form.trim.value())
		disp = data[0]
		xcb  = data[1].x
		data = Hydrostatics.areas(self.ship,self.form.draft.value(),0.0,self.form.trim.value())
		x	= []
		y	= []
		for i in range(0,len(data)):
			x.append(data[i][0])
			y.append(data[i][1])
		PlotAux.Plot(x,y,disp,xcb, self.ship)
		self.preview.clean()
		return True

	def reject(self):
		self.preview.clean()
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
		form.draft = form.findChild(QtGui.QDoubleSpinBox, "Draft")
		form.trim = form.findChild(QtGui.QDoubleSpinBox, "Trim")
		form.output = form.findChild(QtGui.QTextEdit, "OutputData")
		form.doc = QtGui.QTextDocument(form.output)
		self.form = form
		# Initial values
		if self.initValues():
			return True
		self.retranslateUi()
		# Connect Signals and Slots
		QtCore.QObject.connect(form.draft, QtCore.SIGNAL("valueChanged(double)"), self.onData)
		QtCore.QObject.connect(form.trim, QtCore.SIGNAL("valueChanged(double)"), self.onData)

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
		self.form.draft.setMaximum(bbox.ZMax)
		self.form.draft.setMinimum(bbox.ZMin)
		self.form.draft.setValue(self.ship.Draft)
		# Try to use saved values
		props = self.ship.PropertiesList
		flag = True
		try:
			props.index("AreaCurveDraft")
		except ValueError:
			flag = False
		if flag:
			self.form.draft.setValue(self.ship.AreaCurveDraft)
		flag = True
		try:
			props.index("AreaCurveTrim")
		except ValueError:
			flag = False
		if flag:
			self.form.trim.setValue(self.ship.AreaCurveTrim)
		# Update GUI
		self.preview.update(self.form.draft.value(), self.form.trim.value(), self.ship)
		self.onUpdate()
		return False

	def retranslateUi(self):
		""" Set user interface locale strings. 
		"""
		self.form.setWindowTitle(QtGui.QApplication.translate("ship_areas","Plot transversal areas curve",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "DraftLabel").setText(QtGui.QApplication.translate("ship_areas","Draft",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "TrimLabel").setText(QtGui.QApplication.translate("ship_areas","Trim",
                                 None,QtGui.QApplication.UnicodeUTF8))

	def onData(self, value):
		""" Method called when input data is changed.
		 @param value Changed value.
		"""
		if not self.ship:
			return
		self.onUpdate()
		self.preview.update(self.form.draft.value(), self.form.trim.value(), self.ship)

	def onUpdate(self):
		""" Method called when update data request.
		"""
		if not self.ship:
			return
		# Calculate drafts
		angle = math.radians(self.form.trim.value())
		L = self.ship.Length
		draftAP = self.form.draft.value() + 0.5*L*math.tan(angle)
		if draftAP < 0.0:
			draftAP = 0.0
		draftFP = self.form.draft.value() - 0.5*L*math.tan(angle)
		if draftFP < 0.0:
			draftFP = 0.0
		# Calculate hydrostatics involved
		data = Hydrostatics.displacement(self.ship,self.form.draft.value(),0.0,self.form.trim.value())
		# Prepare the string in html format
		string = 'L = %g [m]<BR>' % (self.ship.Length)
		string = string + 'B = %g [m]<BR>' % (self.ship.Beam)
		string = string + 'T = %g [m]<HR>' % (self.form.draft.value())
		string = string + 'Trim = %g [degrees]<BR>' % (self.form.trim.value())
		string = string + 'T<sub>AP</sub> = %g [m]<BR>' % (draftAP)
		string = string + 'T<sub>FP</sub> = %g [m]<HR>' % (draftFP)
		dispText = QtGui.QApplication.translate("ship_areas",'Displacement',
                                 None,QtGui.QApplication.UnicodeUTF8)
		string = string + dispText + ' = %g [ton]<BR>' % (data[0])
		string = string + 'XCB = %g [m]' % (data[1].x)
		# Set the document
		self.form.output.setHtml(string)

	def save(self):
		""" Saves data into ship instance.
		"""
		props = self.ship.PropertiesList
		try:
			props.index("AreaCurveDraft")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("ship_areas","Areas curve tool draft selected [m]",
                                 None,QtGui.QApplication.UnicodeUTF8))
			self.ship.addProperty("App::PropertyFloat","AreaCurveDraft","Ship", tooltip)
		self.ship.AreaCurveDraft = self.form.draft.value()
		try:
			props.index("AreaCurveTrim")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("ship_areas","Areas curve tool trim selected",
                                 None,QtGui.QApplication.UnicodeUTF8))
			self.ship.addProperty("App::PropertyFloat","AreaCurveTrim","Ship", tooltip)
		self.ship.AreaCurveTrim = self.form.trim.value()

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
