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

import six

import FreeCAD as App
import FreeCADGui as Gui

from PySide import QtGui, QtCore

import Plot
from plotUtils import Paths


class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/plotLabels/TaskPanel.ui"
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
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.axId = self.widget(QtGui.QSpinBox, "axesIndex")
        form.title = self.widget(QtGui.QLineEdit, "title")
        form.titleSize = self.widget(QtGui.QSpinBox, "titleSize")
        form.xLabel = self.widget(QtGui.QLineEdit, "titleX")
        form.xSize = self.widget(QtGui.QSpinBox, "xSize")
        form.yLabel = self.widget(QtGui.QLineEdit, "titleY")
        form.ySize = self.widget(QtGui.QSpinBox, "ySize")
        self.form = form
        self.retranslateUi()
        # Look for active axes if can
        axId = 0
        plt = Plot.getPlot()
        if plt:
            while plt.axes != plt.axesList[axId]:
                axId = axId + 1
            form.axId.setValue(axId)
        self.updateUI()
        QtCore.QObject.connect(form.axId,
                               QtCore.SIGNAL('valueChanged(int)'),
                               self.onAxesId)
        QtCore.QObject.connect(form.title,
                               QtCore.SIGNAL("editingFinished()"),
                               self.onLabels)
        QtCore.QObject.connect(form.xLabel,
                               QtCore.SIGNAL("editingFinished()"),
                               self.onLabels)
        QtCore.QObject.connect(form.yLabel,
                               QtCore.SIGNAL("editingFinished()"),
                               self.onLabels)
        QtCore.QObject.connect(form.titleSize,
                               QtCore.SIGNAL("valueChanged(int)"),
                               self.onFontSizes)
        QtCore.QObject.connect(form.xSize,
                               QtCore.SIGNAL("valueChanged(int)"),
                               self.onFontSizes)
        QtCore.QObject.connect(form.ySize,
                               QtCore.SIGNAL("valueChanged(int)"),
                               self.onFontSizes)
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
        """ Set the user interface locale strings.
        """
        self.form.setWindowTitle(QtGui.QApplication.translate(
            "plot_labels",
            "Set labels",
            None))
        self.widget(QtGui.QLabel, "axesLabel").setText(
            QtGui.QApplication.translate("plot_labels",
                                         "Active axes",
                                         None))
        self.widget(QtGui.QLabel, "titleLabel").setText(
            QtGui.QApplication.translate("plot_labels",
                                         "Title",
                                         None))
        self.widget(QtGui.QLabel, "xLabel").setText(
            QtGui.QApplication.translate("plot_labels",
                                         "X label",
                                         None))
        self.widget(QtGui.QLabel, "yLabel").setText(
            QtGui.QApplication.translate("plot_labels",
                                         "Y label",
                                         None))
        self.widget(QtGui.QSpinBox, "axesIndex").setToolTip(QtGui.QApplication.translate(
            "plot_labels",
            "Index of the active axes",
            None))
        self.widget(QtGui.QLineEdit, "title").setToolTip(
            QtGui.QApplication.translate(
                "plot_labels",
                "Title (associated to active axes)",
                None))
        self.widget(QtGui.QSpinBox, "titleSize").setToolTip(
            QtGui.QApplication.translate(
                "plot_labels",
                "Title font size",
                None))
        self.widget(QtGui.QLineEdit, "titleX").setToolTip(
            QtGui.QApplication.translate(
                "plot_labels",
                "X axis title",
                None))
        self.widget(QtGui.QSpinBox, "xSize").setToolTip(
            QtGui.QApplication.translate(
                "plot_labels",
                "X axis title font size",
                None))
        self.widget(QtGui.QLineEdit, "titleY").setToolTip(
            QtGui.QApplication.translate(
                "plot_labels",
                "Y axis title",
                None))
        self.widget(QtGui.QSpinBox, "ySize").setToolTip(
            QtGui.QApplication.translate(
                "plot_labels",
                "Y axis title font size",
                None))

    def onAxesId(self, value):
        """ Executed when axes index is modified. """
        if not self.skip:
            self.skip = True
            # No active plot case
            plt = Plot.getPlot()
            if not plt:
                self.updateUI()
                self.skip = False
                return
            # Get again all the subwidgets (to avoid PySide Pitfalls)
            mw = self.getMainWindow()
            form = mw.findChild(QtGui.QWidget, "TaskPanel")
            form.axId = self.widget(QtGui.QSpinBox, "axesIndex")

            form.axId.setMaximum(len(plt.axesList))
            if form.axId.value() >= len(plt.axesList):
                form.axId.setValue(len(plt.axesList) - 1)
            # Send new control to Plot instance
            plt.setActiveAxes(form.axId.value())
            self.updateUI()
            self.skip = False

    def onLabels(self):
        """ Executed when labels have been modified. """
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return
        # Get again all the subwidgets (to avoid PySide Pitfalls)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.title = self.widget(QtGui.QLineEdit, "title")
        form.xLabel = self.widget(QtGui.QLineEdit, "titleX")
        form.yLabel = self.widget(QtGui.QLineEdit, "titleY")

        Plot.title(six.text_type(form.title.text()))
        Plot.xlabel(six.text_type(form.xLabel.text()))
        Plot.ylabel(six.text_type(form.yLabel.text()))
        plt.update()

    def onFontSizes(self, value):
        """ Executed when font sizes have been modified. """
        # Get apply environment
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return
        # Get again all the subwidgets (to avoid PySide Pitfalls)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.titleSize = self.widget(QtGui.QSpinBox, "titleSize")
        form.xSize = self.widget(QtGui.QSpinBox, "xSize")
        form.ySize = self.widget(QtGui.QSpinBox, "ySize")

        ax = plt.axes
        ax.title.set_fontsize(form.titleSize.value())
        ax.xaxis.label.set_fontsize(form.xSize.value())
        ax.yaxis.label.set_fontsize(form.ySize.value())
        plt.update()

    def onMdiArea(self, subWin):
        """ Executed when window is selected on mdi area.

        Keyword arguments:
        subWin -- Selected window.
        """
        plt = Plot.getPlot()
        if plt != subWin:
            self.updateUI()

    def updateUI(self):
        """ Setup UI controls values if possible """
        # Get again all the subwidgets (to avoid PySide Pitfalls)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.axId = self.widget(QtGui.QSpinBox, "axesIndex")
        form.title = self.widget(QtGui.QLineEdit, "title")
        form.titleSize = self.widget(QtGui.QSpinBox, "titleSize")
        form.xLabel = self.widget(QtGui.QLineEdit, "titleX")
        form.xSize = self.widget(QtGui.QSpinBox, "xSize")
        form.yLabel = self.widget(QtGui.QLineEdit, "titleY")
        form.ySize = self.widget(QtGui.QSpinBox, "ySize")

        plt = Plot.getPlot()
        form.axId.setEnabled(bool(plt))
        form.title.setEnabled(bool(plt))
        form.titleSize.setEnabled(bool(plt))
        form.xLabel.setEnabled(bool(plt))
        form.xSize.setEnabled(bool(plt))
        form.yLabel.setEnabled(bool(plt))
        form.ySize.setEnabled(bool(plt))
        if not plt:
            return
        # Ensure that active axes is correct
        index = min(form.axId.value(), len(plt.axesList) - 1)
        form.axId.setValue(index)
        # Store data before starting changing it.

        ax = plt.axes
        t = ax.get_title()
        x = ax.get_xlabel()
        y = ax.get_ylabel()
        tt = ax.title.get_fontsize()
        xx = ax.xaxis.label.get_fontsize()
        yy = ax.yaxis.label.get_fontsize()
        # Set labels
        form.title.setText(t)
        form.xLabel.setText(x)
        form.yLabel.setText(y)
        # Set font sizes
        form.titleSize.setValue(tt)
        form.xSize.setValue(xx)
        form.ySize.setValue(yy)


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
