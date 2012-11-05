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
		self.ui   = Paths.modulePath() + "/plotLabels/TaskPanel.ui"
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
		form.title      = form.findChild(QtGui.QLineEdit, "title")
		form.titleSize  = form.findChild(QtGui.QSpinBox, "titleSize")
		form.xLabel     = form.findChild(QtGui.QLineEdit, "titleX")
		form.xSize      = form.findChild(QtGui.QSpinBox, "xSize")
		form.yLabel     = form.findChild(QtGui.QLineEdit, "titleY")
		form.ySize      = form.findChild(QtGui.QSpinBox, "ySize")
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
		QtCore.QObject.connect(form.axId,     QtCore.SIGNAL('valueChanged(int)'),self.onAxesId)
		QtCore.QObject.connect(form.title,    QtCore.SIGNAL("editingFinished()"),self.onLabels)
		QtCore.QObject.connect(form.xLabel,   QtCore.SIGNAL("editingFinished()"),self.onLabels)
		QtCore.QObject.connect(form.yLabel,   QtCore.SIGNAL("editingFinished()"),self.onLabels)
		QtCore.QObject.connect(form.titleSize,QtCore.SIGNAL("valueChanged(int)"),self.onFontSizes)
		QtCore.QObject.connect(form.xSize,    QtCore.SIGNAL("valueChanged(int)"),self.onFontSizes)
		QtCore.QObject.connect(form.ySize,    QtCore.SIGNAL("valueChanged(int)"),self.onFontSizes)
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
		self.form.setWindowTitle(QtGui.QApplication.translate("plot_labels", "Set labels",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "axesLabel").setText(QtGui.QApplication.translate("plot_labels", "Active axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "titleLabel").setText(QtGui.QApplication.translate("plot_labels", "Title",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "xLabel").setText(QtGui.QApplication.translate("plot_labels", "X label",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "yLabel").setText(QtGui.QApplication.translate("plot_labels", "Y label",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.axId.setToolTip(QtGui.QApplication.translate("plot_labels", "Index of the active axes",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.title.setToolTip(QtGui.QApplication.translate("plot_labels", "Title (associated to active axes)",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.titleSize.setToolTip(QtGui.QApplication.translate("plot_labels", "Title font size",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.xLabel.setToolTip(QtGui.QApplication.translate("plot_labels", "X axis title",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.xSize.setToolTip(QtGui.QApplication.translate("plot_labels", "X axis title font size",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.yLabel.setToolTip(QtGui.QApplication.translate("plot_labels", "Y axis title",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.ySize.setToolTip(QtGui.QApplication.translate("plot_labels", "Y axis title font size",
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

	def onLabels(self):
		""" Executed when labels have been modified. """
		plt = Plot.getPlot()
		if not plt:
			self.updateUI()
			return
		# Set labels
		Plot.title(unicode(self.form.title.text()))
		Plot.xlabel(unicode(self.form.xLabel.text()))
		Plot.ylabel(unicode(self.form.yLabel.text()))
		plt.update()

	def onFontSizes(self, value):
		""" Executed when font sizes have been modified. """
		# Get apply environment
		plt = Plot.getPlot()
		if not plt:
			self.updateUI()
			return
		# Set font sizes
		ax = plt.axes
		ax.title.set_fontsize(self.form.titleSize.value())
		ax.xaxis.label.set_fontsize(self.form.xSize.value())
		ax.yaxis.label.set_fontsize(self.form.ySize.value())
		plt.update()

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
		self.form.title.setEnabled(bool(plt))
		self.form.titleSize.setEnabled(bool(plt))
		self.form.xLabel.setEnabled(bool(plt))
		self.form.xSize.setEnabled(bool(plt))
		self.form.yLabel.setEnabled(bool(plt))
		self.form.ySize.setEnabled(bool(plt))
		if not plt:
			return
		# Ensure that active axes is correct
		index = min(self.form.axId.value(), len(plt.axesList)-1)
		self.form.axId.setValue(index)
		# Store data before starting changing it.
		ax = plt.axes
		t  = ax.get_title()
		x  = ax.get_xlabel()
		y  = ax.get_ylabel()
		tt = ax.title.get_fontsize()
		xx = ax.xaxis.label.get_fontsize()
		yy = ax.yaxis.label.get_fontsize()
		# Set labels
		self.form.title.setText(t)
		self.form.xLabel.setText(x)
		self.form.yLabel.setText(y)
		# Set font sizes
		self.form.titleSize.setValue(tt)
		self.form.xSize.setValue(xx)
		self.form.ySize.setValue(yy)

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
