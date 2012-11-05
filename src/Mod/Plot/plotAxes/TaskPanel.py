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
import Plot
from plotUtils import Paths

class TaskPanel:
	def __init__(self):
		self.ui   = Paths.modulePath() + "/plotAxes/TaskPanel.ui"
		self.skip = False

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
		mw              = self.getMainWindow()
		form            = mw.findChild(QtGui.QWidget, "TaskPanel")
		form.axId       = form.findChild(QtGui.QSpinBox, "axesIndex")
		form.new        = form.findChild(QtGui.QPushButton, "newAxesButton")
		form.remove     = form.findChild(QtGui.QPushButton, "delAxesButton")
		form.all        = form.findChild(QtGui.QCheckBox, "allAxes")
		form.xMin       = form.findChild(QtGui.QSlider, "posXMin")
		form.xMax       = form.findChild(QtGui.QSlider, "posXMax")
		form.yMin       = form.findChild(QtGui.QSlider, "posYMin")
		form.yMax       = form.findChild(QtGui.QSlider, "posYMax")
		form.xAlign     = form.findChild(QtGui.QComboBox, "xAlign")
		form.yAlign     = form.findChild(QtGui.QComboBox, "yAlign")
		form.xOffset    = form.findChild(QtGui.QSpinBox, "xOffset")
		form.yOffset    = form.findChild(QtGui.QSpinBox, "yOffset")
		form.xAuto      = form.findChild(QtGui.QCheckBox, "xAuto")
		form.yAuto      = form.findChild(QtGui.QCheckBox, "yAuto")
		form.xSMin      = form.findChild(QtGui.QLineEdit, "xMin")
		form.xSMax      = form.findChild(QtGui.QLineEdit, "xMax")
		form.ySMin      = form.findChild(QtGui.QLineEdit, "yMin")
		form.ySMax      = form.findChild(QtGui.QLineEdit, "yMax")
		self.form       = form
		self.retranslateUi()
		# Look for active axes if can
		axId = 0
		plt = Plot.getPlot()
		if plt:
			while plt.axes != plt.axesList[axId]:
				axId = axId + 1
			form.axId.setValue(axId)
		self.updateUI()
		QtCore.QObject.connect(form.axId,   QtCore.SIGNAL('valueChanged(int)'),self.onAxesId)
		QtCore.QObject.connect(form.new,    QtCore.SIGNAL("pressed()"),self.onNew)
		QtCore.QObject.connect(form.remove, QtCore.SIGNAL("pressed()"),self.onRemove)
		QtCore.QObject.connect(form.xMin,   QtCore.SIGNAL("valueChanged(int)"),self.onDims)
		QtCore.QObject.connect(form.xMax,   QtCore.SIGNAL("valueChanged(int)"),self.onDims)
		QtCore.QObject.connect(form.yMin,   QtCore.SIGNAL("valueChanged(int)"),self.onDims)
		QtCore.QObject.connect(form.yMax,   QtCore.SIGNAL("valueChanged(int)"),self.onDims)
		QtCore.QObject.connect(form.xAlign, QtCore.SIGNAL("currentIndexChanged(int)"),self.onAlign)
		QtCore.QObject.connect(form.yAlign, QtCore.SIGNAL("currentIndexChanged(int)"),self.onAlign)
		QtCore.QObject.connect(form.xOffset,QtCore.SIGNAL("valueChanged(int)"),self.onOffset)
		QtCore.QObject.connect(form.yOffset,QtCore.SIGNAL("valueChanged(int)"),self.onOffset)
		QtCore.QObject.connect(form.xAuto,  QtCore.SIGNAL("stateChanged(int)"),self.onScales)
		QtCore.QObject.connect(form.yAuto,  QtCore.SIGNAL("stateChanged(int)"),self.onScales)
		QtCore.QObject.connect(form.xSMin,  QtCore.SIGNAL("editingFinished()"),self.onScales)
		QtCore.QObject.connect(form.xSMax,  QtCore.SIGNAL("editingFinished()"),self.onScales)
		QtCore.QObject.connect(form.ySMin,  QtCore.SIGNAL("editingFinished()"),self.onScales)
		QtCore.QObject.connect(form.ySMax,  QtCore.SIGNAL("editingFinished()"),self.onScales)
		QtCore.QObject.connect(Plot.getMdiArea(),QtCore.SIGNAL("subWindowActivated(QMdiSubWindow*)"),self.onMdiArea)
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

	def retranslateUi(self):
		""" Set user interface locale strings. 
		"""
		self.form.setWindowTitle(QtGui.QApplication.translate("plot_axes", "Configure axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "axesLabel").setText(QtGui.QApplication.translate("plot_axes", "Active axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.all.setText(QtGui.QApplication.translate("plot_axes", "Apply to all axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "dimLabel").setText(QtGui.QApplication.translate("plot_axes", "Dimensions",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "xPosLabel").setText(QtGui.QApplication.translate("plot_axes", "X axis position",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "yPosLabel").setText(QtGui.QApplication.translate("plot_axes", "Y axis position",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "scalesLabel").setText(QtGui.QApplication.translate("plot_axes", "Scales",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.xAuto.setText(QtGui.QApplication.translate("plot_axes", "X auto",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.yAuto.setText(QtGui.QApplication.translate("plot_axes", "Y auto",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QCheckBox, "allAxes").setText(QtGui.QApplication.translate("plot_axes", "Apply to all axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "dimLabel").setText(QtGui.QApplication.translate("plot_axes", "Dimensions",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "xPosLabel").setText(QtGui.QApplication.translate("plot_axes", "X axis position",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "yPosLabel").setText(QtGui.QApplication.translate("plot_axes", "Y axis position",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.axId.setToolTip(QtGui.QApplication.translate("plot_axes", "Index of the active axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.new.setToolTip(QtGui.QApplication.translate("plot_axes", "Add new axes to the plot",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.remove.setToolTip(QtGui.QApplication.translate("plot_axes", "Remove selected axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.all.setToolTip(QtGui.QApplication.translate("plot_axes", "Check it to apply transformations to all axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.xMin.setToolTip(QtGui.QApplication.translate("plot_axes", "Left bound of axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.xMax.setToolTip(QtGui.QApplication.translate("plot_axes", "Right bound of axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.yMin.setToolTip(QtGui.QApplication.translate("plot_axes", "Bottom bound of axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.yMax.setToolTip(QtGui.QApplication.translate("plot_axes", "Top bound of axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.xOffset.setToolTip(QtGui.QApplication.translate("plot_axes", "Outward offset of X axis",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.yOffset.setToolTip(QtGui.QApplication.translate("plot_axes", "Outward offset of Y axis",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.xAuto.setToolTip(QtGui.QApplication.translate("plot_axes", "X axis scale autoselection",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.yAuto.setToolTip(QtGui.QApplication.translate("plot_axes", "Y axis scale autoselection",
                                 None,QtGui.QApplication.UnicodeUTF8))

	def onAxesId(self, value):
		""" Executed when axes index is modified. """
		if not self.skip:
			self.skip = True
			# UI control in some special plot cases
			plt = Plot.getPlot()
			if not plt:
				self.updateUI()
				self.skip = False
				return
			# UI control in most cases
			self.form.axId.setMaximum(len(plt.axesList))
			if self.form.axId.value() >= len(plt.axesList):
				self.form.axId.setValue(len(plt.axesList)-1)
			# Send new control to Plot instance
			plt.setActiveAxes(self.form.axId.value())
			self.updateUI()
			self.skip = False

	def onNew(self):
		""" Executed when new axes must be created. """
		plt = Plot.getPlot()
		if not plt:
			self.updateUI()
			return
		Plot.addNewAxes()
		self.form.axId.setValue(len(plt.axesList)-1)
		plt.update()

	def onRemove(self):
		""" Executed when axes must be deleted. """
		plt = Plot.getPlot()
		if not plt:
			self.updateUI()
			return
		# Don't remove first axes
		if not self.form.axId.value():
			msg = QtGui.QApplication.translate("plot_console", "Axes 0 can not be deleted",
                                 None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg+"\n")
			return
		# Remove axes
		ax = plt.axes
		ax.set_axis_off()
		plt.axesList.pop(self.form.axId.value())
		# Ensure that active axes is correct
		index = min(self.form.axId.value(), len(plt.axesList)-1)
		self.form.axId.setValue(index)
		plt.update()

	def onDims(self, value):
		""" Executed when axes dims have been modified. """
		# Get apply environment
		plt = Plot.getPlot()
		if not plt:
			self.updateUI()
			return
		axesList = [plt.axes]
		if self.form.all.isChecked():
			axesList = plt.axesList
		# Set new dimensions
		xmin = self.form.xMin.value() / 100.0
		xmax = self.form.xMax.value() / 100.0
		ymin = self.form.yMin.value() / 100.0
		ymax = self.form.yMax.value() / 100.0
		for axes in axesList:
			axes.set_position([xmin, ymin, xmax-xmin, ymax-ymin])
		plt.update()

	def onAlign(self, value):
		""" Executed when axes align have been modified. """
		# Get apply environment
		plt = Plot.getPlot()
		if not plt:
			self.updateUI()
			return
		axesList = [plt.axes]
		if self.form.all.isChecked():
			axesList = plt.axesList
		# Set new alignement
		for axes in axesList:
			if self.form.xAlign.currentIndex() == 0:
				axes.xaxis.tick_bottom()
				axes.spines['bottom'].set_color((0.0,0.0,0.0))
				axes.spines['top'].set_color('none')
				axes.xaxis.set_ticks_position('bottom')
				axes.xaxis.set_label_position('bottom')
			else:
				axes.xaxis.tick_top()
				axes.spines['top'].set_color((0.0,0.0,0.0))
				axes.spines['bottom'].set_color('none')
				axes.xaxis.set_ticks_position('top')
				axes.xaxis.set_label_position('top')
			if self.form.yAlign.currentIndex() == 0:
				axes.yaxis.tick_left()
				axes.spines['left'].set_color((0.0,0.0,0.0))
				axes.spines['right'].set_color('none')
				axes.yaxis.set_ticks_position('left')
				axes.yaxis.set_label_position('left')
			else:
				axes.yaxis.tick_right()
				axes.spines['right'].set_color((0.0,0.0,0.0))
				axes.spines['left'].set_color('none')
				axes.yaxis.set_ticks_position('right')
				axes.yaxis.set_label_position('right')
		plt.update()

	def onOffset(self, value):
		""" Executed when axes offsets have been modified. """
		# Get apply environment
		plt = Plot.getPlot()
		if not plt:
			self.updateUI()
			return
		axesList = [plt.axes]
		if self.form.all.isChecked():
			axesList = plt.axesList
		# Set new offset
		for axes in axesList:
			# For some reason, modify spines offset erase axes labels, so we
			# need store it in order to regenerate later
			x  = axes.get_xlabel()
			y  = axes.get_ylabel()
			for loc, spine in axes.spines.iteritems():
				if loc in ['bottom', 'top']:
					spine.set_position(('outward',self.form.xOffset.value()))
				if loc in ['left', 'right']:
					spine.set_position(('outward',self.form.yOffset.value()))
			# Now we can restore axes labels
			Plot.xlabel(unicode(x))
			Plot.ylabel(unicode(y))
		plt.update()

	def onScales(self):
		""" Executed when axes scales have been modified. """
		# Get apply environment
		plt = Plot.getPlot()
		if not plt:
			self.updateUI()
			return
		axesList = [plt.axes]
		if self.form.all.isChecked():
			axesList = plt.axesList
		if not self.skip:
			self.skip = True
			# X axis
			if self.form.xAuto.isChecked():
				for ax in axesList:
					ax.set_autoscalex_on(True)
				self.form.xSMin.setEnabled(False)
				self.form.xSMax.setEnabled(False)
				lim = plt.axes.get_xlim()
				self.form.xSMin.setText(str(lim[0]))
				self.form.xSMax.setText(str(lim[1]))
			else:
				self.form.xSMin.setEnabled(True)
				self.form.xSMax.setEnabled(True)
				try:
					xMin = float(self.form.xSMin.text())
				except:
					xMin = plt.axes.get_xlim()[0]
					self.form.xSMin.setText(str(xMin))
				try:
					xMax = float(self.form.xSMax.text())
				except:
					xMax = plt.axes.get_xlim()[1]
					self.form.xSMax.setText(str(xMax))
				for ax in axesList:
					ax.set_xlim(( xMin,xMax ))
			# Y axis
			if self.form.yAuto.isChecked():
				for ax in axesList:
					ax.set_autoscaley_on(True)
				self.form.ySMin.setEnabled(False)
				self.form.ySMax.setEnabled(False)
				lim = plt.axes.get_ylim()
				self.form.ySMin.setText(str(lim[0]))
				self.form.ySMax.setText(str(lim[1]))
			else:
				self.form.ySMin.setEnabled(True)
				self.form.ySMax.setEnabled(True)
				try:
					yMin = float(self.form.ySMin.text())
				except:
					yMin = plt.axes.get_ylim()[0]
					self.form.ySMin.setText(str(yMin))
				try:
					yMax = float(self.form.ySMax.text())
				except:
					yMax = plt.axes.get_ylim()[1]
					self.form.ySMax.setText(str(yMax))
				for ax in axesList:
					ax.set_ylim(( yMin,yMax ))
			plt.update()
			self.skip = False

	def onMdiArea(self, subWin):
		""" Executed when window is selected on mdi area.
		@param subWin Selected window. 
		"""
		plt = Plot.getPlot()
		if plt != subWin:
			self.updateUI()

	def updateUI(self):
		""" Setup UI controls values if possible """
		plt = Plot.getPlot()
		self.form.axId.setEnabled(bool(plt))
		self.form.new.setEnabled(bool(plt))
		self.form.remove.setEnabled(bool(plt))
		self.form.all.setEnabled(bool(plt))
		self.form.xMin.setEnabled(bool(plt))
		self.form.xMax.setEnabled(bool(plt))
		self.form.yMin.setEnabled(bool(plt))
		self.form.yMax.setEnabled(bool(plt))
		self.form.xAlign.setEnabled(bool(plt))
		self.form.yAlign.setEnabled(bool(plt))
		self.form.xOffset.setEnabled(bool(plt))
		self.form.yOffset.setEnabled(bool(plt))
		self.form.xAuto.setEnabled(bool(plt))
		self.form.yAuto.setEnabled(bool(plt))
		self.form.xSMin.setEnabled(bool(plt))
		self.form.xSMax.setEnabled(bool(plt))
		self.form.ySMin.setEnabled(bool(plt))
		self.form.ySMax.setEnabled(bool(plt))
		if not plt:
			return
		# Ensure that active axes is correct
		index = min(self.form.axId.value(), len(plt.axesList)-1)
		self.form.axId.setValue(index)
		# Set dimensions
		ax = plt.axes
		bb = ax.get_position()
		self.form.xMin.setValue(int(100*bb._get_xmin()))
		self.form.xMax.setValue(int(100*bb._get_xmax()))
		self.form.yMin.setValue(int(100*bb._get_ymin()))
		self.form.yMax.setValue(int(100*bb._get_ymax()))
		# Set alignment and offset
		xPos = ax.xaxis.get_ticks_position()
		yPos = ax.yaxis.get_ticks_position()
		xOffset = ax.spines['bottom'].get_position()[1]
		yOffset = ax.spines['left'].get_position()[1]
		if xPos == 'bottom' or xPos == 'default':
			self.form.xAlign.setCurrentIndex(0)
		else:
			self.form.xAlign.setCurrentIndex(1)
		self.form.xOffset.setValue(xOffset)
		if yPos == 'left' or yPos == 'default':
			self.form.yAlign.setCurrentIndex(0)
		else:
			self.form.yAlign.setCurrentIndex(1)
		self.form.yOffset.setValue(yOffset)
		# Set scales
		if ax.get_autoscalex_on():
			self.form.xAuto.setChecked(True)
			self.form.xSMin.setEnabled(False)
			self.form.xSMax.setEnabled(False)
		else:
			self.form.xAuto.setChecked(False)
			self.form.xSMin.setEnabled(True)
			self.form.xSMax.setEnabled(True)
		lim = ax.get_xlim()
		self.form.xSMin.setText(str(lim[0]))
		self.form.xSMax.setText(str(lim[1]))
		if ax.get_autoscaley_on():
			self.form.yAuto.setChecked(True)
			self.form.ySMin.setEnabled(False)
			self.form.ySMax.setEnabled(False)
		else:
			self.form.yAuto.setChecked(False)
			self.form.ySMin.setEnabled(True)
			self.form.ySMax.setEnabled(True)
		lim = ax.get_ylim()
		self.form.ySMin.setText(str(lim[0]))
		self.form.ySMax.setText(str(lim[1]))

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
