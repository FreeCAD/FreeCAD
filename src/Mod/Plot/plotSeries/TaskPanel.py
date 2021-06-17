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

import matplotlib
from matplotlib.lines import Line2D
import matplotlib.colors as Colors


# The module is used to prevent complaints from code checkers (flake8)
bool(Plot_rc.__name__)


class TaskPanel:
    def __init__(self):
        self.name = "plot series editor"
        self.ui = ":/ui/TaskPanel_plotSeries.ui"
        self.form = Gui.PySideUic.loadUi(self.ui)
        self.skip = False
        self.item = 0
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
        self.form.label = self.widget(QtGui.QLineEdit, "label")
        self.form.isLabel = self.widget(QtGui.QCheckBox, "isLabel")
        self.form.style = self.widget(QtGui.QComboBox, "lineStyle")
        self.form.marker = self.widget(QtGui.QComboBox, "markers")
        self.form.width = self.widget(QtGui.QDoubleSpinBox, "lineWidth")
        self.form.size = self.widget(QtGui.QSpinBox, "markerSize")
        self.form.color = self.widget(QtGui.QPushButton, "color")
        self.form.remove = self.widget(QtGui.QPushButton, "remove")
        self.retranslateUi()
        self.fillStyles()
        self.updateUI()
        QtCore.QObject.connect(
            self.form.items,
            QtCore.SIGNAL("currentRowChanged(int)"),
            self.onItem)
        QtCore.QObject.connect(
            self.form.label,
            QtCore.SIGNAL("editingFinished()"),
            self.onData)
        QtCore.QObject.connect(
            self.form.isLabel,
            QtCore.SIGNAL("stateChanged(int)"),
            self.onData)
        QtCore.QObject.connect(
            self.form.style,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.onData)
        QtCore.QObject.connect(
            self.form.marker,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.onData)
        QtCore.QObject.connect(
            self.form.width,
            QtCore.SIGNAL("valueChanged(double)"),
            self.onData)
        QtCore.QObject.connect(
            self.form.size,
            QtCore.SIGNAL("valueChanged(int)"),
            self.onData)
        QtCore.QObject.connect(
            self.form.color,
            QtCore.SIGNAL("pressed()"),
            self.onColor)
        QtCore.QObject.connect(
            self.form.remove,
            QtCore.SIGNAL("pressed()"),
            self.onRemove)
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
            "plot_series",
            "Configure series",
            None))
        self.widget(QtGui.QCheckBox, "isLabel").setText(
            QtGui.QApplication.translate(
                "plot_series",
                "No label",
                None))
        self.widget(QtGui.QPushButton, "remove").setText(
            QtGui.QApplication.translate(
                "plot_series",
                "Remove series",
                None))
        self.widget(QtGui.QLabel, "styleLabel").setText(
            QtGui.QApplication.translate(
                "plot_series",
                "Line style",
                None))
        self.widget(QtGui.QLabel, "markerLabel").setText(
            QtGui.QApplication.translate(
                "plot_series",
                "Marker",
                None))
        self.widget(QtGui.QListWidget, "items").setToolTip(
            QtGui.QApplication.translate(
                "plot_series",
                "List of available series",
                None))
        self.widget(QtGui.QLineEdit, "label").setToolTip(
            QtGui.QApplication.translate(
                "plot_series",
                "Line title",
                None))
        self.widget(QtGui.QCheckBox, "isLabel").setToolTip(
            QtGui.QApplication.translate(
                "plot_series",
                "If checked, series will not be considered for legend",
                None))
        self.widget(QtGui.QComboBox, "lineStyle").setToolTip(
            QtGui.QApplication.translate(
                "plot_series",
                "Line style",
                None))
        self.widget(QtGui.QComboBox, "markers").setToolTip(
            QtGui.QApplication.translate(
                "plot_series",
                "Marker style",
                None))
        self.widget(QtGui.QDoubleSpinBox, "lineWidth").setToolTip(
            QtGui.QApplication.translate(
                "plot_series",
                "Line width",
                None))
        self.widget(QtGui.QSpinBox, "markerSize").setToolTip(
            QtGui.QApplication.translate(
                "plot_series",
                "Marker size",
                None))
        self.widget(QtGui.QPushButton, "color").setToolTip(
            QtGui.QApplication.translate(
                "plot_series",
                "Line and marker color",
                None))
        self.widget(QtGui.QPushButton, "remove").setToolTip(
            QtGui.QApplication.translate(
                "plot_series",
                "Removes this series",
                None))

    def fillStyles(self):
        """Fill the style combo boxes with the available ones."""
        # Line styles
        for style in Line2D.lineStyles.keys():
            string = "\'" + str(style) + "\'"
            string += " (" + Line2D.lineStyles[style] + ")"
            self.form.style.addItem(string)
        # Markers
        for marker in Line2D.markers.keys():
            string = "\'" + str(marker) + "\'"
            string += " (" + Line2D.markers[marker] + ")"
            self.form.marker.addItem(string)

    def onItem(self, row):
        """Executed when the selected item is modified."""
        if not self.skip:
            self.skip = True

            self.item = row

            self.updateUI()
            self.skip = False

    def onData(self):
        """Executed when the selected item data is modified."""
        if not self.skip:
            self.skip = True
            plt = Plot.getPlot()
            if not plt:
                self.updateUI()
                return
            # Ensure that selected serie exist
            if self.item >= len(Plot.series()):
                self.updateUI()
                return
            # Set label
            serie = Plot.series()[self.item]
            if(self.form.isLabel.isChecked()):
                serie.name = None
                self.form.label.setEnabled(False)
            else:
                serie.name = self.form.label.text()
                self.form.label.setEnabled(True)
            # Set line style and marker
            style = self.form.style.currentIndex()
            linestyles = list(Line2D.lineStyles.keys())
            serie.line.set_linestyle(linestyles[style])
            marker = self.form.marker.currentIndex()
            markers = list(Line2D.markers.keys())
            serie.line.set_marker(markers[marker])
            # Set line width and marker size
            serie.line.set_linewidth(self.form.width.value())
            serie.line.set_markersize(self.form.size.value())
            plt.update()
            # Regenerate series labels
            self.setList()
            self.skip = False

    def onColor(self):
        """ Executed when color palette is requested. """
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return

        # Ensure that selected serie exist
        if self.item >= len(Plot.series()):
            self.updateUI()
            return
        # Show widget to select color
        col = QtGui.QColorDialog.getColor()
        # Send color to widget and serie
        if col.isValid():
            serie = plt.series[self.item]
            self.form.color.setStyleSheet(
                "background-color: rgb({}, {}, {});".format(col.red(),
                                                            col.green(),
                                                            col.blue()))
            serie.line.set_color((col.redF(), col.greenF(), col.blueF()))
            plt.update()

    def onRemove(self):
        """Executed when the data serie must be removed."""
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return
        # Ensure that selected serie exist
        if self.item >= len(Plot.series()):
            self.updateUI()
            return
        # Remove serie
        Plot.removeSerie(self.item)
        self.setList()
        self.updateUI()
        plt.update()

    def onMdiArea(self, subWin):
        """Executed when a new window is selected on the mdi area.

        Keyword arguments:
        subWin -- Selected window.
        """
        plt = Plot.getPlot()
        if plt != subWin:
            self.updateUI()

    def updateUI(self):
        """ Setup UI controls values if possible """
        plt = Plot.getPlot()
        self.form.items.setEnabled(bool(plt))
        self.form.label.setEnabled(bool(plt))
        self.form.isLabel.setEnabled(bool(plt))
        self.form.style.setEnabled(bool(plt))
        self.form.marker.setEnabled(bool(plt))
        self.form.width.setEnabled(bool(plt))
        self.form.size.setEnabled(bool(plt))
        self.form.color.setEnabled(bool(plt))
        self.form.remove.setEnabled(bool(plt))
        if not plt:
            self.plt = plt
            self.form.items.clear()
            return
        self.skip = True
        # Refill list
        if self.plt != plt or len(Plot.series()) != self.form.items.count():
            self.plt = plt
            self.setList()
        # Ensure that have series
        if not len(Plot.series()):
            self.form.label.setEnabled(False)
            self.form.isLabel.setEnabled(False)
            self.form.style.setEnabled(False)
            self.form.marker.setEnabled(False)
            self.form.width.setEnabled(False)
            self.form.size.setEnabled(False)
            self.form.color.setEnabled(False)
            self.form.remove.setEnabled(False)
            return
        # Set label
        serie = Plot.series()[self.item]
        if serie.name is None:
            self.form.isLabel.setChecked(True)
            self.form.label.setEnabled(False)
            self.form.label.setText("")
        else:
            self.form.isLabel.setChecked(False)
            self.form.label.setText(serie.name)
        # Set line style and marker
        self.form.style.setCurrentIndex(0)
        for i, style in enumerate(Line2D.lineStyles.keys()):
            if style == serie.line.get_linestyle():
                self.form.style.setCurrentIndex(i)
        self.form.marker.setCurrentIndex(0)
        for i, marker in enumerate(Line2D.markers.keys()):
            if marker == serie.line.get_marker():
                self.form.marker.setCurrentIndex(i)
        # Set line width and marker size
        self.form.width.setValue(serie.line.get_linewidth())
        self.form.size.setValue(serie.line.get_markersize())
        # Set color
        color = Colors.colorConverter.to_rgb(serie.line.get_color())
        self.form.color.setStyleSheet("background-color: rgb({}, {}, {});".format(
            int(color[0] * 255),
            int(color[1] * 255),
            int(color[2] * 255)))
        self.skip = False

    def setList(self):
        """Setup the UI control values if it is possible."""
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        self.form.items = self.widget(QtGui.QListWidget, "items")
        self.form.items.clear()
        series = Plot.series()
        for i in range(0, len(series)):
            serie = series[i]
            string = 'serie ' + str(i) + ': '
            if serie.name is None:
                string = string + '\"No label\"'
            else:
                string = string + serie.name
            self.form.items.addItem(string)
        # Ensure that selected item is correct
        if len(series) and self.item >= len(series):
            self.item = len(series) - 1
            self.form.items.setCurrentIndex(self.item)


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
