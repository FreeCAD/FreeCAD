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
import Preview, Plot
import Instance
from shipUtils import Paths

class TaskPanel:
	def __init__(self):
		self.ui = Paths.modulePath() + "/shipOutlineDraw/TaskPanel.ui"
		self.ship = None
		self.skip = False
		self.LSections = []
		self.BSections = []
		self.TSections = []
		self.obj = None
		self.preview = Preview.Preview()

	def accept(self):
		self.saveSections()
		self.obj = Plot.Plot(self.form.scale.value(), self.obj.Shape, self.ship.Shape)
		self.preview.clean()
		self.obj.Label = 'OutlineDraw'
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
		form.sections = form.findChild(QtGui.QTableWidget, "Sections")
		try:
			form.sections.setInputMethodHints(QtCore.Qt.ImhFormattedNumbersOnly)
			hasImhFormattedNumbersOnly = True
		except:
			hasImhFormattedNumbersOnly = False			
		form.sectionType = form.findChild(QtGui.QComboBox, "SectionType")
		form.deleteButton = form.findChild(QtGui.QPushButton, "DeleteButton")
		form.nSections = form.findChild(QtGui.QSpinBox, "NSections")
		form.createButton = form.findChild(QtGui.QPushButton, "CreateButton")
		form.scale = form.findChild(QtGui.QSpinBox, "Scale")
		self.form = form
		# Initial values
		if self.initValues():
			return True
		self.retranslateUi()
		self.obj = self.preview.update(self.ship.Length, self.ship.Beam, self.ship.Draft, self.LSections,self.BSections,self.TSections, self.ship.Shape)
		# Connect Signals and Slots
		QtCore.QObject.connect(form.sectionType,QtCore.SIGNAL("activated(QString)"),self.onSectionType)
		QtCore.QObject.connect(form.sections,QtCore.SIGNAL("cellChanged(int,int)"),self.onTableItem);
		QtCore.QObject.connect(form.deleteButton,QtCore.SIGNAL("pressed()"),self.onDeleteButton)
		QtCore.QObject.connect(form.createButton,QtCore.SIGNAL("pressed()"),self.onCreateButton)

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
		# Get selected objects
		selObjs  = Gui.Selection.getSelection()
		if not selObjs:
			msg = QtGui.QApplication.translate("ship_console",
									   "Ship instance must be selected (no object selected)",
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
		# Load sections (if exist)
		self.loadSections()
		return False

	def retranslateUi(self):
		""" Set user interface locale strings. 
		"""
		self.form.setWindowTitle(QtGui.QApplication.translate("ship_outline","Outline draw",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QGroupBox, "AutoCreateBox").setTitle(QtGui.QApplication.translate("ship_outline","Auto create",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QGroupBox, "ScaleBox").setTitle(QtGui.QApplication.translate("ship_outline","Scale",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QPushButton, "DeleteButton").setText(QtGui.QApplication.translate("ship_outline",
								 "Delete all sections",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QPushButton, "CreateButton").setText(QtGui.QApplication.translate("ship_outline",
								 "Create sections",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QComboBox, "SectionType").setItemText(0, QtGui.QApplication.translate("ship_outline",
								 "Transversal",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QComboBox, "SectionType").setItemText(1, QtGui.QApplication.translate("ship_outline",
								 "Longitudinal",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QComboBox, "SectionType").setItemText(2, QtGui.QApplication.translate("ship_outline",
								 "Water lines",
								 None,QtGui.QApplication.UnicodeUTF8))

	def onSectionType(self):
		""" Function called when the section type is changed.
		"""
		# Search section type
		ID = self.form.sectionType.currentIndex()
		self.setSectionType(ID)

	def setSectionType(self, ID):
		""" Function that set the type section related table.
		@param ID Id of the section to set: \n
		0 = Transversal sections \n
		1 = Longitudinal sections \n
		2 = Water lines
		"""
		SectionList = []
		if ID == 0:
			SectionList = self.LSections[:]
		elif ID == 1:
			SectionList = self.BSections[:]
		elif ID == 2:
			SectionList = self.TSections[:]
		nRow = len(SectionList)
		self.form.sections.clearContents()
		self.form.sections.setRowCount(nRow+1)
		if not nRow:
			self.obj = self.preview.update(self.ship.Length, self.ship.Beam, self.ship.Draft, self.LSections,self.BSections,self.TSections, self.ship.Shape)
			return
		self.skip = True					# Avoid recursive call to OnItem
		for i in range(0,nRow):
			if i == nRow-1:
				self.skip = False
			string = '%f' % (SectionList[i])
			item = QtGui.QTableWidgetItem(string)
			self.form.sections.setItem(i,0,item)

	def onTableItem(self, row, column):
		""" Function called when an item of table is changed.
		@param row Changed item row
		@param column Changed item column
		"""
		if self.skip:
			return
		# Ensure that exist one empty item at least
		nRow = self.form.sections.rowCount()
		item = self.form.sections.item(nRow-1,0)
		if item :
			if(item.text() != ''):
				self.form.sections.setRowCount(nRow+1)
		# Ensure that new item is a number
		ID = self.form.sectionType.currentIndex()
		if ID == 0:
			SectionList = self.LSections[:]
		elif ID == 1:
			SectionList = self.BSections[:]
		elif ID == 2:
			SectionList = self.TSections[:]
		item = self.form.sections.item(row,column)
		(number,flag) = item.text().toFloat()
		if not flag:
			if len(SectionList) > nRow-1:
				number = SectionList[nRow-1]
			else:
				number = 0.0
		string = '%f' % (number)
		item.setText(string)
		# Regenerate the list
		SectionList = []
		for i in range(0,nRow):
			item = self.form.sections.item(i,0)
			if item:
				(number,flag) = item.text().toFloat()
				SectionList.append(number)
		# Paste it into the class list
		ID = self.form.sectionType.currentIndex()
		if ID == 0:
			self.LSections = SectionList[:]
		elif ID == 1:
			self.BSections = SectionList[:]
		elif ID == 2:
			self.TSections = SectionList[:]
		self.obj = self.preview.update(self.ship.Length, self.ship.Beam, self.ship.Draft, self.LSections,self.BSections,self.TSections, self.ship.Shape)
		
	def onDeleteButton(self):
		""" Function called when the delete button is pressed.
		All sections mustt be erased
		"""
		self.form.sections.clearContents()
		self.form.sections.setRowCount(1)
		# Clear active list
		ID = self.form.sectionType.currentIndex()
		if ID == 0:
			self.LSections = []
		elif ID == 1:
			self.BSections = []
		elif ID == 2:
			self.TSections = []
		self.setSectionType(ID)
		
	def onCreateButton(self):
		""" Function called when create button is pressed.
		Several sections must be added to list
		"""
		# Recolect data
		nSections = self.form.nSections.value()
		SectionList = []
		L = 0.0
		ID = self.form.sectionType.currentIndex()
		if ID == 0:
			L = self.ship.Length
			d = L / (nSections-1)	   # Distance between sections
			start = - L/2.0		 # Ship must have 0.0 at coordinates origin
		elif ID == 1:
			L = -0.5*self.ship.Beam	  # Ship must be in y<0.0
			d = L / (nSections+1.0)	   # Distance between sections
			start = d
		elif ID == 2:
			L = self.ship.Draft
			d = L / (nSections)	   # Distance between sections
			start = d
		# Calculate sections
		for i in range(0,nSections):
			sec = i*d + start
			SectionList.append(sec)
		# Paste into class table
		if ID == 0:
			self.LSections = SectionList[:]
		elif ID == 1:
			self.BSections = SectionList[:]
		elif ID == 2:
			self.TSections = SectionList[:]
		# Print the table
		self.setSectionType(ID)

	def loadSections(self):
		""" Loads from ship object previously selected sections.
		"""
		# Load sections
		props = self.ship.PropertiesList
		flag=True
		try:
			props.index("LSections")
		except ValueError:
			flag=False
		if flag:
			self.LSections = self.ship.LSections[:]
			self.BSections = self.ship.BSections[:]
			self.TSections = self.ship.TSections[:]
		# Load scale too
		flag=True
		try:
			props.index("PlotScale")
		except ValueError:
			flag=False
		if flag:
			self.form.scale.setValue(self.ship.PlotScale)
		# Set UI
		self.setSectionType(self.form.sectionType.currentIndex())

	def saveSections(self):
		""" Save selected sections into ship object.
		"""
		# Test if previous section have been created
		props = self.ship.PropertiesList
		try:
			props.index("LSections")
		except ValueError:
			# Create new sections list
			tooltip = str(QtGui.QApplication.translate("ship_outline","Transversal sections position [m]",
								 None,QtGui.QApplication.UnicodeUTF8))
			self.ship.addProperty("App::PropertyFloatList","LSections","Ship", tooltip).LSections=[]
			tooltip = str(QtGui.QApplication.translate("ship_outline","Longitudinal sections position [m]",
								 None,QtGui.QApplication.UnicodeUTF8))
			self.ship.addProperty("App::PropertyFloatList","BSections","Ship", tooltip).BSections=[]
			tooltip = str(QtGui.QApplication.translate("ship_outline","Water lines position [m]",
								 None,QtGui.QApplication.UnicodeUTF8))
			self.ship.addProperty("App::PropertyFloatList","TSections","Ship", tooltip).TSections=[]
		# Save sections
		self.ship.LSections = self.LSections[:]
		self.ship.BSections = self.BSections[:]
		self.ship.TSections = self.TSections[:]
		# Save also scale
		try:
			props.index("PlotScale")
		except ValueError:
			tooltip = str(QtGui.QApplication.translate("ship_outline","Plot scale (1:scale format)",
								 None,QtGui.QApplication.UnicodeUTF8))
			self.ship.addProperty("App::PropertyInteger","PlotScale","Ship", tooltip).PlotScale=250
		self.ship.PlotScale = self.form.scale.value()

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
