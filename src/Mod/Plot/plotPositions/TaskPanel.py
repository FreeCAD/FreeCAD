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

from PySide import QtGui, QtCore

import Plot
from plotUtils import Paths


class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/plotPositions/TaskPanel.ui"
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
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.items = self.widget(QtGui.QListWidget, "items")
        form.x = self.widget(QtGui.QDoubleSpinBox, "x")
        form.y = self.widget(QtGui.QDoubleSpinBox, "y")
        form.s = self.widget(QtGui.QDoubleSpinBox, "size")
        self.form = form
        self.retranslateUi()
        self.updateUI()
        QtCore.QObject.connect(
            form.items,
            QtCore.SIGNAL("currentRowChanged(int)"),
            self.onItem)
        QtCore.QObject.connect(
            form.x,
            QtCore.SIGNAL("valueChanged(double)"),
            self.onData)
        QtCore.QObject.connect(
            form.y,
            QtCore.SIGNAL("valueChanged(double)"),
            self.onData)
        QtCore.QObject.connect(
            form.s,
            QtCore.SIGNAL("valueChanged(double)"),
            self.onData)
        QtCore.QObject.connect(
            Plot.getMdiArea(),
            QtCore.SIGNAL("subWindowActivated(QMdiSubWindow*)"),
            self.onMdiArea)
        return False

    def getMainWindow(self):
        toplevel = QtGui.qApp.topLevelWidgets()
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
            None,
            QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QLabel, "posLabel").setText(
            QtGui.QApplication.translate(
                "plot_positions",
                "Position",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QLabel, "sizeLabel").setText(
            QtGui.QApplication.translate(
                "plot_positions",
                "Size",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QListWidget, "items").setToolTip(
            QtGui.QApplication.translate(
                "plot_positions",
                "List of modificable items",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QDoubleSpinBox, "x").setToolTip(
            QtGui.QApplication.translate(
                "plot_positions",
                "X item position",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QDoubleSpinBox, "y").setToolTip(
            QtGui.QApplication.translate(
                "plot_positions",
                "Y item position",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QDoubleSpinBox, "size").setToolTip(
            QtGui.QApplication.translate(
                "plot_positions",
                "Item size",
                None,
                QtGui.QApplication.UnicodeUTF8))

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
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.items = self.widget(QtGui.QListWidget, "items")
        form.x = self.widget(QtGui.QDoubleSpinBox, "x")
        form.y = self.widget(QtGui.QDoubleSpinBox, "y")
        form.s = self.widget(QtGui.QDoubleSpinBox, "size")
        if not self.skip:
            self.skip = True
            name = self.names[self.item]
            obj = self.objs[self.item]
            x = form.x.value()
            y = form.y.value()
            s = form.s.value()
            # x/y labels only have one position control
            if name.find('x label') >= 0:
                form.y.setValue(x)
            elif name.find('y label') >= 0:
                form.x.setValue(y)
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
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.items = self.widget(QtGui.QListWidget, "items")
        form.x = self.widget(QtGui.QDoubleSpinBox, "x")
        form.y = self.widget(QtGui.QDoubleSpinBox, "y")
        form.s = self.widget(QtGui.QDoubleSpinBox, "size")
        plt = Plot.getPlot()
        form.items.setEnabled(bool(plt))
        form.x.setEnabled(bool(plt))
        form.y.setEnabled(bool(plt))
        form.s.setEnabled(bool(plt))
        if not plt:
            self.plt = plt
            form.items.clear()
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
                form.y.setEnabled(False)
                form.y.setValue(x)
            elif name.find('y label') >= 0:
                form.x.setEnabled(False)
                form.x.setValue(y)
        else:
            x = plt.legPos[0]
            y = plt.legPos[1]
            s = obj.get_texts()[-1].get_fontsize()
        # Send it to controls
        form.x.setValue(x)
        form.y.setValue(y)
        form.s.setValue(s)

    def setList(self):
        """ Setup UI controls values if possible """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.items = self.widget(QtGui.QListWidget, "items")
        form.x = self.widget(QtGui.QDoubleSpinBox, "x")
        form.y = self.widget(QtGui.QDoubleSpinBox, "y")
        form.s = self.widget(QtGui.QDoubleSpinBox, "size")
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
        form.items.clear()
        for name in self.names:
            form.items.addItem(name)
        # Ensure that selected item is correct
        if self.item >= len(self.names):
            self.item = len(self.names) - 1
            form.items.setCurrentIndex(self.item)


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
