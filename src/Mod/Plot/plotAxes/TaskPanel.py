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
        self.ui = Paths.modulePath() + "/plotAxes/TaskPanel.ui"
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
        self.form = form
        form.axId = self.widget(QtGui.QSpinBox, "axesIndex")
        form.new = self.widget(QtGui.QPushButton, "newAxesButton")
        form.remove = self.widget(QtGui.QPushButton, "delAxesButton")
        form.all = self.widget(QtGui.QCheckBox, "allAxes")
        form.xMin = self.widget(QtGui.QSlider, "posXMin")
        form.xMax = self.widget(QtGui.QSlider, "posXMax")
        form.yMin = self.widget(QtGui.QSlider, "posYMin")
        form.yMax = self.widget(QtGui.QSlider, "posYMax")
        form.xAlign = self.widget(QtGui.QComboBox, "xAlign")
        form.yAlign = self.widget(QtGui.QComboBox, "yAlign")
        form.xOffset = self.widget(QtGui.QSpinBox, "xOffset")
        form.yOffset = self.widget(QtGui.QSpinBox, "yOffset")
        form.xAuto = self.widget(QtGui.QCheckBox, "xAuto")
        form.yAuto = self.widget(QtGui.QCheckBox, "yAuto")
        form.xSMin = self.widget(QtGui.QLineEdit, "xMin")
        form.xSMax = self.widget(QtGui.QLineEdit, "xMax")
        form.ySMin = self.widget(QtGui.QLineEdit, "yMin")
        form.ySMax = self.widget(QtGui.QLineEdit, "yMax")
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
        QtCore.QObject.connect(form.new,
                               QtCore.SIGNAL("pressed()"),
                               self.onNew)
        QtCore.QObject.connect(form.remove,
                               QtCore.SIGNAL("pressed()"),
                               self.onRemove)
        QtCore.QObject.connect(form.xMin,
                               QtCore.SIGNAL("valueChanged(int)"),
                               self.onDims)
        QtCore.QObject.connect(form.xMax,
                               QtCore.SIGNAL("valueChanged(int)"),
                               self.onDims)
        QtCore.QObject.connect(form.yMin,
                               QtCore.SIGNAL("valueChanged(int)"),
                               self.onDims)
        QtCore.QObject.connect(form.yMax,
                               QtCore.SIGNAL("valueChanged(int)"),
                               self.onDims)
        QtCore.QObject.connect(form.xAlign,
                               QtCore.SIGNAL("currentIndexChanged(int)"),
                               self.onAlign)
        QtCore.QObject.connect(form.yAlign,
                               QtCore.SIGNAL("currentIndexChanged(int)"),
                               self.onAlign)
        QtCore.QObject.connect(form.xOffset,
                               QtCore.SIGNAL("valueChanged(int)"),
                               self.onOffset)
        QtCore.QObject.connect(form.yOffset,
                               QtCore.SIGNAL("valueChanged(int)"),
                               self.onOffset)
        QtCore.QObject.connect(form.xAuto,
                               QtCore.SIGNAL("stateChanged(int)"),
                               self.onScales)
        QtCore.QObject.connect(form.yAuto,
                               QtCore.SIGNAL("stateChanged(int)"),
                               self.onScales)
        QtCore.QObject.connect(form.xSMin,
                               QtCore.SIGNAL("editingFinished()"),
                               self.onScales)
        QtCore.QObject.connect(form.xSMax,
                               QtCore.SIGNAL("editingFinished()"),
                               self.onScales)
        QtCore.QObject.connect(form.ySMin,
                               QtCore.SIGNAL("editingFinished()"),
                               self.onScales)
        QtCore.QObject.connect(form.ySMax,
                               QtCore.SIGNAL("editingFinished()"),
                               self.onScales)
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
        """Set the user interface locale strings.
        """
        form = self.form
        form.setWindowTitle(QtGui.QApplication.translate(
            "plot_axes",
            "Configure axes",
            None))
        self.widget(QtGui.QLabel, "axesLabel").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "Active axes",
                                         None))
        self.widget(QtGui.QCheckBox, "allAxes").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "Apply to all axes",
                                         None))
        self.widget(QtGui.QLabel, "dimLabel").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "Dimensions",
                                         None))
        self.widget(QtGui.QLabel, "xPosLabel").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "X axis position",
                                         None))
        self.widget(QtGui.QLabel, "yPosLabel").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "Y axis position",
                                         None))
        self.widget(QtGui.QLabel, "scalesLabel").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "Scales",
                                         None))
        self.widget(QtGui.QCheckBox, "xAuto").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "X auto",
                                         None))
        self.widget(QtGui.QCheckBox, "yAuto").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "Y auto",
                                         None))
        self.widget(QtGui.QCheckBox, "allAxes").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "Apply to all axes",
                                         None))
        self.widget(QtGui.QLabel, "dimLabel").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "Dimensions",
                                         None))
        self.widget(QtGui.QLabel, "xPosLabel").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "X axis position",
                                         None))
        self.widget(QtGui.QLabel, "yPosLabel").setText(
            QtGui.QApplication.translate("plot_axes",
                                         "Y axis position",
                                         None))
        self.widget(QtGui.QSpinBox, "axesIndex").setToolTip(
            QtGui.QApplication.translate("plot_axes",
                                         "Index of the active axes",
                                         None))
        self.widget(QtGui.QPushButton, "newAxesButton").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "Add new axes to the plot",
                None))
        self.widget(QtGui.QPushButton, "delAxesButton").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "Remove selected axes",
                None))
        self.widget(QtGui.QCheckBox, "allAxes").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "Check it to apply transformations to all axes",
                None))
        self.widget(QtGui.QSlider, "posXMin").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "Left bound of axes",
                None))
        self.widget(QtGui.QSlider, "posXMax").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "Right bound of axes",
                None))
        self.widget(QtGui.QSlider, "posYMin").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "Bottom bound of axes",
                None))
        self.widget(QtGui.QSlider, "posYMax").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "Top bound of axes",
                None))
        self.widget(QtGui.QSpinBox, "xOffset").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "Outward offset of X axis",
                None))
        self.widget(QtGui.QSpinBox, "yOffset").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "Outward offset of Y axis",
                None))
        self.widget(QtGui.QCheckBox, "xAuto").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "X axis scale autoselection",
                None))
        self.widget(QtGui.QCheckBox, "yAuto").setToolTip(
            QtGui.QApplication.translate(
                "plot_axes",
                "Y axis scale autoselection",
                None))

    def onAxesId(self, value):
        """Executed when axes index is modified."""
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

    def onNew(self):
        """Executed when new axes must be created."""
        # Ensure that we can work
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return
        # Get again all the subwidgets (to avoid PySide Pitfalls)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.axId = self.widget(QtGui.QSpinBox, "axesIndex")

        Plot.addNewAxes()
        form.axId.setValue(len(plt.axesList) - 1)
        plt.update()

    def onRemove(self):
        """Executed when axes must be deleted."""
        # Ensure that we can work
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return
        # Get again all the subwidgets (to avoid PySide Pitfalls)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.axId = self.widget(QtGui.QSpinBox, "axesIndex")

        # Don't remove first axes
        if not form.axId.value():
            msg = QtGui.QApplication.translate(
                "plot_console",
                "Axes 0 can not be deleted",
                None)
            App.Console.PrintError(msg + "\n")
            return
        # Remove axes
        ax = plt.axes
        ax.set_axis_off()
        plt.axesList.pop(form.axId.value())
        # Ensure that active axes is correct
        index = min(form.axId.value(), len(plt.axesList) - 1)
        form.axId.setValue(index)
        plt.update()

    def onDims(self, value):
        """Executed when axes dims have been modified."""
        # Ensure that we can work
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return
        # Get again all the subwidgets (to avoid PySide Pitfalls)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.all = self.widget(QtGui.QCheckBox, "allAxes")
        form.xMin = self.widget(QtGui.QSlider, "posXMin")
        form.xMax = self.widget(QtGui.QSlider, "posXMax")
        form.yMin = self.widget(QtGui.QSlider, "posYMin")
        form.yMax = self.widget(QtGui.QSlider, "posYMax")

        axesList = [plt.axes]
        if form.all.isChecked():
            axesList = plt.axesList
        # Set new dimensions
        xmin = form.xMin.value() / 100.0
        xmax = form.xMax.value() / 100.0
        ymin = form.yMin.value() / 100.0
        ymax = form.yMax.value() / 100.0
        for axes in axesList:
            axes.set_position([xmin, ymin, xmax - xmin, ymax - ymin])
        plt.update()

    def onAlign(self, value):
        """Executed when axes align have been modified."""
        # Ensure that we can work
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return
        # Get again all the subwidgets (to avoid PySide Pitfalls)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.all = self.widget(QtGui.QCheckBox, "allAxes")
        form.xAlign = self.widget(QtGui.QComboBox, "xAlign")
        form.yAlign = self.widget(QtGui.QComboBox, "yAlign")

        axesList = [plt.axes]
        if form.all.isChecked():
            axesList = plt.axesList
        # Set new alignment
        for axes in axesList:
            if form.xAlign.currentIndex() == 0:
                axes.xaxis.tick_bottom()
                axes.spines['bottom'].set_color((0.0, 0.0, 0.0))
                axes.spines['top'].set_color('none')
                axes.xaxis.set_ticks_position('bottom')
                axes.xaxis.set_label_position('bottom')
            else:
                axes.xaxis.tick_top()
                axes.spines['top'].set_color((0.0, 0.0, 0.0))
                axes.spines['bottom'].set_color('none')
                axes.xaxis.set_ticks_position('top')
                axes.xaxis.set_label_position('top')
            if form.yAlign.currentIndex() == 0:
                axes.yaxis.tick_left()
                axes.spines['left'].set_color((0.0, 0.0, 0.0))
                axes.spines['right'].set_color('none')
                axes.yaxis.set_ticks_position('left')
                axes.yaxis.set_label_position('left')
            else:
                axes.yaxis.tick_right()
                axes.spines['right'].set_color((0.0, 0.0, 0.0))
                axes.spines['left'].set_color('none')
                axes.yaxis.set_ticks_position('right')
                axes.yaxis.set_label_position('right')
        plt.update()

    def onOffset(self, value):
        """Executed when axes offsets have been modified."""
        # Ensure that we can work
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return
        # Get again all the subwidgets (to avoid PySide Pitfalls)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.all = self.widget(QtGui.QCheckBox, "allAxes")
        form.xOffset = self.widget(QtGui.QSpinBox, "xOffset")
        form.yOffset = self.widget(QtGui.QSpinBox, "yOffset")

        axesList = [plt.axes]
        if form.all.isChecked():
            axesList = plt.axesList
        # Set new offset
        for axes in axesList:
            # For some reason, modify spines offset erase axes labels, so we
            # need store it in order to regenerate later
            x = axes.get_xlabel()
            y = axes.get_ylabel()
            for loc, spine in axes.spines.items():
                if loc in ['bottom', 'top']:
                    spine.set_position(('outward', form.xOffset.value()))
                if loc in ['left', 'right']:
                    spine.set_position(('outward', form.yOffset.value()))
            # Now we can restore axes labels
            Plot.xlabel(unicode(x))
            Plot.ylabel(unicode(y))
        plt.update()

    def onScales(self):
        """Executed when axes scales have been modified."""
        # Ensure that we can work
        plt = Plot.getPlot()
        if not plt:
            self.updateUI()
            return
        # Get again all the subwidgets (to avoid PySide Pitfalls)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.all = self.widget(QtGui.QCheckBox, "allAxes")
        form.xAuto = self.widget(QtGui.QCheckBox, "xAuto")
        form.yAuto = self.widget(QtGui.QCheckBox, "yAuto")
        form.xSMin = self.widget(QtGui.QLineEdit, "xMin")
        form.xSMax = self.widget(QtGui.QLineEdit, "xMax")
        form.ySMin = self.widget(QtGui.QLineEdit, "yMin")
        form.ySMax = self.widget(QtGui.QLineEdit, "yMax")

        axesList = [plt.axes]
        if form.all.isChecked():
            axesList = plt.axesList
        if not self.skip:
            self.skip = True
            # X axis
            if form.xAuto.isChecked():
                for ax in axesList:
                    ax.set_autoscalex_on(True)
                form.xSMin.setEnabled(False)
                form.xSMax.setEnabled(False)
                lim = plt.axes.get_xlim()
                form.xSMin.setText(str(lim[0]))
                form.xSMax.setText(str(lim[1]))
            else:
                form.xSMin.setEnabled(True)
                form.xSMax.setEnabled(True)
                try:
                    xMin = float(form.xSMin.text())
                except:
                    xMin = plt.axes.get_xlim()[0]
                    form.xSMin.setText(str(xMin))
                try:
                    xMax = float(form.xSMax.text())
                except:
                    xMax = plt.axes.get_xlim()[1]
                    form.xSMax.setText(str(xMax))
                for ax in axesList:
                    ax.set_xlim((xMin, xMax))
            # Y axis
            if form.yAuto.isChecked():
                for ax in axesList:
                    ax.set_autoscaley_on(True)
                form.ySMin.setEnabled(False)
                form.ySMax.setEnabled(False)
                lim = plt.axes.get_ylim()
                form.ySMin.setText(str(lim[0]))
                form.ySMax.setText(str(lim[1]))
            else:
                form.ySMin.setEnabled(True)
                form.ySMax.setEnabled(True)
                try:
                    yMin = float(form.ySMin.text())
                except:
                    yMin = plt.axes.get_ylim()[0]
                    form.ySMin.setText(str(yMin))
                try:
                    yMax = float(form.ySMax.text())
                except:
                    yMax = plt.axes.get_ylim()[1]
                    form.ySMax.setText(str(yMax))
                for ax in axesList:
                    ax.set_ylim((yMin, yMax))
            plt.update()
            self.skip = False

    def onMdiArea(self, subWin):
        """Executed when window is selected on mdi area.

        Keyword arguments:
        subWin -- Selected window.
        """
        plt = Plot.getPlot()
        if plt != subWin:
            self.updateUI()

    def updateUI(self):
        """Setup UI controls values if possible"""
        plt = Plot.getPlot()
        # Get again all the subwidgets (to avoid PySide Pitfalls)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.axId = self.widget(QtGui.QSpinBox, "axesIndex")
        form.new = self.widget(QtGui.QPushButton, "newAxesButton")
        form.remove = self.widget(QtGui.QPushButton, "delAxesButton")
        form.all = self.widget(QtGui.QCheckBox, "allAxes")
        form.xMin = self.widget(QtGui.QSlider, "posXMin")
        form.xMax = self.widget(QtGui.QSlider, "posXMax")
        form.yMin = self.widget(QtGui.QSlider, "posYMin")
        form.yMax = self.widget(QtGui.QSlider, "posYMax")
        form.xAlign = self.widget(QtGui.QComboBox, "xAlign")
        form.yAlign = self.widget(QtGui.QComboBox, "yAlign")
        form.xOffset = self.widget(QtGui.QSpinBox, "xOffset")
        form.yOffset = self.widget(QtGui.QSpinBox, "yOffset")
        form.xAuto = self.widget(QtGui.QCheckBox, "xAuto")
        form.yAuto = self.widget(QtGui.QCheckBox, "yAuto")
        form.xSMin = self.widget(QtGui.QLineEdit, "xMin")
        form.xSMax = self.widget(QtGui.QLineEdit, "xMax")
        form.ySMin = self.widget(QtGui.QLineEdit, "yMin")
        form.ySMax = self.widget(QtGui.QLineEdit, "yMax")
        # Enable/disable them
        form.axId.setEnabled(bool(plt))
        form.new.setEnabled(bool(plt))
        form.remove.setEnabled(bool(plt))
        form.all.setEnabled(bool(plt))
        form.xMin.setEnabled(bool(plt))
        form.xMax.setEnabled(bool(plt))
        form.yMin.setEnabled(bool(plt))
        form.yMax.setEnabled(bool(plt))
        form.xAlign.setEnabled(bool(plt))
        form.yAlign.setEnabled(bool(plt))
        form.xOffset.setEnabled(bool(plt))
        form.yOffset.setEnabled(bool(plt))
        form.xAuto.setEnabled(bool(plt))
        form.yAuto.setEnabled(bool(plt))
        form.xSMin.setEnabled(bool(plt))
        form.xSMax.setEnabled(bool(plt))
        form.ySMin.setEnabled(bool(plt))
        form.ySMax.setEnabled(bool(plt))
        if not plt:
            form.axId.setValue(0)
            return
        # Ensure that active axes is correct
        index = min(form.axId.value(), len(plt.axesList) - 1)
        form.axId.setValue(index)
        # Set dimensions
        ax = plt.axes
        bb = ax.get_position()
        form.xMin.setValue(int(100 * bb._get_xmin()))
        form.xMax.setValue(int(100 * bb._get_xmax()))
        form.yMin.setValue(int(100 * bb._get_ymin()))
        form.yMax.setValue(int(100 * bb._get_ymax()))
        # Set alignment and offset
        xPos = ax.xaxis.get_ticks_position()
        yPos = ax.yaxis.get_ticks_position()
        xOffset = ax.spines['bottom'].get_position()[1]
        yOffset = ax.spines['left'].get_position()[1]
        if xPos == 'bottom' or xPos == 'default':
            form.xAlign.setCurrentIndex(0)
        else:
            form.xAlign.setCurrentIndex(1)
        form.xOffset.setValue(xOffset)
        if yPos == 'left' or yPos == 'default':
            form.yAlign.setCurrentIndex(0)
        else:
            form.yAlign.setCurrentIndex(1)
        form.yOffset.setValue(yOffset)
        # Set scales
        if ax.get_autoscalex_on():
            form.xAuto.setChecked(True)
            form.xSMin.setEnabled(False)
            form.xSMax.setEnabled(False)
        else:
            form.xAuto.setChecked(False)
            form.xSMin.setEnabled(True)
            form.xSMax.setEnabled(True)
        lim = ax.get_xlim()
        form.xSMin.setText(str(lim[0]))
        form.xSMax.setText(str(lim[1]))
        if ax.get_autoscaley_on():
            form.yAuto.setChecked(True)
            form.ySMin.setEnabled(False)
            form.ySMax.setEnabled(False)
        else:
            form.yAuto.setChecked(False)
            form.ySMin.setEnabled(True)
            form.ySMax.setEnabled(True)
        lim = ax.get_ylim()
        form.ySMin.setText(str(lim[0]))
        form.ySMax.setText(str(lim[1]))


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
