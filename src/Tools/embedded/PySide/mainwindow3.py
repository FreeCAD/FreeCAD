import sys

# sys.path.append("")

from PySide import QtCore, QtGui
import FreeCAD, FreeCADGui
import ctypes

from ui_mainwindow import Ui_MainWindow


class MainWindow(QtGui.QMainWindow):
    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)
        from PySide import QtNetwork

        # Webkit is used to create icons from SVG files. This could cause a deadlock
        # when setting up the internally used network interface. Doing this before
        # creating the icons fixes the issue.
        QtNetwork.QNetworkConfigurationManager()

    @QtCore.Slot()
    def on_actionEmbed_triggered(self):
        FreeCADGui.showMainWindow()
        hwnd = self.winId()
        PyCObject_AsVoidPtr = ctypes.PYFUNCTYPE(ctypes.c_void_p, ctypes.py_object)(
            ("PyCObject_AsVoidPtr", ctypes.pythonapi)
        )
        addr = PyCObject_AsVoidPtr(hwnd)
        FreeCADGui.embedToWindow(hex(addr))
        # Need version >= 0.16.5949
        class BlankWorkbench(FreeCADGui.Workbench):
            MenuText = "Blank"
            ToolTip = "Blank workbench"

            def Initialize(self):
                return

            def GetClassName(self):
                return "Gui::BlankWorkbench"

        FreeCADGui.addWorkbench(BlankWorkbench)
        FreeCADGui.activateWorkbench("BlankWorkbench")

    @QtCore.Slot()
    def on_actionDocument_triggered(self):
        FreeCAD.newDocument()

    @QtCore.Slot()
    def on_actionCube_triggered(self):
        FreeCAD.ActiveDocument.addObject("Part::Box")
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.ActiveView.fitAll()


app = QtGui.QApplication(sys.argv)
ui = Ui_MainWindow()
mw = MainWindow()
ui.setupUi(mw)
mw.resize(1200, 800)
mw.show()
app.exec_()
