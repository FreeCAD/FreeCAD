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

from PySide import QtGui, QtCore

import Plot
from plotUtils import Paths


class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/plotSave/TaskPanel.ui"

    def accept(self):
        plt = Plot.getPlot()
        if not plt:
            msg = QtGui.QApplication.translate(
                "plot_console",
                "Plot document must be selected in order to save it",
                None)
            App.Console.PrintError(msg + "\n")
            return False
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.path = self.widget(QtGui.QLineEdit, "path")
        form.sizeX = self.widget(QtGui.QDoubleSpinBox, "sizeX")
        form.sizeY = self.widget(QtGui.QDoubleSpinBox, "sizeY")
        form.dpi = self.widget(QtGui.QSpinBox, "dpi")
        path = six.text_type(form.path.text())
        size = (form.sizeX.value(), form.sizeY.value())
        dpi = form.dpi.value()
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
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.path = self.widget(QtGui.QLineEdit, "path")
        form.pathButton = self.widget(QtGui.QPushButton, "pathButton")
        form.sizeX = self.widget(QtGui.QDoubleSpinBox, "sizeX")
        form.sizeY = self.widget(QtGui.QDoubleSpinBox, "sizeY")
        form.dpi = self.widget(QtGui.QSpinBox, "dpi")
        self.form = form
        self.retranslateUi()
        QtCore.QObject.connect(
            form.pathButton,
            QtCore.SIGNAL("pressed()"),
            self.onPathButton)
        QtCore.QObject.connect(
            Plot.getMdiArea(),
            QtCore.SIGNAL("subWindowActivated(QMdiSubWindow*)"),
            self.onMdiArea)
        home = os.getenv('USERPROFILE') or os.getenv('HOME')
        form.path.setText(os.path.join(home, "plot.png"))
        self.updateUI()
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
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.path = self.widget(QtGui.QLineEdit, "path")
        form.pathButton = self.widget(QtGui.QPushButton, "pathButton")
        form.sizeX = self.widget(QtGui.QDoubleSpinBox, "sizeX")
        form.sizeY = self.widget(QtGui.QDoubleSpinBox, "sizeY")
        form.dpi = self.widget(QtGui.QSpinBox, "dpi")
        plt = Plot.getPlot()
        form.path.setEnabled(bool(plt))
        form.pathButton.setEnabled(bool(plt))
        form.sizeX.setEnabled(bool(plt))
        form.sizeY.setEnabled(bool(plt))
        form.dpi.setEnabled(bool(plt))
        if not plt:
            return
        fig = plt.fig
        size = fig.get_size_inches()
        dpi = fig.get_dpi()
        form.sizeX.setValue(size[0])
        form.sizeY.setValue(size[1])
        form.dpi.setValue(dpi)

    def onPathButton(self):
        """Executed when the path selection button is pressed."""
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.path = self.widget(QtGui.QLineEdit, "path")
        path = form.path.text()
        file_choices = ("Portable Network Graphics (*.png)|*.png;;"
                        "Portable Document Format (*.pdf)|*.pdf;;"
                        "PostScript (*.ps)|*.ps;;"
                        "Encapsulated PostScript (*.eps)|*.eps")
        path = QtGui.QFileDialog.getSaveFileName(None,
                                                 'Save figure',
                                                 path,
                                                 file_choices)
        if path:
            form.path.setText(path)

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
