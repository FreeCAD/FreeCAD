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

import math
import FreeCAD as App
import FreeCADGui as Gui
import Units
from PyQt4 import QtGui,QtCore
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
		if self.initValues():
			return True
		self.retranslateUi()
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
		selObjs  = Gui.Selection.getSelection()
		if not selObjs:
			msg = QtGui.QApplication.translate("ship_console",
			                           "A ship instance must be selected before using this tool (no objects selected)",
                                       None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg + '\n')
			return True
		for i in range(0,len(selObjs)):
			obj = selObjs[i]
			props = obj.PropertiesList
			try:
				props.index("IsShip")
			except ValueError:
				continue
			if obj.IsShip:
				if self.ship:
					msg = QtGui.QApplication.translate("ship_console",
					                           "More than one ship have been selected (the extra ships will be ignored)",
				                               None,QtGui.QApplication.UnicodeUTF8)
					App.Console.PrintWarning(msg + '\n')
					break
				self.ship = obj
		if not self.ship:
			msg = QtGui.QApplication.translate("ship_console",
                                       "A ship instance must be selected before using this tool (no valid ship found at the selected objects)",
                                       None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg + '\n')
			return True
		# Get the bounds for the tools
		bbox = self.ship.Shape.BoundBox
		self.form.draft.setMaximum(bbox.ZMax/Units.translateUnit('m'))
		self.form.draft.setMinimum(bbox.ZMin/Units.translateUnit('m'))
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
		""" Set user interface locale strings. """
		self.form.setWindowTitle(QtGui.QApplication.translate("ship_areas","Plot the transversal areas curve",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "DraftLabel").setText(QtGui.QApplication.translate("ship_areas","Draft",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "TrimLabel").setText(QtGui.QApplication.translate("ship_areas","Trim",
                                 None,QtGui.QApplication.UnicodeUTF8))

	def onData(self, value):
		""" Method called when the tool input data is touched.
		@param value Changed value.
		"""
		if not self.ship:
			return
		self.onUpdate()
		self.preview.update(self.form.draft.value(), self.form.trim.value(), self.ship)

	def onUpdate(self):
		""" Method called when the data update is requested. """
		if not self.ship:
			return
		# Calculate the drafts at each perpendicular
		angle = math.radians(self.form.trim.value())
		L = self.ship.Length
		draftAP = self.form.draft.value() + 0.5*L*math.tan(angle)
		if draftAP < 0.0:
			draftAP = 0.0
		draftFP = self.form.draft.value() - 0.5*L*math.tan(angle)
		if draftFP < 0.0:
			draftFP = 0.0
		# Calculate the involved hydrostatics
		data = Hydrostatics.displacement(self.ship,self.form.draft.value(),0.0,self.form.trim.value())
		# Setup the html string
		string = 'L = {0} [m]<BR>'.format(self.ship.Length)
		string = string + 'B = {0} [m]<BR>'.format(self.ship.Breadth)
		string = string + 'T = {0} [m]<HR>'.format(self.form.draft.value())
		string = string + 'Trim = {0} [degrees]<BR>'.format(self.form.trim.value())
		string = string + 'T<sub>AP</sub> = {0} [m]<BR>'.format(draftAP)
		string = string + 'T<sub>FP</sub> = {0} [m]<HR>'.format(draftFP)
		dispText = QtGui.QApplication.translate("ship_areas",'Displacement',
                                 None,QtGui.QApplication.UnicodeUTF8)
		string = string + dispText + ' = {0} [ton]<BR>'.format(data[0])
		string = string + 'XCB = {0} [m]'.format(data[1].x)
		self.form.output.setHtml(string)

	def save(self):
		""" Saves the data into ship instance. """
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
			tooltip = str(QtGui.QApplication.translate("ship_areas","Areas curve tool trim selected [deg]",
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
