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
from shipUtils import Paths

class TaskPanel:
	def __init__(self):
		self.ui = Paths.modulePath() + "/shipLoadExample/TaskPanel.ui"

	def accept(self):
		path = Paths.modulePath() + "/resources/examples/"
		if(self.form.ship.currentIndex() == 0):	 # s60 from Iowa University
			App.open(path + "s60.fcstd")
		elif(self.form.ship.currentIndex() == 1):   # Wigley canonical ship
			App.open(path + "wigley.fcstd")
		elif(self.form.ship.currentIndex() == 2):   # s60 (Katamaran)
			App.open(path + "s60_katamaran.fcstd")
		elif(self.form.ship.currentIndex() == 2):   # Wigley (Katamaran)
			App.open(path + "wigley_katamaran.fcstd")
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
		form.ship = form.findChild(QtGui.QComboBox, "Ship")
		form.mainLogo = form.findChild(QtGui.QLabel, "MainLogo")
		iconPath = Paths.iconsPath() + "/Ico.xpm"
		form.mainLogo.setPixmap(QtGui.QPixmap(iconPath))
		self.form = form
		self.retranslateUi()

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

	def retranslateUi(self):
		""" Set user interface locale strings. 
		"""
		self.form.setWindowTitle(QtGui.QApplication.translate("ship_load","Load example ship",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QGroupBox, "ShipSelectionBox").setTitle(QtGui.QApplication.translate("ship_load",
								 "Select ship example geometry",
								 None,QtGui.QApplication.UnicodeUTF8))

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
