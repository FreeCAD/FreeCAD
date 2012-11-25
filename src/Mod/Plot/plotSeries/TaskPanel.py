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
# matplotlib
import matplotlib
from matplotlib.lines import Line2D
import matplotlib.colors as Colors

class TaskPanel:
	def __init__(self):
		self.ui   = Paths.modulePath() + "/plotSeries/TaskPanel.ui"
		self.skip   = False
		self.item   = 0
		self.plt    = None

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
		form.items      = form.findChild(QtGui.QListWidget, "items")
		form.label      = form.findChild(QtGui.QLineEdit, "label")
		form.isLabel    = form.findChild(QtGui.QCheckBox, "isLabel")
		form.style      = form.findChild(QtGui.QComboBox, "lineStyle")
		form.marker     = form.findChild(QtGui.QComboBox, "markers")
		form.width      = form.findChild(QtGui.QDoubleSpinBox, "lineWidth")
		form.size       = form.findChild(QtGui.QSpinBox, "markerSize")
		form.color      = form.findChild(QtGui.QPushButton, "color")
		form.remove     = form.findChild(QtGui.QPushButton, "remove")
		self.form       = form
		self.retranslateUi()
		self.fillStyles()
		self.updateUI()
		QtCore.QObject.connect(form.items,  QtCore.SIGNAL("currentRowChanged(int)"),self.onItem)
		QtCore.QObject.connect(form.label,  QtCore.SIGNAL("editingFinished()"),self.onData)
		QtCore.QObject.connect(form.isLabel,QtCore.SIGNAL("stateChanged(int)"),self.onData)
		QtCore.QObject.connect(form.style,  QtCore.SIGNAL("currentIndexChanged(int)"),self.onData)
		QtCore.QObject.connect(form.marker, QtCore.SIGNAL("currentIndexChanged(int)"),self.onData)
		QtCore.QObject.connect(form.width,  QtCore.SIGNAL("valueChanged(double)"),self.onData)
		QtCore.QObject.connect(form.size,   QtCore.SIGNAL("valueChanged(int)"),self.onData)
		QtCore.QObject.connect(form.color,  QtCore.SIGNAL("pressed()"),self.onColor)
		QtCore.QObject.connect(form.remove, QtCore.SIGNAL("pressed()"),self.onRemove)
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
		self.form.setWindowTitle(QtGui.QApplication.translate("plot_series", "Configure series",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.isLabel.setText(QtGui.QApplication.translate("plot_series", "No label",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.remove.setText(QtGui.QApplication.translate("plot_series", "Remove serie",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "styleLabel").setText(QtGui.QApplication.translate("plot_series", "Line style",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "markerLabel").setText(QtGui.QApplication.translate("plot_series", "Marker",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.items.setToolTip(QtGui.QApplication.translate("plot_series", "List of available series",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.label.setToolTip(QtGui.QApplication.translate("plot_series", "Line title",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.isLabel.setToolTip(QtGui.QApplication.translate("plot_series", "If checked serie will not be considered for legend",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.style.setToolTip(QtGui.QApplication.translate("plot_series", "Line style",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.marker.setToolTip(QtGui.QApplication.translate("plot_series", "Marker style",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.width.setToolTip(QtGui.QApplication.translate("plot_series", "Line width",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.size.setToolTip(QtGui.QApplication.translate("plot_series", "Marker size",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.color.setToolTip(QtGui.QApplication.translate("plot_series", "Line and marker color",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.remove.setToolTip(QtGui.QApplication.translate("plot_series", "Removes this serie",
                                 None,QtGui.QApplication.UnicodeUTF8))

	def fillStyles(self):
		""" Fill style combo boxes. """
		# Line styles
		linestyles = Line2D.lineStyles.keys()
		for i in range(0,len(linestyles)):
			style = linestyles[i]
			string = "\'" + str(style) + "\' (" + Line2D.lineStyles[style] + ")"
			self.form.style.addItem(string)
		# Markers
		markers = Line2D.markers.keys()
		for i in range(0,len(markers)):
			marker = markers[i]
			string = "\'" + str(marker) + "\' (" + Line2D.markers[marker] + ")"
			self.form.marker.addItem(string)

	def onItem(self, row):
		""" Executed when selected item is modified. """
		if not self.skip:
			self.skip = True
			# Get selected item
			self.item = row
			# Call to update
			self.updateUI()
			self.skip = False

	def onData(self):
		""" Executed when selected item data is modified. """
		if not self.skip:
			self.skip = True
			plt = Plot.getPlot()
			if not plt:
				self.updateUI()
				return
			# Ensure that selected serie exist
			if self.item >= len(Plot.series()):
				self.updateUI()
				return
			# Set label
			serie = Plot.series()[self.item]
			if(self.form.isLabel.isChecked()):
				serie.name = None
				self.form.label.setEnabled(False)
			else:
				serie.name = self.form.label.text()
				self.form.label.setEnabled(True)
			# Set line style and marker
			style = self.form.style.currentIndex()
			linestyles = Line2D.lineStyles.keys()
			serie.line.set_linestyle(linestyles[style])
			marker = self.form.marker.currentIndex()
			markers = Line2D.markers.keys()
			serie.line.set_marker(markers[marker])
			# Set line width and marker size
			serie.line.set_linewidth(self.form.width.value())
			serie.line.set_markersize(self.form.size.value())
			plt.update()
			# Regenerate series labels
			self.setList()
			self.skip = False

	def onColor(self):
		""" Executed when color pallete is requested. """
		plt = Plot.getPlot()
		if not plt:
			self.updateUI()
			return
		# Ensure that selected serie exist
		if self.item >= len(Plot.series()):
			self.updateUI()
			return
		# Show widget to select color
		col = QtGui.QColorDialog.getColor()
		# Send color to widget and serie
		if col.isValid():
			serie = plt.series[self.item]
			self.form.color.setStyleSheet("background-color: rgb(%d, %d, %d);" % (col.red(),
                                          col.green(), col.blue()))
			serie.line.set_color((col.redF(), col.greenF(), col.blueF()))			
			plt.update()		

	def onRemove(self):
		""" Executed when data serie must be removed. """
		plt = Plot.getPlot()
		if not plt:
			self.updateUI()
			return
		# Ensure that selected serie exist
		if self.item >= len(Plot.series()):
			self.updateUI()
			return
		# Remove serie
		Plot.removeSerie(self.item)
		self.setList()
		self.updateUI()
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
		self.form.items.setEnabled(bool(plt))
		self.form.label.setEnabled(bool(plt))
		self.form.isLabel.setEnabled(bool(plt))
		self.form.style.setEnabled(bool(plt))
		self.form.marker.setEnabled(bool(plt))
		self.form.width.setEnabled(bool(plt))
		self.form.size.setEnabled(bool(plt))
		self.form.color.setEnabled(bool(plt))
		self.form.remove.setEnabled(bool(plt))
		if not plt:
			self.plt = plt
			self.form.items.clear()
			return
		self.skip = True
		# Refill list
		if self.plt != plt or len(Plot.series()) != self.form.items.count():
			self.plt = plt
			self.setList()
		# Ensure that have series
		if not len(Plot.series()):
			self.form.label.setEnabled(False)
			self.form.isLabel.setEnabled(False)
			self.form.style.setEnabled(False)
			self.form.marker.setEnabled(False)
			self.form.width.setEnabled(False)
			self.form.size.setEnabled(False)
			self.form.color.setEnabled(False)
			self.form.remove.setEnabled(False)
			return
		# Set label
		serie = Plot.series()[self.item]
		if serie.name == None:
			self.form.isLabel.setChecked(True)
			self.form.label.setEnabled(False)
			self.form.label.setText("")
		else:
			self.form.isLabel.setChecked(False)
			self.form.label.setText(serie.name)
		# Set line style and marker
		self.form.style.setCurrentIndex(0)
		linestyles = Line2D.lineStyles.keys()
		for i in range(0,len(linestyles)):
			style = linestyles[i]
			if style == serie.line.get_linestyle():
				self.form.style.setCurrentIndex(i)
		self.form.marker.setCurrentIndex(0)
		markers = Line2D.markers.keys()
		for i in range(0,len(markers)):
			marker = markers[i]
			if marker == serie.line.get_marker():
				self.form.marker.setCurrentIndex(i)
		# Set line width and marker size
		self.form.width.setValue(serie.line.get_linewidth())
		self.form.size.setValue(serie.line.get_markersize())
		# Set color
		color = Colors.colorConverter.to_rgb(serie.line.get_color())
		self.form.color.setStyleSheet("background-color: rgb(%d, %d, %d);" % (int(color[0]*255),
                                      int(color[1]*255), int(color[2]*255)))
		self.skip = False

	def setList(self):
		""" Setup UI controls values if possible """
		self.form.items.clear()
		series = Plot.series()
		for i in range(0,len(series)):
			serie = series[i]
			string = 'serie ' + str(i) + ': '
			if serie.name == None:
				string = string + '\"No label\"'
			else:
				string = string + serie.name
			self.form.items.addItem(string)
		# Ensure that selected item is correct
		if len(series) and self.item >= len(series):
			self.item = len(series)-1
			self.form.items.setCurrentIndex(self.item)

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
