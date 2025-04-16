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

import io
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt

from vtkmodules.numpy_interface.dataset_adapter import VTKArray

from . import view_post_extract
from . import view_base_fempostvisualization
from femtaskpanels import task_post_lineplot

_GuiPropHelper = view_base_fempostvisualization._GuiPropHelper

class EditViewWidget(QtGui.QWidget):

    def __init__(self, obj, post_dialog):
        super().__init__()

        self._object = obj
        self._post_dialog = post_dialog

        # load the ui and set it up
        self.widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/PostLineplotFieldViewEdit.ui"
        )
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.widget)
        self.setLayout(layout)

        self.__init_widget()

    def __init_widget(self):
        vobj = self._object.ViewObject

        self.widget.Legend.setText(vobj.Legend)
        self._post_dialog._enumPropertyToCombobox(vobj, "LineStyle", self.widget.LineStyle)
        self._post_dialog._enumPropertyToCombobox(vobj, "MarkerStyle", self.widget.MarkerStyle)
        self.widget.LineWidth.setValue(vobj.LineWidth)
        self.widget.MarkerSize.setValue(vobj.MarkerSize)
        self.widget.Color.setProperty("color", QtGui.QColor(*[v*255 for v in vobj.Color]))

        self.widget.Legend.editingFinished.connect(self.legendChanged)
        self.widget.MarkerStyle.activated.connect(self.markerStyleChanged)
        self.widget.LineStyle.activated.connect(self.lineStyleChanged)
        self.widget.MarkerSize.valueChanged.connect(self.markerSizeChanged)
        self.widget.LineWidth.valueChanged.connect(self.lineWidthChanged)
        self.widget.Color.changed.connect(self.colorChanged)

    @QtCore.Slot()
    def colorChanged(self):
        color = self.widget.Color.property("color")
        self._object.ViewObject.Color = color.getRgb()

    @QtCore.Slot(float)
    def lineWidthChanged(self, value):
        self._object.ViewObject.LineWidth = value

    @QtCore.Slot(float)
    def markerSizeChanged(self, value):
        self._object.ViewObject.MarkerSize = value

    @QtCore.Slot(int)
    def markerStyleChanged(self, index):
        self._object.ViewObject.MarkerStyle = index

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
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/PostLineplotFieldAppEdit.ui"
        )
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.widget)
        self.setLayout(layout)

        self.__init_widget()

    def __init_widget(self):
        # set the other properties

        self._post_dialog._enumPropertyToCombobox(self._object, "XField", self.widget.XField)
        self._post_dialog._enumPropertyToCombobox(self._object, "XComponent", self.widget.XComponent)
        self._post_dialog._enumPropertyToCombobox(self._object, "YField", self.widget.YField)
        self._post_dialog._enumPropertyToCombobox(self._object, "YComponent", self.widget.YComponent)
        self.widget.Extract.setChecked(self._object.ExtractFrames)

        self.widget.XField.activated.connect(self.xFieldChanged)
        self.widget.XComponent.activated.connect(self.xComponentChanged)
        self.widget.YField.activated.connect(self.yFieldChanged)
        self.widget.YComponent.activated.connect(self.yComponentChanged)
        self.widget.Extract.toggled.connect(self.extractionChanged)

    @QtCore.Slot(int)
    def xFieldChanged(self, index):
        self._object.XField = index
        self._post_dialog._enumPropertyToCombobox(self._object, "XComponent", self.widget.XComponent)
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def xComponentChanged(self, index):
        self._object.XComponent = index
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def yFieldChanged(self, index):
        self._object.YField = index
        self._post_dialog._enumPropertyToCombobox(self._object, "YComponent", self.widget.YComponent)
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def yComponentChanged(self, index):
        self._object.YComponent = index
        self._post_dialog._recompute()

    @QtCore.Slot(bool)
    def extractionChanged(self, extract):
        self._object.ExtractFrames = extract
        self._post_dialog._recompute()


class VPPostLineplotFieldData(view_post_extract.VPPostExtractor):
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
                group="Lineplot",
                doc="The name used in the plots legend",
                value="",
            ),
            _GuiPropHelper(
                type="App::PropertyColor",
                name="Color",
                group="Lineplot",
                doc="The color the line and the markers are drawn with",
                value=(0, 85, 255, 255),
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="LineStyle",
                group="Lineplot",
                doc="The style the line is drawn in",
                value=['-', '--', '-.', ':', 'None'],
            ),
            _GuiPropHelper(
                type="App::PropertyFloatConstraint",
                name="LineWidth",
                group="Lineplot",
                doc="The width the line is drawn with",
                value=(1, 0.1, 99, 0.1),
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="MarkerStyle",
                group="Lineplot",
                doc="The style the data markers are drawn with",
                value=['None', '*', '+', 's', '.', 'o', 'x'],
            ),
            _GuiPropHelper(
                type="App::PropertyFloatConstraint",
                name="MarkerSize",
                group="Lineplot",
                doc="The size the data markers are drawn in",
                value=(10, 0.1, 99, 0.1),
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
        # Returns the preview tuple of icon and label: (QPixmap, str)
        # Note: QPixmap in ratio 2:1

        fig = plt.figure(figsize=(0.2,0.1), dpi=1000)
        ax = plt.Axes(fig, [0., 0., 1., 1.])
        ax.set_axis_off()
        fig.add_axes(ax)
        kwargs = self.get_kw_args()
        kwargs["markevery"] = [1]
        ax.plot([0,0.5,1],[0.5,0.5,0.5], **kwargs)
        data = io.BytesIO()
        plt.savefig(data, bbox_inches=0, transparent=True)
        plt.close()

        pixmap = QtGui.QPixmap()
        pixmap.loadFromData(data.getvalue())

        return (pixmap, self.ViewObject.Legend)


    def get_kw_args(self):
        # builds kw args from the properties
        kwargs = {}

        # colors need a workaround, some error occurs with rgba tuple
        kwargs["color"] = self.ViewObject.Color
        kwargs["markeredgecolor"] = self.ViewObject.Color
        kwargs["markerfacecolor"] = self.ViewObject.Color
        kwargs["linestyle"] = self.ViewObject.LineStyle
        kwargs["linewidth"] = self.ViewObject.LineWidth
        kwargs["marker"] = self.ViewObject.MarkerStyle
        kwargs["markersize"] = self.ViewObject.MarkerSize
        return kwargs


class VPPostLineplot(view_base_fempostvisualization.VPPostVisualization):
    """
    A View Provider for Lineplot plots
    """

    def __init__(self, vobj):
        super().__init__(vobj)
        vobj.addExtension("Gui::ViewProviderGroupExtensionPython")

    def _get_properties(self):

        prop = [
            _GuiPropHelper(
                type="App::PropertyBool",
                name="Grid",
                group="Lineplot",
                doc="If be the bars shoud show the cumulative sum left to rigth",
                value=False,
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="Scale",
                group="Lineplot",
                doc="The scale the axis are drawn in",
                value=["linear","semi-log x", "semi-log y", "log"],
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
        return ":/icons/FEM_PostLineplot.svg"

    def doubleClicked(self,vobj):

        self.show_visualization()
        super().doubleClicked(vobj)

    def setEdit(self, vobj, mode):

        # build up the task panel
        taskd = task_post_lineplot._TaskPanel(vobj)

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

        # we do not iterate the table, but iterate the children. This makes it possible
        # to attribute the correct styles
        for child in self.Object.Group:

            table = child.Table
            kwargs = self.get_kw_args(child)

            # iterate over the table and plot all (note: column 0 is always X!)
            color_factor = np.linspace(1,0.5,int(table.GetNumberOfColumns()/2))
            legend_multiframe = table.GetNumberOfColumns() > 2

            for i in range(0,table.GetNumberOfColumns(),2):

                # add the kw args, with some slide change over color for multiple frames
                tmp_args = {}
                for key in kwargs:
                    if "color" in key:
                        value = np.array(kwargs[key])*color_factor[int(i/2)]
                        tmp_args[key] = mpl.colors.to_hex(value)
                    else:
                        tmp_args[key] = kwargs[key]

                xdata = VTKArray(table.GetColumn(i))
                ydata = VTKArray(table.GetColumn(i+1))

                # legend labels
                if child.ViewObject.Legend:
                    if not legend_multiframe:
                        label = child.ViewObject.Legend
                    else:
                        postfix = table.GetColumnName(i+1).split("-")[-1]
                        label = child.ViewObject.Legend + " - " + postfix
                else:
                    legend_prefix = ""
                    if len(self.Object.Group) > 1:
                        legend_prefix = child.Source.Label + ": "
                    label = legend_prefix + table.GetColumnName(i+1)

                match self.ViewObject.Scale:
                    case "log":
                        self._plot.axes.loglog(xdata, ydata, **tmp_args, label=label)
                    case "semi-log x":
                        self._plot.axes.semilogx(xdata, ydata, **tmp_args, label=label)
                    case "semi-log y":
                        self._plot.axes.semilogy(xdata, ydata, **tmp_args, label=label)
                    case _:
                        self._plot.axes.plot(xdata, ydata, **tmp_args, label=label)

        if self.ViewObject.Title:
            self._plot.axes.set_title(self.ViewObject.Title)
        if self.ViewObject.XLabel:
            self._plot.axes.set_xlabel(self.ViewObject.XLabel)
        if self.ViewObject.YLabel:
            self._plot.axes.set_ylabel(self.ViewObject.YLabel)

        if self.ViewObject.Legend and self.Object.Group:
            self._plot.axes.legend(loc = self.ViewObject.LegendLocation)

        self._plot.axes.grid(self.ViewObject.Grid)

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
