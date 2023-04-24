import sys
from PySide2 import QtWidgets
import FreeCADGui


class MainWindow(QtWidgets.QMainWindow):
    def showEvent(self, event):
        FreeCADGui.showMainWindow()
        self.setCentralWidget(FreeCADGui.getMainWindow())


app = QtWidgets.QApplication(sys.argv)
mw = MainWindow()
mw.resize(1200, 800)
mw.show()

# must be done a few times to update the GUI
app.processEvents()
app.processEvents()
app.processEvents()

import Part

cube = Part.makeBox(2, 2, 2)
# creates a document and a Part feature with the cube
Part.show(cube)
app.processEvents()
app.processEvents()
