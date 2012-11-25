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
import Preview
from Instance import *
from shipUtils import Paths

class TaskPanel:
	def __init__(self):
		self.ui = Paths.modulePath() + "/tankWeights/TaskPanel.ui"
		self.ship = None
		self.preview = Preview.Preview()

	def accept(self):
		self.preview.clean()
		if not self.ship:
			return False
		# Setup lists
		name  = []
		mass  = []
		pos   = []
		for i in range(0,self.form.weights.rowCount() - 1):
			item = self.form.weights.item(i,0)
			name.append(item.text().__str__())
			item = self.form.weights.item(i,1)
			mass.append(item.text().toFloat()[0])
			vec  = []
			item = self.form.weights.item(i,2)
			vec.append(item.text().toFloat()[0])
			item = self.form.weights.item(i,3)
			vec.append(item.text().toFloat()[0])
			item = self.form.weights.item(i,4)
			vec.append(item.text().toFloat()[0])
			pos.append(App.Base.Vector(vec[0],vec[1],vec[2]))
		# Send to ship
		self.ship.WeightNames = name[:]
		self.ship.WeightMass  = mass[:]
		self.ship.WeightPos   = pos[:]
		return True

	def reject(self):
		self.preview.clean()
		if not self.ship:
			return False
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
		form.weights = form.findChild(QtGui.QTableWidget, "Weights")
		self.form = form
		# Initial values
		if self.initValues():
			return True
		self.retranslateUi()
		# Connect Signals and Slots
		QtCore.QObject.connect(form.weights,QtCore.SIGNAL("cellChanged(int,int)"),self.onTableItem);
		# Update screen
		name  = []
		pos   = []
		for i in range(0,self.form.weights.rowCount() - 1):
			item = self.form.weights.item(i,0)
			name.append(item.text().__str__())
			vec  = []
			item = self.form.weights.item(i,2)
			vec.append(item.text().toFloat()[0])
			item = self.form.weights.item(i,3)
			vec.append(item.text().toFloat()[0])
			item = self.form.weights.item(i,4)
			vec.append(item.text().toFloat()[0])
			pos.append(App.Base.Vector(vec[0],vec[1],vec[2]))		
		self.preview.update(name, pos)

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
		@return False if sucessfully values initialized.
		"""
		# Get selected objects
		selObjs  = FreeCADGui.Selection.getSelection()
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
					msg = QtGui.QApplication.translate("ship_console",
													   "More than one ship selected (extra ships will be neglected)",
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
		# Get weights
		w = weights(self.ship)
		# Set the items
		self.form.weights.setRowCount(len(w)+1)
		for i in range(0,len(w)):
			item = QtGui.QTableWidgetItem(w[i][0])
			self.form.weights.setItem(i,0,item)
			string = '%g' % (w[i][1])
			item = QtGui.QTableWidgetItem(string)
			self.form.weights.setItem(i,1,item)
			string = '%g' % (w[i][2].x)
			item = QtGui.QTableWidgetItem(string)
			self.form.weights.setItem(i,2,item)
			string = '%g' % (w[i][2].y)
			item = QtGui.QTableWidgetItem(string)
			self.form.weights.setItem(i,3,item)
			string = '%g' % (w[i][2].z)
			item = QtGui.QTableWidgetItem(string)
			self.form.weights.setItem(i,4,item)
		return False

	def retranslateUi(self):
		""" Set user interface locale strings. 
		"""
		self.form.setWindowTitle(QtGui.QApplication.translate("shiptank_weights","Set weights",
								 None,QtGui.QApplication.UnicodeUTF8))
		labels = []
		labels.append(QtGui.QApplication.translate("shiptank_weights","Name",
								 None,QtGui.QApplication.UnicodeUTF8))
		labels.append(QtGui.QApplication.translate("shiptank_weights","Mass",
								 None,QtGui.QApplication.UnicodeUTF8) + " [kg]")		
		labels.append(QtCore.QString("g.x [m]"))		
		labels.append(QtCore.QString("g.y [m]"))		
		labels.append(QtCore.QString("g.z [m]"))   
		self.form.weights.setHorizontalHeaderLabels(labels)

	def onTableItem(self, row, column):
		""" Function called when an item of table is changed.
		@param row Changed item row
		@param column Changed item column
		"""
		item = self.form.weights.item(row,column)
		# Row deletion
		if column == 0:
			if not item.text():
				self.form.weights.removeRow(row)
		# Ensure that exist one empty item at the end
		nRow = self.form.weights.rowCount()
		last = self.form.weights.item(nRow-1,0)
		if last:
			if(last.text() != ''):
				self.form.weights.setRowCount(nRow+1)
		# Fields must be numbers
		for i in range(0,self.form.weights.rowCount()-1):	  # Avoid last row
			for j in range(1,self.form.weights.columnCount()): # Avoid name column
				item = self.form.weights.item(i,j)
				if not item:
					item = QtGui.QTableWidgetItem('0.0')
					self.form.weights.setItem(i,j,item)
					continue
				(number,flag) = item.text().toFloat()
				if not flag:
					item.setText('0.0')
		# Update screen annotations
		name  = []
		pos   = []
		for i in range(0,self.form.weights.rowCount() - 1):
			item = self.form.weights.item(i,0)
			name.append(item.text().__str__())
			vec  = []
			item = self.form.weights.item(i,2)
			vec.append(item.text().toFloat()[0])
			item = self.form.weights.item(i,3)
			vec.append(item.text().toFloat()[0])
			item = self.form.weights.item(i,4)
			vec.append(item.text().toFloat()[0])
			pos.append(App.Base.Vector(vec[0],vec[1],vec[2]))		
		self.preview.update(name, pos)

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
