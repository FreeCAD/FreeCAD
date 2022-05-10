# FreeCAD TemplatePyMod module  
# (c) 2011 Werner Mayer LGPL

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtGui,QtCore

class MyLineEdit(QtGui.QLineEdit):
    pass

class TaskWatcher:
    def __init__(self):
        self.commands = ["Part_Box", "Part_Sphere", "Part_Cylinder"]
        self.title = "Create primitives"
        self.icon = "Part_Sphere"
        self.widgets = [MyLineEdit()]
        self.widgets[0].setText("Line edit inside task box")
    def shouldShow(self):
        return App.ActiveDocument is not None

class TaskLineEdit:
    def __init__(self):
        self.widgets = [MyLineEdit()]
        self.widgets[0].setText("Line edit with no task box")
    def shouldShow(self):
        return True

class TaskWatcherFilter:
    def __init__(self):
        self.commands = ["Sketcher_NewSketch", "PartDesign_Fillet", "PartDesign_Chamfer"]
        self.filter = "SELECT Part::Feature SUBELEMENT Face COUNT 1"
        self.title = "Face tools"
        self.icon = "Part_Box"

class TaskPanel:
    def __init__(self):
        self.ui = App.getResourceDir() + "Mod/TemplatePyMod/TaskPanel.ui"

    def accept(self):
        return True

    def reject(self):
        return True

    def clicked(self, index):
        pass

    def open(self):
        pass

    def needsFullSpace(self):
        return False

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True

    def isAllowedAlterDocument(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def helpRequested(self):
        pass

    def setupUi(self):
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.pushButton = form.findChild(QtGui.QPushButton, "pushButton")
        form.listWidget = form.findChild(QtGui.QListWidget, "listWidget")
        self.form = form
        #Connect Signals and Slots
        QtCore.QObject.connect(form.pushButton, QtCore.SIGNAL("clicked()"), self.addElement)

    def getMainWindow(self):
        "returns the main window"
        # using QtGui.QApplication.activeWindow() isn't very reliable because if another
        # widget than the mainwindow is active (e.g. a dialog) the wrong widget is
        # returned
        toplevel = QtGui.QApplication.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise RuntimeError("No main window found")

    def addElement(self):
        item=QtGui.QInputDialog.getText(self.form, 'Add item', 'Enter:')
        if item[1]:
            self.form.listWidget.addItem(item[0])

class TaskCalendar:
    def __init__(self):
        self.form = QtGui.QCalendarWidget()

class TaskManyTaskBoxes:
    "illustrates how to add several taskboxes"
    def __init__(self):
        widget1 = QtGui.QCalendarWidget()
        widget2 = QtGui.QWidget()
        widget2.setWindowTitle("My Test Box")
        text = QtGui.QLabel("testBox",widget2)
        text.setObjectName("label")
        self.form = [widget1,widget2]

def createTask():
    Gui.Control.addTaskWatcher([TaskWatcher(), TaskLineEdit(), TaskWatcherFilter()])
    panel = TaskCalendar()
    #panel = TaskPanel()
    Gui.Control.showDialog(panel)
    #panel.setupUi()
    return panel
