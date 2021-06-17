#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import os

import six

import FreeCAD as App
import FreeCADGui as Gui
import Plot_rc  # include resources, icons, ui files

from PySide import QtGui, QtCore

import Plot
from plotUtils import Paths


# The module is used to prevent complaints from code checkers (flake8)
bool(Plot_rc.__name__)


class TaskPanel:
    def __init__(self):
        self.name = "plot save"
        self.ui = ":/ui/TaskPanel_plotSave.ui"
        self.form = Gui.PySideUic.loadUi(self.ui)

    def accept(self):
        plt = Plot.getPlot()
        if not plt:
            msg = QtGui.QApplication.translate(
                "plot_console",
                "Plot document must be selected in order to save it",
                None)
            App.Console.PrintError(msg + "\n")
            return False
        path = six.text_type(self.form.path.text())
        size = (self.form.sizeX.value(), self.form.sizeY.value())
        dpi = self.form.dpi.value()
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
        self.form.path = self.widget(QtGui.QLineEdit, "path")
        self.form.pathButton = self.widget(QtGui.QPushButton, "pathButton")
        self.form.sizeX = self.widget(QtGui.QDoubleSpinBox, "sizeX")
        self.form.sizeY = self.widget(QtGui.QDoubleSpinBox, "sizeY")
        self.form.dpi = self.widget(QtGui.QSpinBox, "dpi")
        self.retranslateUi()
        home = os.getenv('USERPROFILE') or os.getenv('HOME')
        self.form.path.setText(os.path.join(home, "plot.png"))
        self.updateUI()
        QtCore.QObject.connect(
            self.form.pathButton,
            QtCore.SIGNAL("pressed()"),
            self.onPathButton)
        QtCore.QObject.connect(
            Plot.getMdiArea(),
            QtCore.SIGNAL("subWindowActivated(QMdiSubWindow*)"),
            self.onMdiArea)
        return False

    def getMainWindow(self):
        toplevel = QtGui.QApplication.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise RuntimeError("No main window found")

    def widget(self, class_id, name):
        """Return the selected widget.

        Keyword arguments:
        class_id -- Class identifier
        name -- Name of the widget
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        return form.findChild(class_id, name)

    def retranslateUi(self):
        """Set the user interface locale strings."""
        self.form.setWindowTitle(QtGui.QApplication.translate(
            "plot_save",
            "Save figure",
            None))
        self.widget(QtGui.QLabel, "sizeLabel").setText(
            QtGui.QApplication.translate(
                "plot_save",
                "Inches",
                None))
        self.widget(QtGui.QLabel, "dpiLabel").setText(
            QtGui.QApplication.translate(
                "plot_save",
                "Dots per Inch",
                None))
        self.widget(QtGui.QLineEdit, "path").setToolTip(
            QtGui.QApplication.translate(
                "plot_save",
                "Output image file path",
                None))
        self.widget(QtGui.QPushButton, "pathButton").setToolTip(
            QtGui.QApplication.translate(
                "plot_save",
                "Show a file selection dialog",
                None))
        self.widget(QtGui.QDoubleSpinBox, "sizeX").setToolTip(
            QtGui.QApplication.translate(
                "plot_save",
                "X image size",
                None))
        self.widget(QtGui.QDoubleSpinBox, "sizeY").setToolTip(
            QtGui.QApplication.translate(
                "plot_save",
                "Y image size",
                None))
        self.widget(QtGui.QSpinBox, "dpi").setToolTip(
            QtGui.QApplication.translate(
                "plot_save",
                "Dots per point, with size will define output image"
                " resolution",
                None))

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
        dpi = fig.get_dpi()
        self.form.sizeX.setValue(size[0])
        self.form.sizeY.setValue(size[1])
        self.form.dpi.setValue(dpi)

    def onPathButton(self):
        """Executed when the path selection button is pressed."""
        path = self.form.path.text()
        file_choices = ("Portable Network Graphics (*.png)|*.png;;"
                        "Portable Document Format (*.pdf)|*.pdf;;"
                        "PostScript (*.ps)|*.ps;;"
                        "Encapsulated PostScript (*.eps)|*.eps")
        path = QtGui.QFileDialog.getSaveFileName(None,
                                                 'Save figure',
                                                 path,
                                                 file_choices)
        if path and path[0]:
            self.form.path.setText(path[0])

    def onMdiArea(self, subWin):
        """Executed when a new window is selected on the mdi area.

        Keyword arguments:
        subWin -- Selected window.
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
