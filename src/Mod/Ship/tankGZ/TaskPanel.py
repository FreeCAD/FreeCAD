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
from Instance import *
from TankInstance import *
from shipUtils import Paths
from shipHydrostatics import Tools as Hydrostatics

class TaskPanel:
	def __init__(self):
		self.ui = Paths.modulePath() + "/tankGZ/TaskPanel.ui"
		self.ship  = None
		self.tanks = {}
		self.running  = False

	def accept(self):
		if not self.ship:
			return False
		if self.running:
			return
		# Get general data
		disp  = self.computeDisplacement()
		draft = self.computeDraft(disp[0], self.form.trim.value())
		trim  = self.form.trim.value()
		# Get roll angles
		roll0 = self.form.roll0.value()
		roll1 = self.form.roll1.value()
		nRoll = self.form.nRoll.value()
		dRoll = (roll1 - roll0) / (nRoll - 1)
		roll  = []
		GZ	= []
		msg = QtGui.QApplication.translate("ship_console","Computing GZ",
                                   None,QtGui.QApplication.UnicodeUTF8)
		App.Console.PrintMessage(msg + "...\n")
		loop=QtCore.QEventLoop()
		timer=QtCore.QTimer()
		timer.setSingleShot(True)
		QtCore.QObject.connect(timer,QtCore.SIGNAL("timeout()"),loop,QtCore.SLOT("quit()"))
		self.running = True
		for i in range(0, nRoll):
			App.Console.PrintMessage("\t%d/%d\n" % (i+1,nRoll))
			roll.append(i*dRoll)
			GZ.append(self.computeGZ(draft[0], trim, roll[-1]))
			timer.start(0.0)
			loop.exec_()
			if(not self.running):
				break
		PlotAux.Plot(roll, GZ, disp[0]/1000.0, draft[0], trim)
		return True

	def reject(self):
		if not self.ship:
			return False
		if self.running:
			self.running = False
			return
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
		mw			= self.getMainWindow()
		form		  = mw.findChild(QtGui.QWidget, "TaskPanel")
		form.tanks	= form.findChild(QtGui.QListWidget, "Tanks")
		form.disp	 = form.findChild(QtGui.QLabel, "DisplacementLabel")
		form.draft	= form.findChild(QtGui.QLabel, "DraftLabel")
		form.update   = form.findChild(QtGui.QPushButton, "UpdateData")
		form.trim	 = form.findChild(QtGui.QDoubleSpinBox, "Trim")
		form.autoTrim = form.findChild(QtGui.QPushButton, "TrimAutoCompute")
		form.roll0	= form.findChild(QtGui.QDoubleSpinBox, "StartAngle")
		form.roll1	= form.findChild(QtGui.QDoubleSpinBox, "EndAngle")
		form.nRoll	= form.findChild(QtGui.QSpinBox, "NAngle")
		self.form = form
		# Initial values
		if self.initValues():
			return True
		self.retranslateUi()
		# Connect Signals and Slots
		QtCore.QObject.connect(form.tanks,QtCore.SIGNAL("itemSelectionChanged()"),self.onTanksSelection)
		QtCore.QObject.connect(form.update,QtCore.SIGNAL("pressed()"),self.onUpdate)
		QtCore.QObject.connect(form.trim,QtCore.SIGNAL("valueChanged(double)"),self.onTrim)
		QtCore.QObject.connect(form.autoTrim,QtCore.SIGNAL("pressed()"),self.onAutoTrim)
		QtCore.QObject.connect(form.roll0,QtCore.SIGNAL("valueChanged(double)"),self.onRoll)
		QtCore.QObject.connect(form.roll1,QtCore.SIGNAL("valueChanged(double)"),self.onRoll)
		QtCore.QObject.connect(form.nRoll,QtCore.SIGNAL("valueChanged(int)"),self.onRoll)
		return False

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
			msg = QtGui.QApplication.translate("ship_console","Ship instance must be selected (no object selected)",
                                   None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg)
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
		props = self.ship.PropertiesList
		try:
			props.index("WeightNames")
		except:
			msg = QtGui.QApplication.translate("ship_console",
                                   "Ship weights has not been set. You need to set weights before use this tool",
                                   None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg + '\n')
			return True
		# Setup available tanks list
		objs = App.ActiveDocument.Objects
		iconPath  = Paths.iconsPath() + "/Tank.xpm"
		icon	  = QtGui.QIcon(QtGui.QPixmap(iconPath))
		for obj in objs:
			# Try to get valid tank property
			props = obj.PropertiesList
			try:
				props.index("IsShipTank")
			except ValueError:
				continue
			if not obj.IsShipTank:
				continue
			# Add tank to list
			name  = obj.Name
			label = obj.Label
			tag   = label + ' (' + name + ')'
			self.tanks[tag] = name
			# self.tanks.append([name, tag])
			item  = QtGui.QListWidgetItem(tag)
			item.setIcon(icon)
			self.form.tanks.addItem(item)
		return False

	def retranslateUi(self):
		""" Set user interface locale strings. 
		"""
		self.form.findChild(QtGui.QLabel, "DraftLabel").setText(QtGui.QApplication.translate("shiptank_gz","Draft",
                                 None,QtGui.QApplication.UnicodeUTF8))


		self.form.setWindowTitle(QtGui.QApplication.translate("shiptank_gz","GZ curve computation",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QGroupBox, "LoadConditionGroup").setTitle(QtGui.QApplication.translate("shiptank_gz",
                                 "Loading condition",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QGroupBox, "AnglesGroup").setTitle(QtGui.QApplication.translate("shiptank_gz","Roll angles",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "TrimLabel").setText(QtGui.QApplication.translate("shiptank_gz","Trim",
                                 None,QtGui.QApplication.UnicodeUTF8) + " [deg]")
		self.form.findChild(QtGui.QLabel, "StartAngleLabel").setText(QtGui.QApplication.translate("shiptank_gz","Start",
                                 None,QtGui.QApplication.UnicodeUTF8) + " [deg]")
		self.form.findChild(QtGui.QLabel, "EndAngleLabel").setText(QtGui.QApplication.translate("shiptank_gz","End",
                                 None,QtGui.QApplication.UnicodeUTF8) + " [deg]")
		self.form.findChild(QtGui.QLabel, "NAngleLabel").setText(QtGui.QApplication.translate("shiptank_gz","Number of points",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.disp.setText(QtGui.QApplication.translate("shiptank_gz","Displacement",
                                 None,QtGui.QApplication.UnicodeUTF8) + ' = ' + \
                                 QtGui.QApplication.translate("shiptank_gz","Press update to compute",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.draft.setText(QtGui.QApplication.translate("shiptank_gz","Draft",
                                 None,QtGui.QApplication.UnicodeUTF8) + ' = ' + \
                                 QtGui.QApplication.translate("shiptank_gz","Press update to compute",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.update.setText(QtGui.QApplication.translate("shiptank_gz","Update displacement and draft",
                                 None,QtGui.QApplication.UnicodeUTF8))

	def onTanksSelection(self):
		""" Called when tanks are selected or deselected.
		"""
		pass
		
	def onUpdate(self):
		""" Called when update displacement and draft is requested.
		"""
		# Set displacement label
		disp = self.computeDisplacement()
		self.form.disp.setText(QtGui.QApplication.translate("shiptank_gz","Displacement",
                               None,QtGui.QApplication.UnicodeUTF8) + ' = %g [kg]' % (disp[0]))
		# Set draft label
		draft = self.computeDraft(disp[0], self.form.trim.value())
		self.form.draft.setText(QtGui.QApplication.translate("shiptank_gz","Draft",
                                None,QtGui.QApplication.UnicodeUTF8) + ' = %g [m]' % (draft[0]))

	def onTrim(self, trim):
		""" Called when trim angle value is changed.
		@param trim Selected trim angle.
		"""
		self.onTanksSelection()
	
	def onAutoTrim(self):
		""" Called when trim angle must be auto computed.
		"""
		# Start at null trim angle
		trim = 0.0
		# Get center of gravity
		disp  = self.computeDisplacement(trim)
		G	 = [disp[1], disp[2], disp[3]]
		disp  = disp[0]
		# Get bouyancy center
		draft = self.computeDraft(disp)
		B	 = [draft[1].x, draft[1].y, draft[1].z]
		draft = draft[0]
		# Get stability initial condition
		BG	= [G[0]-B[0], G[1]-B[1], G[2]-B[2]]
		x	 = BG[0]*math.cos(math.radians(trim)) - BG[2]*math.sin(math.radians(trim))
		y	 = BG[1]
		z	 = BG[0]*math.sin(math.radians(trim)) + BG[2]*math.cos(math.radians(trim))
		var   = math.degrees(math.atan2(x,z))
		# Iterate looking stability point
		dVar = math.copysign(0.01, var)
		while True:
			if (dVar*var < 0.0) or (abs(var) < 0.1):
				break
			trim = trim - math.copysign(max(dVar, abs(var)/200.0), var)
			# Get center of gravity
			disp  = self.computeDisplacement(trim)
			G	 = [disp[1], disp[2], disp[3]]
			disp  = disp[0]
			# Get bouyancy center
			draft = self.computeDraft(disp, trim)
			B	 = [draft[1].x, draft[1].y, draft[1].z]
			draft = draft[0]
			# Get stability initial condition
			BG	= [G[0]-B[0], G[1]-B[1], G[2]-B[2]]
			x	 = BG[0]*math.cos(math.radians(trim)) - BG[2]*math.sin(math.radians(trim))
			y	 = BG[1]
			z	 = BG[0]*math.sin(math.radians(trim)) + BG[2]*math.cos(math.radians(trim))
			var   = math.degrees(math.atan2(x,z))
		self.form.trim.setValue(trim)

	def onRoll(self, value):
		""" Called when roll angles options are modified.
		@param value Dummy changed value.
		"""
		roll0 = self.form.roll0.value()
		self.form.roll1.setMinimum(roll0)
		roll1 = self.form.roll1.value()
		self.form.roll0.setMaximum(roll1)

	def getTanks(self):
		""" Get the selected tanks objects list.
		@return Selected tanks list.
		"""
		items = self.form.tanks.selectedItems()
		tanks = []
		for item in items:
			tag  = str(item.text())
			name = self.tanks[tag]
			t	= App.ActiveDocument.getObject(name)
			if not t:
				continue
			tanks.append(t)
		return tanks

	def computeDisplacement(self, trim=0.0, roll=0.0):
		""" Computes ship displacement.
		@param trim Trim angle [degrees].
		@return Ship displacement and center of gravity. None if errors 
		detected.
		@note Returned center of gravity is refered to ship attached 
		axis coordinates.
		"""
		if not self.ship:
			return None
		# Get ship structure weights
		W = [0.0, 0.0, 0.0, 0.0]
		sWeights = weights(self.ship)
		for w in sWeights:
			W[0] = W[0] + w[1]
			W[1] = W[1] + w[1]*w[2][0]
			W[2] = W[2] + w[1]*w[2][1]
			W[3] = W[3] + w[1]*w[2][2]
		# Get selected tanks weights
		tanks = self.getTanks()
		for t in tanks:
			w = tankWeight(t, App.Base.Vector(roll,-trim,0.0))
			# Unrotate center of gravity
			x = w[1]*math.cos(math.radians(-trim)) - w[3]*math.sin(math.radians(-trim))
			y = w[2]
			z = w[1]*math.sin(math.radians(-trim)) + w[3]*math.cos(math.radians(-trim))
			w[1] = x
			w[2] = y*math.cos(math.radians(-roll)) - z*math.sin(math.radians(-roll))
			w[3] = y*math.sin(math.radians(-roll)) + z*math.cos(math.radians(-roll))
			W[0] = W[0] + w[0]
			W[1] = W[1] + w[0]*w[1]
			W[2] = W[2] + w[0]*w[2]
			W[3] = W[3] + w[0]*w[3]
		return [W[0], W[1]/W[0], W[2]/W[0], W[3]/W[0]]

	def computeDraft(self, disp, trim=0.0):
		""" Computes ship draft.
		@param disp Ship displacement.
		@param trim Trim angle [degrees].
		@return Ship draft, and bouyance center position. None if errors detected.
		"""
		if not self.ship:
			return None
		# Initial condition
		dens  = 1025
		bbox  = self.ship.Shape.BoundBox
		draft = bbox.ZMin
		dx	= bbox.XMax - bbox.XMin
		dy	= bbox.YMax - bbox.YMin
		w	 = 0.0
		xcb   = 0.0
		while(abs(disp - w)/disp > 0.01):
			draft = draft + (disp - w) / (dens*dx*dy)
			ww	= Hydrostatics.displacement(self.ship, draft, 0.0, trim, 0.0)
			w	 = 1000.0*ww[0]
			B	 = ww[1]
		return [draft,B]

	def computeGZ(self, draft, trim, roll):
		""" Compute GZ value.
		@param draft Ship draft.
		@param trim Ship trim angle [degrees].
		@param roll Ship roll angle [degrees].
		@return GZ value [m].
		"""
		# Get center of gravity (x coordinate not relevant)
		disp = self.computeDisplacement(trim, roll)
		G	= [disp[2], disp[3]]
		disp = disp[0]
		# Get bouyancy center (x coordinate not relevant)
		disp  = Hydrostatics.displacement(self.ship, draft, roll, trim, 0.0)
		B	 = [disp[1].y, disp[1].z]
		# GZ computation
		BG   = [G[0] - B[0], G[1] - B[1]]
		y	= BG[0]*math.cos(math.radians(roll)) - BG[1]*math.sin(math.radians(roll))
		z	= BG[0]*math.sin(math.radians(roll)) + BG[1]*math.cos(math.radians(roll))
		return y

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
