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
# pyOpenCL
import pyopencl as cl
# Module
import SimInstance
from shipUtils import Paths
from simRun import Simulation
Sim = Simulation.FreeCADShipSimulation

class TaskPanel:
	def __init__(self):
		self.ui  = Paths.modulePath() + "/simPost/TaskPanel.ui"

	def accept(self):
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
		form.time  = form.findChild(QtGui.QLabel, "TimeLabel")
		form.first = form.findChild(QtGui.QPushButton, "First")
		form.prev  = form.findChild(QtGui.QPushButton, "Prev")
		form.now   = form.findChild(QtGui.QPushButton, "Now")
		form.next  = form.findChild(QtGui.QPushButton, "Next")
		form.last  = form.findChild(QtGui.QPushButton, "Last")
		self.form  = form
		# Initial values
		if self.initValues():
			return True
		self.retranslateUi()
		# Connect Signals and Slots
		QtCore.QObject.connect(form.first, QtCore.SIGNAL("pressed()"), self.onFirst)
		QtCore.QObject.connect(form.prev, QtCore.SIGNAL("pressed()"), self.onPrev)
		QtCore.QObject.connect(form.now, QtCore.SIGNAL("pressed()"), self.onNow)
		QtCore.QObject.connect(form.next, QtCore.SIGNAL("pressed()"), self.onNext)
		QtCore.QObject.connect(form.last, QtCore.SIGNAL("pressed()"), self.onLast)

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
		self.form.setWindowTitle(QtGui.QApplication.translate("shipsim_track","Track simulation",
								 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QPushButton, "Now").setText(QtGui.QApplication.translate("shipsim_track","Now",
								 None,QtGui.QApplication.UnicodeUTF8))

	def onFirst(self):
		""" Called when first frame button is pressed.
		"""

	def onPrev(self):
		""" Called when previous frame button is pressed.
		"""

	def onNow(self):
		""" Called when actual frame button is pressed.
		"""
		sim = Sim()
		pos = sim.sim.FS_Position[:]
		nx  = sim.FS['Nx']
		ny  = sim.FS['Ny']
		for i in range(0, nx):
			for j in range(0, ny):
				pos[i*ny+j].z = float(sim.FS['pos'][i,j][2])
		sim.sim.FS_Position = pos[:]
		App.ActiveDocument.recompute()
		self.form.time.setText("t = %g s" % (sim.t))

	def onNext(self):
		""" Called when next frame button is pressed.
		"""

	def onLast(self):
		""" Called when last frame button is pressed.
		"""

def createTask():
	try:
		simulator = Sim()
	except:
		msg = QtGui.QApplication.translate("ship_console", "Can't find any active simulation",
										   None,QtGui.QApplication.UnicodeUTF8)
		App.Console.PrintError(msg + '\n')
		return
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
