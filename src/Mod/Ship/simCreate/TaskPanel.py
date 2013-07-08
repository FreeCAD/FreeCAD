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
# Qt library
from PyQt4 import QtGui,QtCore
# Module
import SimInstance
from shipUtils import Paths

class TaskPanel:
	def __init__(self):
		self.ui = Paths.modulePath() + "/simCreate/TaskPanel.ui"

	def accept(self):
		form = self.form
		# Read waves data
		w = []
		for i in range(0,form.waves.rowCount() - 1):
			item = form.waves.item(i,0)
			A = item.text().toFloat()[0]
			item = form.waves.item(i,1)
			T = item.text().toFloat()[0]
			item = form.waves.item(i,2)
			phi = item.text().toFloat()[0]
			item = form.waves.item(i,3)
			head = item.text().toFloat()[0]
			w.append([A,T,phi,head])
		obj = App.ActiveDocument.addObject("Part::FeaturePython","ShipSimulation")
		sim = SimInstance.ShipSimulation(obj,
		      [form.fsL.value(), form.fsB.value(), form.fsNx.value(), form.fsNy.value()],
		      w,
		      form.error.value())
		SimInstance.ViewProviderShipSimulation(obj.ViewObject)
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
		mw          = self.getMainWindow()
		form        = mw.findChild(QtGui.QWidget, "TaskPanel")
		form.fsL    = form.findChild(QtGui.QDoubleSpinBox, "Length")
		form.fsB    = form.findChild(QtGui.QDoubleSpinBox, "Beam")
		form.fsNx   = form.findChild(QtGui.QSpinBox, "Nx")
		form.fsNy   = form.findChild(QtGui.QSpinBox, "Ny")
		form.error  = form.findChild(QtGui.QDoubleSpinBox, "Error")
		form.waves  = form.findChild(QtGui.QTableWidget, "Waves")
		self.form = form
		# Initial values
		if self.initValues():
			return True
		self.retranslateUi()
		# Connect Signals and Slots
		QtCore.QObject.connect(form.fsL, QtCore.SIGNAL("valueChanged(double)"), self.onFS)
		QtCore.QObject.connect(form.fsB, QtCore.SIGNAL("valueChanged(double)"), self.onFS)
		QtCore.QObject.connect(form.fsNx, QtCore.SIGNAL("valueChanged(int)"), self.onFS)
		QtCore.QObject.connect(form.fsNy, QtCore.SIGNAL("valueChanged(int)"), self.onFS)
		QtCore.QObject.connect(form.error, QtCore.SIGNAL("valueChanged(double)"), self.onError)
		QtCore.QObject.connect(form.waves,QtCore.SIGNAL("cellChanged(int,int)"),self.onWaves);

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
		return False

	def retranslateUi(self):
		""" Set user interface locale strings. 
		"""
		self.form.setWindowTitle(QtGui.QApplication.translate("shipsim_create","Create a new ship simulation",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QGroupBox, "FSDataBox").setTitle(QtGui.QApplication.translate("shipsim_create","Free surface",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QGroupBox, "WavesDataBox").setTitle(QtGui.QApplication.translate("shipsim_create","Waves",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QGroupBox, "OtherBox").setTitle(QtGui.QApplication.translate("shipsim_create","Other",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "ErrorLabel").setText(QtGui.QApplication.translate("shipsim_create","Relative error",
								 None,QtGui.QApplication.UnicodeUTF8))
		labels = []
		labels.append(QtGui.QApplication.translate("shipsim_create","Amplitude",
					  None,QtGui.QApplication.UnicodeUTF8) + " [m]")
		labels.append(QtGui.QApplication.translate("shipsim_create","Period",
					  None,QtGui.QApplication.UnicodeUTF8) + " [s]")
		labels.append(QtGui.QApplication.translate("shipsim_create","Phase",
					  None,QtGui.QApplication.UnicodeUTF8) + " [rad]")
		labels.append(QtGui.QApplication.translate("shipsim_create","Heading",
					  None,QtGui.QApplication.UnicodeUTF8) + " [deg]")
		self.form.waves.setHorizontalHeaderLabels(labels)
		# Set some tooltips
		tooltip = QtGui.QApplication.translate("shipsim_create","Free surface length on x direction",
								 None,QtGui.QApplication.UnicodeUTF8)
		self.form.findChild(QtGui.QLabel, "LengthLabel").setToolTip(tooltip)
		self.form.findChild(QtGui.QDoubleSpinBox, "Length").setToolTip(tooltip)
		tooltip = QtGui.QApplication.translate("shipsim_create","Free surface length on y direction",
								 None,QtGui.QApplication.UnicodeUTF8)
		self.form.findChild(QtGui.QLabel, "BeamLabel").setToolTip(tooltip)
		self.form.findChild(QtGui.QDoubleSpinBox, "Beam").setToolTip(tooltip)
		tooltip = QtGui.QApplication.translate("shipsim_create","Number of nodes on x direction. Take into acount the following considerations:\n1.\tNodes must have an aspect ratio as near to 1,0 as possible, so this values must\n\taccomplish approximately that Nx/Ny = L/B\n3.\tThe linear system matrix generated will be of dimensions NxN, where\n\tN = Nx*Ny\n\tSo be mindful with the values selected and computer capabilities.",
								 None,QtGui.QApplication.UnicodeUTF8)
		self.form.findChild(QtGui.QLabel, "NxLabel").setToolTip(tooltip)
		self.form.findChild(QtGui.QSpinBox, "Nx").setToolTip(tooltip)
		tooltip = QtGui.QApplication.translate("shipsim_create","Number of nodes on y direction. Take into acount the following considerations:\n1.\tNodes must have an aspect ratio as near to 1,0 as possible, so this values must\n\taccomplish approximately that Nx/Ny = L/B\n3.\tThe linear system matrix generated will be of dimensions NxN, where\n\tN = Nx*Ny\n\tSo be mindful with the values selected and computer capabilities.",
								 None,QtGui.QApplication.UnicodeUTF8)
		self.form.findChild(QtGui.QLabel, "NyLabel").setToolTip(tooltip)
		self.form.findChild(QtGui.QSpinBox, "Ny").setToolTip(tooltip)
		tooltip = QtGui.QApplication.translate("shipsim_create","Relation between the minimum value of the Green's function (fartest point) and the maximum one.\nThis variable set the number of times that the Free surface will be virtually repeated.\nLower values may imply too much repeated free surfaces with a significant cost.",
								 None,QtGui.QApplication.UnicodeUTF8)
		self.form.findChild(QtGui.QLabel, "ErrorLabel").setToolTip(tooltip)
		self.form.findChild(QtGui.QDoubleSpinBox, "Error").setToolTip(tooltip)

	def onFS(self, value):
		""" Method called when free surface data is changed.
		 @param value Changed value.
		"""
		pass

	def onError(self, value):
		""" Method called when sea data is changed.
		 @param value Changed value.
		"""
		pass

	def onWaves(self, row, column):
		""" Method called when waves data is changed.
		 @param row Affected row.
		 @param col Affected column.
		"""
		item = self.form.waves.item(row,column)
		# Row deletion
		if column == 0:
			if not item.text():
				self.form.waves.removeRow(row)
		# Ensure that exist one empty item at the end
		nRow = self.form.waves.rowCount()
		if not nRow:
			self.form.waves.setRowCount(1)
		else:
			last = self.form.waves.item(nRow-1,0)
			if last:
				if(last.text() != ''):
					self.form.waves.setRowCount(nRow+1)
		# Fields must be numbers
		for i in range(0,self.form.waves.rowCount()-1):	  # Avoid last row
			for j in range(0,self.form.waves.columnCount()): # Avoid name column
				item = self.form.waves.item(i,j)
				if not item:
					item = QtGui.QTableWidgetItem('0.0')
					self.form.waves.setItem(i,j,item)
					continue
				(number,flag) = item.text().toFloat()
				if not flag:
					item.setText('0.0')

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
