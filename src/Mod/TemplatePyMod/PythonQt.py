"""
Examples for customizing the FreeCAD application with PySide facilities.
(c) 2007 Werner Mayer LGPL
"""

__author__ = "Werner Mayer <werner.wm.mayer@gmx.de>"

from PySide import QtCore,QtGui
import FreeCADGui, __main__

class MainWindow:
	def __init__(self):
		self.app = QtGui.qApp
		self.mw = FreeCADGui.getMainWindow()
		self.dock = {}

	def setWindowTitle(self, name):
		self.mw.setWindowTitle(name)

	def addCalendar(self):
		d = QtGui.QDockWidget()
		d.setWindowTitle("Calendar")
		c = QtGui.QCalendarWidget()
		d.setWidget(c)
		self.mw.addDockWidget(QtCore.Qt.RightDockWidgetArea,d)
		self.dock[d] = c

	def information(self, title, text):
		QtGui.QMessageBox.information(self.mw, title, text)

	def warning(self, title, text):
		QtGui.QMessageBox.warning(self.mw, title, text)

	def critical(self, title, text):
		QtGui.QMessageBox.critical(self.mw, title, text)

	def question(self, title, text):
		QtGui.QMessageBox.question(self.mw, title, text)

	def aboutQt(self):
		QtGui.QMessageBox.aboutQt(self.mw, self.mw.tr("About Qt"))


class PythonQtWorkbench (__main__.Workbench):
	"Python Qt workbench object"
	Icon = "python"
	MenuText = "PySide sandbox"
	ToolTip = "Python Qt workbench"

	def __init__(self):
		self.mw = FreeCADGui.getMainWindow()
		self.dock = {}
		self.item = []

	def information(self):
		QtGui.QMessageBox.information(self.mw, "Info", "This is an information")

	def warning(self):
		QtGui.QMessageBox.warning(self.mw, "Warning", "This is a warning")

	def critical(self):
		QtGui.QMessageBox.critical(self.mw, "Error", "This is an error")

	def Initialize(self):
		self.menu = QtGui.QMenu()
		self.menu.setTitle("Python Qt")
		self.item.append(self.menu.addAction("Test 1"))
		self.item.append(self.menu.addAction("Test 2"))
		self.item.append(self.menu.addAction("Test 3"))

		QtCore.QObject.connect(self.item[0], QtCore.SIGNAL("triggered()"), self.information)
		QtCore.QObject.connect(self.item[1], QtCore.SIGNAL("triggered()"), self.warning)
		QtCore.QObject.connect(self.item[2], QtCore.SIGNAL("triggered()"), self.critical)

	def Activated(self):
		self.__title__ = self.mw.windowTitle()
		self.mw.setWindowTitle("FreeCAD -- PythonQt")

		d = QtGui.QDockWidget()
		d.setWindowTitle("Calendar")
		c = QtGui.QCalendarWidget()
		d.setWidget(c)
		self.mw.addDockWidget(QtCore.Qt.RightDockWidgetArea,d)
		self.dock[d] = c

		bar = self.mw.menuBar()
		a=bar.actions()
		for i in a:
			if i.objectName() == "&Windows":
				break
		bar.insertMenu(i, self.menu)
		self.menu.setTitle("Python Qt")
		self.menu.menuAction().setVisible(True)

	def Deactivated(self):
		self.mw.setWindowTitle(self.__title__)
		self.dock.clear()

FreeCADGui.addWorkbench(PythonQtWorkbench)
