# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM postprocessing line plot ViewProvider for the document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package view_post_lineplot
#  \ingroup FEM
#  \brief view provider for post line plot object

import FreeCAD
import FreeCADGui

import Plot
import FemGui
from PySide import QtGui, QtCore

import numpy as np
import matplotlib as mpl

from vtkmodules.numpy_interface.dataset_adapter import VTKArray

from . import view_post_extract
from . import view_base_fempostvisualization
from femtaskpanels import task_post_histogram

_GuiPropHelper = view_base_fempostvisualization._GuiPropHelper

class EditViewWidget(QtGui.QWidget):

    def __init__(self, obj, post_dialog):
        super().__init__()

        self._object = obj
        self._post_dialog = post_dialog

        # load the ui and set it up
        self.widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/PostHistogramFieldViewEdit.ui"
        )
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.widget)
        self.setLayout(layout)

        self.__init_widget()

    def __init_widget(self):
        vobj = self._object.ViewObject

        self.widget.Legend.setText(vobj.Legend)
        self._post_dialog._enumPropertyToCombobox(vobj, "Hatch", self.widget.Hatch)
        self._post_dialog._enumPropertyToCombobox(vobj, "LineStyle", self.widget.LineStyle)
        self.widget.LineWidth.setValue(vobj.LineWidth)
        self.widget.HatchDensity.setValue(vobj.HatchDensity)
        self.widget.BarColor.setProperty("color", QtGui.QColor(*[v*255 for v in vobj.BarColor]))
        self.widget.LineColor.setProperty("color", QtGui.QColor(*[v*255 for v in vobj.LineColor]))

        self.widget.Legend.editingFinished.connect(self.legendChanged)
        self.widget.Hatch.activated.connect(self.hatchPatternChanged)
        self.widget.LineStyle.activated.connect(self.lineStyleChanged)
        self.widget.HatchDensity.valueChanged.connect(self.hatchDensityChanged)
        self.widget.LineWidth.valueChanged.connect(self.lineWidthChanged)
        self.widget.LineColor.changed.connect(self.lineColorChanged)
        self.widget.BarColor.changed.connect(self.barColorChanged)

    @QtCore.Slot()
    def lineColorChanged(self):
        color = self.widget.LineColor.property("color")
        self._object.ViewObject.LineColor = color.getRgb()

    @QtCore.Slot()
    def barColorChanged(self):
        color = self.widget.BarColor.property("color")
        self._object.ViewObject.BarColor = color.getRgb()

    @QtCore.Slot(float)
    def lineWidthChanged(self, value):
        self._object.ViewObject.LineWidth = value

    @QtCore.Slot(float)
    def hatchDensityChanged(self, value):
        self._object.ViewObject.HatchDensity = value

    @QtCore.Slot(int)
    def hatchPatternChanged(self, index):
        self._object.ViewObject.Hatch = index

    @QtCore.Slot(int)
    def lineStyleChanged(self, index):
        self._object.ViewObject.LineStyle = index

    @QtCore.Slot()
    def legendChanged(self):
        self._object.ViewObject.Legend = self.widget.Legend.text()


class EditAppWidget(QtGui.QWidget):

    def __init__(self, obj, post_dialog):
        super().__init__()

        self._object = obj
        self._post_dialog = post_dialog

        # load the ui and set it up
        self.widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/PostHistogramFieldAppEdit.ui"
        )
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.widget)
        self.setLayout(layout)

        self.__init_widget()

    def __init_widget(self):
        # set the other properties

        self._post_dialog._enumPropertyToCombobox(self._object, "XField", self.widget.Field)
        self._post_dialog._enumPropertyToCombobox(self._object, "XComponent", self.widget.Component)
        self.widget.Extract.setChecked(self._object.ExtractFrames)

        self.widget.Field.activated.connect(self.fieldChanged)
        self.widget.Component.activated.connect(self.componentChanged)
        self.widget.Extract.toggled.connect(self.extractionChanged)

    @QtCore.Slot(int)
    def fieldChanged(self, index):
        self._object.XField = index
        self._post_dialog._enumPropertyToCombobox(self._object, "XComponent", self.widget.Component)
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def componentChanged(self, index):
        self._object.XComponent = index
        self._post_dialog._recompute()

    @QtCore.Slot(bool)
    def extractionChanged(self, extract):
        self._object.ExtractFrames = extract
        self._post_dialog._recompute()


class VPPostHistogramFieldData(view_post_extract.VPPostExtractor):
    """
    A View Provider for extraction of 1D field data specialy for histograms
    """

    def __init__(self, vobj):
        super().__init__(vobj)
        vobj.Proxy = self

    def _get_properties(self):

        prop = [
            _GuiPropHelper(
                type="App::PropertyString",
                name="Legend",
                group="HistogramPlot",
                doc="The name used in the plots legend",
                value="",
            ),
            _GuiPropHelper(
                type="App::PropertyColor",
                name="BarColor",
                group="HistogramBar",
                doc="The color the data bin area is drawn with",
                value=(0, 85, 255, 255),
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="Hatch",
                group="HistogramBar",
                doc="The hatch pattern drawn in the bar",
                value=['None', '/', '\\', '|', '-', '+', 'x', 'o', 'O', '.', '*'],
            ),
            _GuiPropHelper(
                type="App::PropertyIntegerConstraint",
                name="HatchDensity",
                group="HistogramBar",
                doc="The line width of the hatch",
                value=(1, 1, 99, 1),
            ),
            _GuiPropHelper(
                type="App::PropertyColor",
                name="LineColor",
                group="HistogramLine",
                doc="The color the data bin area is drawn with",
                value=(0, 85, 255, 255),
            ),
            _GuiPropHelper(
                type="App::PropertyFloatConstraint",
                name="LineWidth",
                group="HistogramLine",
                doc="The width of the bar, between 0 and 1 (1 being without gaps)",
                value=(1, 0, 99, 0.1),
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="LineStyle",
                group="HistogramLine",
                doc="The style the line is drawn in",
                value=['None', '-', '--', '-.', ':'],
            ),
        ]
        return super()._get_properties() + prop

    def attach(self, vobj):
        self.Object = vobj.Object
        self.ViewObject = vobj

    def getIcon(self):
        return ":/icons/FEM_PostField.svg"

    def get_app_edit_widget(self, post_dialog):
        return EditAppWidget(self.Object, post_dialog)

    def get_view_edit_widget(self, post_dialog):
        return EditViewWidget(self.Object, post_dialog)

    def get_preview(self):
        return (QtGui.QPixmap(), self.ViewObject.Legend)

    def get_kw_args(self):
        # builds kw args from the properties
        kwargs = {}

        # colors need a workaround, some error occurs with rgba tuple
        kwargs["edgecolor"] = self.ViewObject.LineColor
        kwargs["facecolor"] = self.ViewObject.BarColor
        kwargs["linestyle"] = self.ViewObject.LineStyle
        kwargs["linewidth"] = self.ViewObject.LineWidth
        if self.ViewObject.Hatch != "None":
            kwargs["hatch"] = self.ViewObject.Hatch*self.ViewObject.HatchDensity

        return kwargs


class VPPostHistogram(view_base_fempostvisualization.VPPostVisualization):
    """
    A View Provider for Histogram plots
    """

    def __init__(self, vobj):
        super().__init__(vobj)
        vobj.addExtension("Gui::ViewProviderGroupExtensionPython")

    def _get_properties(self):

        prop = [
            _GuiPropHelper(
                type="App::PropertyBool",
                name="Cumulative",
                group="Histogram",
                doc="If be the bars shoud show the cumulative sum left to rigth",
                value=False,
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="Type",
                group="Histogram",
                doc="The type of histogram plotted",
                value=["bar","barstacked", "step", "stepfilled"],
            ),
            _GuiPropHelper(
                type="App::PropertyFloatConstraint",
                name="BarWidth",
                group="Histogram",
                doc="The width of the bar, between 0 and 1 (1 being without gaps)",
                value=(0.9, 0, 1, 0.05),
            ),
            _GuiPropHelper(
                type="App::PropertyFloatConstraint",
                name="HatchLineWidth",
                group="Histogram",
                doc="The line width of all drawn hatch patterns",
                value=(1, 0, 99, 0.1),
            ),
            _GuiPropHelper(
                type="App::PropertyInteger",
                name="Bins",
                group="Histogram",
                doc="The number of bins the data is split into",
                value=10,
            ),
            _GuiPropHelper(
                type="App::PropertyString",
                name="Title",
                group="Plot",
                doc="The histogram plot title",
                value="",
            ),
            _GuiPropHelper(
                type="App::PropertyString",
                name="XLabel",
                group="Plot",
                doc="The label shown for the histogram X axis",
                value="",
            ),
            _GuiPropHelper(
                type="App::PropertyString",
                name="YLabel",
                group="Plot",
                doc="The label shown for the histogram Y axis",
                value="",
            ),
            _GuiPropHelper(
                type="App::PropertyBool",
                name="Legend",
                group="Plot",
                doc="Determines if the legend is plotted",
                value=True,
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="LegendLocation",
                group="Plot",
                doc="Determines if the legend is plotted",
                value=['best','upper right','upper left','lower left','lower right','right',
                       'center left','center right','lower center','upper center','center'],
            ),

        ]
        return prop

    def getIcon(self):
        return ":/icons/FEM_PostHistogram.svg"

    def doubleClicked(self,vobj):

        self.show_visualization()
        super().doubleClicked(vobj)

    def setEdit(self, vobj, mode):

        # build up the task panel
        taskd = task_post_histogram._TaskPanel(vobj)

        #show it
        FreeCADGui.Control.showDialog(taskd)

        return True


    def show_visualization(self):

        if not hasattr(self, "_plot") or not self._plot:
            self._plot = Plot.Plot()
            self._dialog = QtGui.QDialog(Plot.getMainWindow())
            box = QtGui.QVBoxLayout()
            box.addWidget(self._plot)
            self._dialog.setLayout(box)

        self.drawPlot()
        self._dialog.show()

    def get_kw_args(self, obj):
        view = obj.ViewObject
        if not view or not hasattr(view, "Proxy"):
            return {}
        if not hasattr(view.Proxy, "get_kw_args"):
            return {}
        return view.Proxy.get_kw_args()

    def drawPlot(self):

        if not hasattr(self, "_plot") or not self._plot:
            return

        self._plot.axes.clear()
        bins = self.ViewObject.Bins

        # we do not iterate the table, but iterate the children. This makes it possible
        # to attribute the correct styles
        full_args = {}
        full_data = []
        labels = []
        for child in self.Object.Group:

            table = child.Table
            kwargs = self.get_kw_args(child)

            # iterate over the table and plot all
            color_factor = np.linspace(1,0.5,table.GetNumberOfColumns())
            legend_multiframe = table.GetNumberOfColumns() > 1
            for i in range(table.GetNumberOfColumns()):

                # add the kw args, with some slide change over color for multiple frames
                for key in kwargs:
                    if not (key in full_args):
                        full_args[key] = []

                    if "color" in key:
                        value = np.array(kwargs[key])*color_factor[i]
                        full_args[key].append(mpl.colors.to_hex(value))
                    else:
                        full_args[key].append(kwargs[key])

                data = VTKArray(table.GetColumn(i))
                full_data.append(data)

                # legend labels
                if child.ViewObject.Legend:
                    if not legend_multiframe:
                        labels.append(child.ViewObject.Legend)
                    else:
                        postfix = table.GetColumnName(i).split("-")[-1]
                        labels.append(child.ViewObject.Legend + " - " + postfix)
                else:
                    legend_prefix = ""
                    if len(self.Object.Group) > 1:
                        legend_prefix = child.Source.Label + ": "
                    labels.append(legend_prefix + table.GetColumnName(i))


        full_args["hatch_linewidth"] = self.ViewObject.HatchLineWidth
        full_args["rwidth"] = self.ViewObject.BarWidth
        full_args["cumulative"] = self.ViewObject.Cumulative
        full_args["histtype"] = self.ViewObject.Type
        full_args["label"] = labels

        self._plot.axes.hist(full_data, bins, **full_args)

        if self.ViewObject.Title:
            self._plot.axes.set_title(self.ViewObject.Title)
        if self.ViewObject.XLabel:
            self._plot.axes.set_xlabel(self.ViewObject.XLabel)
        if self.ViewObject.YLabel:
            self._plot.axes.set_ylabel(self.ViewObject.YLabel)

        if self.ViewObject.Legend and labels:
            self._plot.axes.legend(loc = self.ViewObject.LegendLocation)

        self._plot.update()


    def updateData(self, obj, prop):
        # we only react if the table changed, as then know that new data is available
        if prop == "Table":
            self.drawPlot()


    def onChanged(self, vobj, prop):

        # for all property changes we need to redraw the plot
        self.drawPlot()

    def childViewPropertyChanged(self, vobj, prop):

        # on of our extractors has a changed view property.
        self.drawPlot()

    def dumps(self):
        return None


    def loads(self, state):
        return None
