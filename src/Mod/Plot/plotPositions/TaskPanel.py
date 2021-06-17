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
        self.name = "plot positions"
        self.ui = ":/ui/TaskPanel_plotPositions.ui"
        self.form = Gui.PySideUic.loadUi(self.ui)
        self.skip = False
        self.item = 0
        self.names = []
        self.objs = []
        self.plt = None

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
        self.form.items = self.widget(QtGui.QListWidget, "items")
        self.form.x = self.widget(QtGui.QDoubleSpinBox, "x")
        self.form.y = self.widget(QtGui.QDoubleSpinBox, "y")
        self.form.s = self.widget(QtGui.QDoubleSpinBox, "size")
        self.retranslateUi()
        self.updateUI()
        QtCore.QObject.connect(
            self.form.items,
            QtCore.SIGNAL("currentRowChanged(int)"),
            self.onItem)
        QtCore.QObject.connect(
            self.form.x,
            QtCore.SIGNAL("valueChanged(double)"),
            self.onData)
        QtCore.QObject.connect(
            self.form.y,
            QtCore.SIGNAL("valueChanged(double)"),
            self.onData)
        QtCore.QObject.connect(
            self.form.s,
            QtCore.SIGNAL("valueChanged(double)"),
            self.onData)
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
            "plot_positions",
            "Set positions and sizes",
            None))
        self.widget(QtGui.QLabel, "posLabel").setText(
            QtGui.QApplication.translate(
                "plot_positions",
                "Position",
                None))
        self.widget(QtGui.QLabel, "sizeLabel").setText(
            QtGui.QApplication.translate(
                "plot_positions",
                "Size",
                None))
        self.widget(QtGui.QListWidget, "items").setToolTip(
            QtGui.QApplication.translate(
                "plot_positions",
                "List of modifiable items",
                None))
        self.widget(QtGui.QDoubleSpinBox, "x").setToolTip(
            QtGui.QApplication.translate(
                "plot_positions",
                "X item position",
                None))
        self.widget(QtGui.QDoubleSpinBox, "y").setToolTip(
            QtGui.QApplication.translate(
                "plot_positions",
                "Y item position",
                None))
        self.widget(QtGui.QDoubleSpinBox, "size").setToolTip(
            QtGui.QApplication.translate(
                "plot_positions",
                "Item size",
                None))

    def onItem(self, row):
        """ Executed when selected item is modified. """
        self.item = row
        self.updateUI()

    def onData(self, value):
        """ Executed when selected item data is modified. """
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return
        if not self.skip:
            self.skip = True
            name = self.names[self.item]
            obj = self.objs[self.item]
            x = self.form.x.value()
            y = self.form.y.value()
            s = self.form.s.value()
            # x/y labels only have one position control
            if name.find('x label') >= 0:
                self.form.y.setValue(x)
            elif name.find('y label') >= 0:
                self.form.x.setValue(y)
            # title and labels only have one size control
            if name.find('title') >= 0 or name.find('label') >= 0:
                obj.set_position((x, y))
                obj.set_size(s)
            # legend have all controls
            else:
                Plot.legend(plt.legend, (x, y), s)
            plt.update()
            self.skip = False

    def onMdiArea(self, subWin):
        """Executed when a new window is selected on the mdi area.

        Keyword arguments:
        subWin -- Selected window.
        """
        plt = Plot.getPlot()
        if plt != subWin:
            self.updateUI()

    def updateUI(self):
        """Setup the UI control values if it is possible."""
        plt = Plot.getPlot()
        self.form.items.setEnabled(bool(plt))
        self.form.x.setEnabled(bool(plt))
        self.form.y.setEnabled(bool(plt))
        self.form.s.setEnabled(bool(plt))
        if not plt:
            self.plt = plt
            self.form.items.clear()
            return
        # Refill items list only if Plot instance have been changed
        if self.plt != plt:
            self.plt = plt
            self.plt.update()
            self.setList()
        # Get data for controls
        name = self.names[self.item]
        obj = self.objs[self.item]
        if name.find('title') >= 0 or name.find('label') >= 0:
            p = obj.get_position()
            x = p[0]
            y = p[1]
            s = obj.get_size()
            if name.find('x label') >= 0:
                self.form.y.setEnabled(False)
                self.form.y.setValue(x)
            elif name.find('y label') >= 0:
                self.form.x.setEnabled(False)
                self.form.x.setValue(y)
        else:
            x = plt.legPos[0]
            y = plt.legPos[1]
            s = obj.get_texts()[-1].get_fontsize()
        # Send it to controls
        self.form.x.setValue(x)
        self.form.y.setValue(y)
        self.form.s.setValue(s)

    def setList(self):
        """ Setup UI controls values if possible """
        # Clear lists
        self.names = []
        self.objs = []
        # Fill lists with available objects
        if self.plt:
            # Axes data
            for i in range(0, len(self.plt.axesList)):
                ax = self.plt.axesList[i]
                # Each axes have title, xaxis and yaxis
                self.names.append('title (axes {})'.format(i))
                self.objs.append(ax.title)
                self.names.append('x label (axes {})'.format(i))
                self.objs.append(ax.xaxis.get_label())
                self.names.append('y label (axes {})'.format(i))
                self.objs.append(ax.yaxis.get_label())
            # Legend if exist
            ax = self.plt.axesList[-1]
            if ax.legend_:
                self.names.append('legend')
                self.objs.append(ax.legend_)
        # Send list to widget
        self.form.items.clear()
        for name in self.names:
            self.form.items.addItem(name)
        # Ensure that selected item is correct
        if self.item >= len(self.names):
            self.item = len(self.names) - 1
            self.form.items.setCurrentIndex(self.item)


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
