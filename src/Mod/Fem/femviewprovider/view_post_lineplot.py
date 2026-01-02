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
from PySide import QtGui, QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP


import io
import numpy as np
import matplotlib as mpl

from vtkmodules.numpy_interface.dataset_adapter import VTKArray

from . import view_base_fempostextractors
from . import view_base_fempostvisualization
from femtaskpanels import task_post_lineplot

from . import view_base_femobject

_GuiPropHelper = view_base_femobject._GuiPropHelper


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

        self._setup_color_button(self.widget.Color, vobj.Color, self.colorChanged)

        self.widget.Legend.editingFinished.connect(self.legendChanged)
        self.widget.MarkerStyle.activated.connect(self.markerStyleChanged)
        self.widget.LineStyle.activated.connect(self.lineStyleChanged)
        self.widget.MarkerSize.valueChanged.connect(self.markerSizeChanged)
        self.widget.LineWidth.valueChanged.connect(self.lineWidthChanged)

        # sometimes weird sizes occur with spinboxes
        self.widget.MarkerSize.setMaximumHeight(self.widget.MarkerStyle.sizeHint().height())
        self.widget.LineWidth.setMaximumHeight(self.widget.LineStyle.sizeHint().height())

    def _setup_color_button(self, button, fcColor, callback):

        barColor = QtGui.QColor(*[v * 255 for v in fcColor])
        icon_size = button.iconSize()
        icon_size.setWidth(icon_size.width() * 2)
        button.setIconSize(icon_size)
        pixmap = QtGui.QPixmap(icon_size)
        pixmap.fill(barColor)
        button.setIcon(pixmap)

        action = QtGui.QWidgetAction(button)
        diag = QtGui.QColorDialog(barColor, parent=button)
        diag.setOption(QtGui.QColorDialog.DontUseNativeDialog, True)
        diag.accepted.connect(action.trigger)
        diag.rejected.connect(action.trigger)
        diag.colorSelected.connect(callback)

        action.setDefaultWidget(diag)
        button.addAction(action)
        button.setPopupMode(QtGui.QToolButton.InstantPopup)

    @QtCore.Slot(QtGui.QColor)
    def colorChanged(self, color):

        pixmap = QtGui.QPixmap(self.widget.Color.iconSize())
        pixmap.fill(color)
        self.widget.Color.setIcon(pixmap)

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


class EditFieldAppWidget(QtGui.QWidget):

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
        self._post_dialog._enumPropertyToCombobox(
            self._object, "XComponent", self.widget.XComponent
        )
        self._post_dialog._enumPropertyToCombobox(self._object, "YField", self.widget.YField)
        self._post_dialog._enumPropertyToCombobox(
            self._object, "YComponent", self.widget.YComponent
        )
        self.widget.Extract.setChecked(self._object.ExtractFrames)

        self.widget.XField.activated.connect(self.xFieldChanged)
        self.widget.XComponent.activated.connect(self.xComponentChanged)
        self.widget.YField.activated.connect(self.yFieldChanged)
        self.widget.YComponent.activated.connect(self.yComponentChanged)
        self.widget.Extract.toggled.connect(self.extractionChanged)

    @QtCore.Slot(int)
    def xFieldChanged(self, index):
        self._object.XField = index
        self._post_dialog._enumPropertyToCombobox(
            self._object, "XComponent", self.widget.XComponent
        )
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def xComponentChanged(self, index):
        self._object.XComponent = index
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def yFieldChanged(self, index):
        self._object.YField = index
        self._post_dialog._enumPropertyToCombobox(
            self._object, "YComponent", self.widget.YComponent
        )
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def yComponentChanged(self, index):
        self._object.YComponent = index
        self._post_dialog._recompute()

    @QtCore.Slot(bool)
    def extractionChanged(self, extract):
        self._object.ExtractFrames = extract
        self._post_dialog._recompute()


class EditIndexAppWidget(QtGui.QWidget):

    def __init__(self, obj, post_dialog):
        super().__init__()

        self._object = obj
        self._post_dialog = post_dialog

        # load the ui and set it up
        self.widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/PostLineplotIndexAppEdit.ui"
        )
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.widget)
        self.setLayout(layout)

        self.__init_widget()

    def __init_widget(self):
        # set the other properties

        self.widget.Index.setValue(self._object.Index)
        self._post_dialog._enumPropertyToCombobox(self._object, "YField", self.widget.YField)
        self._post_dialog._enumPropertyToCombobox(
            self._object, "YComponent", self.widget.YComponent
        )

        self.widget.Index.valueChanged.connect(self.indexChanged)
        self.widget.YField.activated.connect(self.yFieldChanged)
        self.widget.YComponent.activated.connect(self.yComponentChanged)

        # sometimes weird sizes occur with spinboxes
        self.widget.Index.setMaximumHeight(self.widget.YField.sizeHint().height())

    @QtCore.Slot(int)
    def indexChanged(self, value):
        self._object.Index = value
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def yFieldChanged(self, index):
        self._object.YField = index
        self._post_dialog._enumPropertyToCombobox(
            self._object, "YComponent", self.widget.YComponent
        )
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def yComponentChanged(self, index):
        self._object.YComponent = index
        self._post_dialog._recompute()


class VPPostLineplotFieldData(view_base_fempostextractors.VPPostExtractor):
    """
    A View Provider for extraction of 2D field data specially for histograms
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
                doc=QT_TRANSLATE_NOOP("FEM", "The name used in the plots legend"),
                value="",
            ),
            _GuiPropHelper(
                type="App::PropertyColor",
                name="Color",
                group="Lineplot",
                doc=QT_TRANSLATE_NOOP("FEM", "The color the line and the markers are drawn with"),
                value=(0, 85, 255, 255),
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="LineStyle",
                group="Lineplot",
                doc=QT_TRANSLATE_NOOP("FEM", "The style the line is drawn in"),
                value=["-", "--", "-.", ":", "None"],
            ),
            _GuiPropHelper(
                type="App::PropertyFloatConstraint",
                name="LineWidth",
                group="Lineplot",
                doc=QT_TRANSLATE_NOOP("FEM", "The width the line is drawn with"),
                value=(1, 0.1, 99, 0.1),
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="MarkerStyle",
                group="Lineplot",
                doc=QT_TRANSLATE_NOOP("FEM", "The style the data markers are drawn with"),
                value=["None", "*", "+", "s", ".", "o", "x"],
            ),
            _GuiPropHelper(
                type="App::PropertyFloatConstraint",
                name="MarkerSize",
                group="Lineplot",
                doc=QT_TRANSLATE_NOOP("FEM", "The size the data markers are drawn in"),
                value=(5, 0.1, 99, 0.1),
            ),
        ]
        return super()._get_properties() + prop

    def attach(self, vobj):
        self.Object = vobj.Object
        self.ViewObject = vobj

    def getIcon(self):
        return ":/icons/FEM_PostField.svg"

    def get_app_edit_widget(self, post_dialog):
        return EditFieldAppWidget(self.Object, post_dialog)

    def get_view_edit_widget(self, post_dialog):
        return EditViewWidget(self.Object, post_dialog)

    def get_preview(self):
        # Returns the preview tuple of icon and label: (QPixmap, str)
        # Note: QPixmap in ratio 2:1

        fig = mpl.pyplot.figure(figsize=(0.2, 0.1), dpi=1000)
        ax = mpl.pyplot.Axes(fig, [0.0, 0.0, 1.0, 1.0])
        ax.set_axis_off()
        fig.add_axes(ax)
        kwargs = self.get_kw_args()
        kwargs["markevery"] = [1]
        ax.plot([0, 0.5, 1], [0.5, 0.5, 0.5], **kwargs)
        data = io.BytesIO()
        mpl.pyplot.savefig(data, bbox_inches=0, transparent=True)
        mpl.pyplot.close()

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

    def get_default_color_property(self):
        return "Color"


class VPPostLineplotIndexOverFrames(VPPostLineplotFieldData):
    """
    A View Provider for extraction of 2D index over frames data
    """

    def __init__(self, vobj):
        super().__init__(vobj)

    def getIcon(self):
        return ":/icons/FEM_PostIndex.svg"

    def get_app_edit_widget(self, post_dialog):
        return EditIndexAppWidget(self.Object, post_dialog)


class VPPostLineplot(view_base_fempostvisualization.VPPostVisualization):
    """
    A View Provider for Lineplot plots
    """

    def __init__(self, vobj):
        super().__init__(vobj)

    def _get_properties(self):

        prop = [
            _GuiPropHelper(
                type="App::PropertyBool",
                name="Grid",
                group="Lineplot",
                doc=QT_TRANSLATE_NOOP(
                    "FEM", "If be the bars should show the cumulative sum left to right"
                ),
                value=True,
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="Scale",
                group="Lineplot",
                doc=QT_TRANSLATE_NOOP("FEM", "The scale the axis are drawn in"),
                value=["linear", "semi-log x", "semi-log y", "log"],
            ),
            _GuiPropHelper(
                type="App::PropertyString",
                name="Title",
                group="Plot",
                doc=QT_TRANSLATE_NOOP("FEM", "The histogram plot title"),
                value="",
            ),
            _GuiPropHelper(
                type="App::PropertyString",
                name="XLabel",
                group="Plot",
                doc=QT_TRANSLATE_NOOP("FEM", "The label shown for the histogram X axis"),
                value="",
            ),
            _GuiPropHelper(
                type="App::PropertyString",
                name="YLabel",
                group="Plot",
                doc=QT_TRANSLATE_NOOP("FEM", "The label shown for the histogram Y axis"),
                value="",
            ),
            _GuiPropHelper(
                type="App::PropertyBool",
                name="Legend",
                group="Plot",
                doc=QT_TRANSLATE_NOOP("FEM", "Determines if the legend is plotted"),
                value=True,
            ),
            _GuiPropHelper(
                type="App::PropertyEnumeration",
                name="LegendLocation",
                group="Plot",
                doc=QT_TRANSLATE_NOOP("FEM", "Determines if the legend is plotted"),
                value=[
                    "best",
                    "upper right",
                    "upper left",
                    "lower left",
                    "lower right",
                    "right",
                    "center left",
                    "center right",
                    "lower center",
                    "upper center",
                    "center",
                ],
            ),
        ]
        return prop

    def getIcon(self):
        return ":/icons/FEM_PostLineplot.svg"

    def setEdit(self, vobj, mode):

        # build up the task panel
        taskd = task_post_lineplot._TaskPanel(vobj)

        # show it
        FreeCADGui.Control.showDialog(taskd)

        return True

    def show_visualization(self):

        if not hasattr(self, "_plot") or not self._plot:
            main = FreeCADGui.getMainWindow()
            self._plot = Plot.Plot()
            self._plot.setWindowTitle(self.Object.Label)
            self._plot.setParent(main)
            self._plot.setWindowFlags(QtGui.Qt.Dialog)
            self._plot.resize(
                main.size().height() / 2, main.size().height() / 3
            )  # keep the aspect ratio
            self.update_visualization()

        self._plot.show()

    def get_kw_args(self, obj):
        view = obj.ViewObject
        if not view or not hasattr(view, "Proxy"):
            return {}
        if not hasattr(view.Proxy, "get_kw_args"):
            return {}
        return view.Proxy.get_kw_args()

    def update_visualization(self):

        if not hasattr(self, "_plot") or not self._plot:
            return

        self._plot.axes.clear()

        # we do not iterate the table, but iterate the children. This makes it possible
        # to attribute the correct styles
        plotted = False
        for child in self.Object.Group:

            table = child.Table
            kwargs = self.get_kw_args(child)

            # iterate over the table and plot all (note: column 0 is always X!)
            color_factor = np.linspace(1, 0.5, int(table.GetNumberOfColumns() / 2))
            legend_multiframe = table.GetNumberOfColumns() > 2

            for i in range(0, table.GetNumberOfColumns(), 2):

                plotted = True

                # add the kw args, with some slide change over color for multiple frames
                tmp_args = {}
                for key in kwargs:
                    if "color" in key:
                        value = np.array(kwargs[key]) * color_factor[int(i / 2)]
                        tmp_args[key] = mpl.colors.to_hex(value)
                    else:
                        tmp_args[key] = kwargs[key]

                xdata = VTKArray(table.GetColumn(i))
                ydata = VTKArray(table.GetColumn(i + 1))

                # ensure points are visible if it is a single datapoint
                if len(xdata) == 1 and tmp_args["marker"] == "None":
                    tmp_args["marker"] = "o"

                # legend labels
                if child.ViewObject.Legend:
                    if not legend_multiframe:
                        label = child.ViewObject.Legend
                    else:
                        postfix = table.GetColumnName(i + 1).split("-")[-1]
                        label = child.ViewObject.Legend + " - " + postfix
                else:
                    legend_prefix = ""
                    if len(self.Object.Group) > 1:
                        legend_prefix = child.Source.Label + ": "
                    label = legend_prefix + table.GetColumnName(i + 1)

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

        if self.ViewObject.Legend and plotted:
            self._plot.axes.legend(loc=self.ViewObject.LegendLocation)

        self._plot.axes.grid(self.ViewObject.Grid)
        self._plot.update()

    def get_next_default_color(self):
        # we use the next color in order. We do not check (yet) if this
        # color is already taken
        i = len(self.Object.Group)
        cmap = mpl.pyplot.get_cmap("tab10")
        return cmap(i)
