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

import os
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
		self.ui = Paths.modulePath() + "/plotSave/TaskPanel.ui"

	def accept(self):
		plt = Plot.getPlot()
		if not plt:
			msg = QtGui.QApplication.translate("plot_console", "Plot document must be selected in order to save it",
                                 None,QtGui.QApplication.UnicodeUTF8)
			App.Console.PrintError(msg+"\n")
			return False
		path = unicode(self.form.path.text())
		size = (self.form.sizeX.value(), self.form.sizeY.value())
		dpi  = self.form.dpi.value()
		Plot.save(path, size, dpi)
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
		form.path       = form.findChild(QtGui.QLineEdit, "path")
		form.pathButton = form.findChild(QtGui.QPushButton, "pathButton")
		form.sizeX      = form.findChild(QtGui.QDoubleSpinBox, "sizeX")
		form.sizeY      = form.findChild(QtGui.QDoubleSpinBox, "sizeY")
		form.dpi        = form.findChild(QtGui.QSpinBox, "dpi")
		self.form       = form
		self.retranslateUi()
		QtCore.QObject.connect(form.pathButton,QtCore.SIGNAL("pressed()"),self.onPathButton)
		QtCore.QObject.connect(Plot.getMdiArea(),QtCore.SIGNAL("subWindowActivated(QMdiSubWindow*)"),self.onMdiArea)
		home = os.getenv('USERPROFILE') or os.getenv('HOME')
		form.path.setText(os.path.join(home,"plot.png"))
		self.updateUI()
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
		self.form.setWindowTitle(QtGui.QApplication.translate("plot_save", "Save figure",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "sizeLabel").setText(QtGui.QApplication.translate("plot_save", "Inches",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.findChild(QtGui.QLabel, "dpiLabel").setText(QtGui.QApplication.translate("plot_save", "Dots per Inch",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.path.setToolTip(QtGui.QApplication.translate("plot_save", "Output image file path",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.pathButton.setToolTip(QtGui.QApplication.translate("plot_save", "Show a file selection dialog",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.sizeX.setToolTip(QtGui.QApplication.translate("plot_save", "X image size",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.sizeY.setToolTip(QtGui.QApplication.translate("plot_save", "Y image size",
                                 None,QtGui.QApplication.UnicodeUTF8))
		self.form.dpi.setToolTip(QtGui.QApplication.translate("plot_save", "Dots per point, with size will define output image resolution",
                                 None,QtGui.QApplication.UnicodeUTF8))

	def updateUI(self):
		""" Setup UI controls values if possible """
		plt = Plot.getPlot()
		self.form.path.setEnabled(bool(plt))
		self.form.pathButton.setEnabled(bool(plt))
		self.form.sizeX.setEnabled(bool(plt))
		self.form.sizeY.setEnabled(bool(plt))
		self.form.dpi.setEnabled(bool(plt))
		if not plt:
			return
		fig = plt.fig
		size = fig.get_size_inches()
		dpi  = fig.get_dpi()
		self.form.sizeX.setValue(size[0])
		self.form.sizeY.setValue(size[1])
		self.form.dpi.setValue(dpi)

	def onPathButton(self):
		""" Executed when path button is pressed. 
		"""
		path = self.form.path.text()
		file_choices = "Portable Network Graphics (*.png)|*.png;;Portable Document Format (*.pdf)|*.pdf;;PostScript (*.ps)|*.ps;;Encapsulated PostScript (*.eps)|*.eps"
		path = QtGui.QFileDialog.getSaveFileName(None, 'Save figure', path, file_choices)
		if path:
			self.form.path.setText(path)

	def onMdiArea(self, subWin):
		""" Executed when window is selected on mdi area.
		@param subWin Selected window. 
		"""
		plt = Plot.getPlot()
		if plt != subWin:
			self.updateUI()

def createTask():
	panel = TaskPanel()
	Gui.Control.showDialog(panel)
	if panel.setupUi():
		Gui.Control.closeDialog(panel)
		return None
	return panel
