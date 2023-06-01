# ***************************************************************************
# *   Copyright (c) 2023 David Carter <dcarter@dvidcarter.ca>               *
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

__title__ = "FreeCAD material value editor"
__author__ = "David Carter"
__url__ = "http://www.freecad.org"

import os
import math
from PySide import QtCore, QtGui
from PySide.QtCore import Qt
from PySide2.QtCharts import QtCharts

import FreeCAD
import FreeCADGui

class ChartView(QtCharts.QChartView):
    # Modified code from what is found here https://stackoverflow.com/questions/60058507/draw-cursor-on-a-qchartview-object
    _x = None

    @property
    def x(self):
        return self._x

    @x.setter
    def x(self, x):
        self._x = x
        self.scene().update()

    def _scaleX(self):
        area = self.chart().plotArea()
        count = self.chart().series()[0].count() - 1 # Assumes ordered series numbered from 0

        chartX = int((area.width() / count) * (self._x / 1000) + area.left())

        return chartX

    def drawForeground(self, painter, rect):
        if self.x is None:
            return
        painter.save()

        pen = QtGui.QPen(QtGui.QColor("indigo"))
        pen.setWidth(3)
        painter.setPen(pen)

        p = QtCore.QPointF(self._scaleX(), 0)
        r = self.chart().plotArea()

        p1 = QtCore.QPointF(p.x(), r.top())
        p2 = QtCore.QPointF(p.x(), r.bottom())
        painter.drawLine(p1, p2)

        value_at_position = self._x / 1000.0 # In km
        for series_i in self.chart().series():
            pen2 = QtGui.QPen(series_i.color())
            pen2.setWidth(10)
            painter.setPen(pen2)

            # Find the nearest points
            min_distance_left = math.inf
            min_distance_right = math.inf
            nearest_point_left = None
            nearest_point_right = None
            exact_point = None

            for p_i in series_i.pointsVector():
                if p_i.x() > value_at_position:
                    if p_i.x() - value_at_position < min_distance_right:
                        min_distance_right = p_i.x() - value_at_position
                        nearest_point_right = p_i
                elif p_i.x() < value_at_position:
                    if value_at_position - p_i.x() < min_distance_left:
                        min_distance_left = value_at_position - p_i.x()
                        nearest_point_left = p_i
                else:
                    exact_point = p_i
                    nearest_point_left = None
                    nearest_point_right = None
                    break
            if nearest_point_right is not None and nearest_point_left is not None:
                # do linear interpolation
                k = ((nearest_point_right.y() - nearest_point_left.y()) / (nearest_point_right.x() - nearest_point_left.x()))
                point_interpolated_y = nearest_point_left.y() + k * (value_at_position - nearest_point_left.x())
                point_interpolated_x = value_at_position

                point_interpolated = QtCore.QPointF(point_interpolated_x, point_interpolated_y)

                painter.drawPoint(self.chart().mapToScene(self.chart().mapToPosition(point_interpolated)))
            if exact_point is not None:
                painter.drawPoint(self.chart().mapToScene(self.chart().mapToPosition(exact_point)))

        painter.restore()

class ListModel(QtCore.QAbstractTableModel):

    def __init__(self, data, rowCount, columnCount, rowHeaders, columnHeaders):
        super(ListModel, self).__init__()
        self._data = data
        self._rowCount = rowCount
        self._columnCount = columnCount
        self._rowHeaders = rowHeaders
        self._columnHeaders = columnHeaders

    def data(self, index, role):
        if role == Qt.DisplayRole:
            value = self._data[index.row()][index.column()]
            return str(value)

    def rowCount(self, index):
        return self._rowCount

    def columnCount(self, index):
        return self._columnCount

    def headerData(self, section, orientation, role):
        # section is the index of the column/row.
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal:
                if self._columnHeaders is not None: # and section in self._columnHeaders:
                    return str(self._columnHeaders[section])
                return ""

            if orientation == Qt.Vertical:
                if self._rowHeaders is not None: # and section in self._rowHeaders:
                    return str(self._rowHeaders[section])
                return ""


class ValueEditor:

    __distributions = {
        "Arcsine" : ("X min", True, "X max" , True),
        "Bernoulli" : ("p", True, "Value 2" , False),
        "Beta" : ("α", True, "β" , True),
        "Binomial" : ("n", True, "p" , True),
        "Cauchy-Lorentz" : ("location", True, "scale" , True),
        "Chi Squared" : ("ν", True, "Value 2" , False),
        "Exponential" : ("λ", True, "Value 2" , False),
        "Extreme Value" : ("location", True, "scale" , True),
        "F" : ("m", True, "n" , True),
        "Gamma" : ("shape", True, "scale" , True),
        "Geometric" : ("p", True, "Value 2" , False),
        # "Hyperexponential" : ("Value 1", True, "Value 2" , True), # Up to 4 params
        # "Hypergeometric" : ("Value 1", True, "Value 2" , True), # Up to 3 params
        "Inverse Chi Squared" : ("ν", True, "χ" , True),
        "Inverse Gamma" : ("α", True, "β" , True),
        "Inverse Gaussian" : ("mean", True, "scale" , True),
        "Kolmogorov-Smirnov" : ("n", True, "Value 2" , False),
        "Laplace" : ("location", True, "scale" , True),
        "Logistic" : ("location", True, "scale" , True),
        "Log Normal" : ("location", True, "scale" , True),
        "Negative Binomial" : ("n", True, "p" , True),
        # "Noncentral Beta" : ("Value 1", True, "Value 2" , True), # Up to 3 params
        "Noncentral Chi Squared" : ("ν", True, "λ" , True),
        # "Noncentral F" : ("Value 1", True, "Value 2" , True),
        "Noncentral T" : ("v", True, "δ" , True),
        "Normal (Gaussian)" : ("Mean", True, "σ" , True),
        "Pareto" : ("shape", True, "scale" , True),
        "Poisson" : ("mean", True, "Value 2" , False),
        "Rayleigh" : ("σ", True, "Value 2" , False),
        # "Skew Normal" : ("Value 1", True, "Value 2" , True),
        "Students t" : ("ν", True, "Value 2" , False),
        # "Triangular" : ("Value 1", True, "Value 2" , True),
        "Uniform" : ("lower", True, "upper" , True),
        "Weibull" : ("shape", True, "scale" , True),
    }

    def __init__(self):
        # load the UI file from the same directory as this script
        filePath = os.path.dirname(__file__) + os.sep
        self.widget = FreeCADGui.PySideUic.loadUi(filePath + "Resources" + os.sep + "ui" + os.sep + "ValueEditor.ui")

        # remove unused Help button
        self.widget.setWindowFlags(self.widget.windowFlags()
                                   & ~QtCore.Qt.WindowContextHelpButtonHint)

        # restore size and position
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material")
        width = param.GetInt("ValueEditorWidth", 441)
        height = param.GetInt("ValueEditorHeight", 626)
        self.widget.resize(width, height)

        # # additional UI fixes and tweaks
        widget = self.widget
        standardButtons = widget.standardButtons

        self.setTypeCombo()
        self.setDistributionCombo()
        self.setDefaultMeasureCombo()
        self.setArray()
        self.setChart()

        self.setVisibility()

        standardButtons.accepted.connect(self.accept)
        standardButtons.rejected.connect(self.reject)

    def setTypeCombo(self):
        combo = self.widget.comboType
        combo.addItems(("Scalar",
                        "Distribution",
                        "List",
                        "Array",
                        "Temperature Series",
                        "Pressure Series"))
        combo.setCurrentText("Scaler")

        combo.currentTextChanged.connect(self.onType)

    def onType(self, value):
        print("New type value {0}".format(value))
        self.updateArray()
        self.updateChart()
        self.setVisibility()

    def setDistributionCombo(self):
        combo = self.widget.comboDistribution
        combo.addItems(("Arcsine",
                        "Bernoulli",
                        "Beta",
                        "Binomial",
                        "Cauchy-Lorentz",
                        "Chi Squared",
                        "Exponential",
                        "Extreme Value",
                        "F",
                        "Gamma",
                        "Geometric",
                        # "Hyperexponential",
                        # "Hypergeometric",
                        "Inverse Chi Squared",
                        "Inverse Gamma",
                        "Inverse Gaussian",
                        "Kolmogorov-Smirnov",
                        "Laplace",
                        "Logistic",
                        "Log Normal",
                        "Negative Binomial",
                        # "Noncentral Beta",
                        "Noncentral Chi Squared",
                        # "Noncentral F",
                        "Noncentral T",
                        "Normal (Gaussian)",
                        "Pareto",
                        "Poisson",
                        "Rayleigh",
                        # "Skew Normal",
                        "Students t",
                        # "Triangular",
                        "Uniform",
                        "Weibull"))
        combo.setCurrentText("Normal (Gaussian)")

        combo.currentTextChanged.connect(self.onDistribution)

    def onDistribution(self, value):
        print("New dist value {0}".format(value))
        self.setVisibility()

    def setDefaultMeasureCombo(self):
        combo = self.widget.comboDefaultMeasure
        combo.addItems(("STP",
                        "NTP",
                        "Custom"))
        combo.setCurrentText("NTP")

        combo.currentTextChanged.connect(self.onDefaultMeasure)
        pass

    def onDefaultMeasure(self, value):
        print("New default measure value {0}".format(value))
        self.setVisibility()

    def setVisibility(self):
        type = self.widget.comboType.currentText()

        isScalar = (type == "Scalar")
        isArray = (type in ["List", "Array", "Temperature Series", "Pressure Series"])
        self.widget.editValue.setHidden(not isScalar)
        self.widget.labelValue.setHidden(not isScalar)
        self.widget.tableView.setHidden(not isArray)
        if type == "Array":
            self.widget.spinArrayX.setHidden(False)
            self.widget.spinArrayY.setHidden(False)
            self.widget.labelX.setHidden(False)
        else:
            self.widget.spinArrayX.setHidden(True)
            self.widget.spinArrayY.setHidden(True)
            self.widget.labelX.setHidden(True)

        if type in ["Temperature Series", "Pressure Series"]:
            # self.chartView.setHidden(False)
            self.widget.labelDefaultMeasure.setHidden(False)
            self.widget.editDefaultMeasure.setHidden(False)
            self.widget.comboDefaultMeasure.setHidden(False)
            if type == "Temperature Series":
                self.widget.labelDefaultMeasure.setText("Default Temperature")
            else:
                self.widget.labelDefaultMeasure.setText("Default Pressure")
        else:
            # self.chartView.setHidden(True)
            self.widget.labelDefaultMeasure.setHidden(True)
            self.widget.editDefaultMeasure.setHidden(True)
            self.widget.comboDefaultMeasure.setHidden(True)

        self.setDistributionVisibility()

    def setDistributionVisibility(self):
        type = self.widget.comboType.currentText()

        isDistribution = (type == "Distribution")

        self.widget.comboDistribution.setHidden(not isDistribution)
        self.widget.labelDistribution.setHidden(not isDistribution)

        distribution = self.widget.comboDistribution.currentText()
        if distribution in ValueEditor.__distributions:
            values = ValueEditor.__distributions[distribution]
            self.widget.labelDistValue1.setText(values[0])
            self.widget.labelDistValue2.setText(values[2])

            self.widget.editDistValue1.setHidden(not (values[1] and isDistribution))
            self.widget.labelDistValue1.setHidden(not (values[1] and isDistribution))
            self.widget.editDistValue2.setHidden(not (values[3] and isDistribution))
            self.widget.labelDistValue2.setHidden(not (values[3] and isDistribution))

        else:
            self.widget.labelDistValue1.setText("Value 1")
            self.widget.labelDistValue2.setText("Value 2")

            self.widget.editDistValue1.setHidden(not isDistribution)
            self.widget.labelDistValue1.setHidden(not isDistribution)
            self.widget.editDistValue2.setHidden(not isDistribution)
            self.widget.labelDistValue2.setHidden(not isDistribution)

    def setArray(self):
        # model = ListModel()
        # self.widget.tableView.setModel(model)
        # self.widget.tableView.setUniformRowHeights(True)

        # model.itemChanged.connect(self.modelChange)
        pass

    def updateArray(self):
        type = self.widget.comboType.currentText()
        model = self.widget.tableView.model()

        if type == "List":
            self.widget.tableView.horizontalHeader().setHidden(True)
            self.widget.tableView.verticalHeader().setHidden(False)
            data = [
                [98.6],
                [3.14],
                [7.28],
                [1.414],
                [2.718],
                []
            ]
            columns = ['A', 'B', 'C']
            rows=['1', '2', '3', '4', '5', '*']
            model = ListModel(data, 6, 1, rows, None)
            self.widget.tableView.setModel(model)

        elif type == "Array":
            self.widget.tableView.horizontalHeader().setHidden(False)
            self.widget.tableView.verticalHeader().setHidden(False)
            data = [
                [1, 9, 2],
                [1, 0, -1],
                [3, 5, 2],
                [3, 3, 2],
                [5, 8, 9],
            ]
            columns = ['α', 'β', 'γ']
            rows=['1', '2', '3', '4', '5']
            model = ListModel(data, 5, 3, rows, columns)
            self.widget.tableView.setModel(model)

        elif type == "Temperature Series":
            self.widget.tableView.horizontalHeader().setHidden(False)
            self.widget.tableView.verticalHeader().setHidden(True)

            data = [
                [10, 10],
                [15, 20],
                [30, 60],
                [35, 90],
                [50, 150],
            ]
            columns = ['Temperature', 'Value']
            rows=['1', '2', '3', '4', '5']
            model = ListModel(data, 5, 2, rows, columns)
            self.widget.tableView.setModel(model)

        elif type == "Pressure Series":
            self.widget.tableView.horizontalHeader().setHidden(False)
            self.widget.tableView.verticalHeader().setHidden(True)
            data = [
                [10, 10],
                [15, 20],
                [30, 60],
                [35, 90],
                [50, 150],
            ]
            columns = ['Pressure', 'Value']
            rows=['1', '2', '3', '4', '5']
            model = ListModel(data, 5, 2, rows, columns)
            self.widget.tableView.setModel(model)


    def setChart(self):
        # Creating QChart
        chart = QtCharts.QChart()
        chart.setAnimationOptions(QtCharts.QChart.NoAnimation)

        # Creating QChartView
        self.chartView = ChartView(chart)
        self.chartView.setRenderHint(QtGui.QPainter.Antialiasing)
        self.widget.graphLayout.addWidget(self.chartView)

        chart.removeAllSeries()

        # Create QLineSeries
        self.flutterSeries = QtCharts.QSplineSeries()
        self.flutterSeries.setName("Flutter")

        self.divergenceSeries = QtCharts.QSplineSeries()
        self.divergenceSeries.setName("Divergence")

        # Filling QSplineSeries
        max = 30

        for i in range(0, max+1):
            altitude = i * 1000000.0 # to mm

            # Getting the data
            x = math.exp(i)
            y = 0.2 * altitude

            self.flutterSeries.append(i, x)
            self.divergenceSeries.append(i, y)

        chart.addSeries(self.flutterSeries)
        chart.addSeries(self.divergenceSeries)

        # self._clearAllAxes()
        chart.createDefaultAxes()

        # Setting X-axis
        self.axis_x = chart.axes(QtCore.Qt.Horizontal)[0]
        self.axis_x.setTickCount(11) # 10 + 0th position
        self.axis_x.setLabelFormat("%.2f")
        self.axis_x.setTitleText("Altitude (km)")

        # Setting Y-axis
        self.axis_y = chart.axes(QtCore.Qt.Vertical)[0]
        self.axis_y.setTickCount(10)
        self.axis_y.setLabelFormat("%.2f")
        self.axis_y.setTitleText("Velocity (m/s)")


    def updateChart(self):
        pass

    def accept(self):
        ""

        self.storeSize()
        QtGui.QDialog.accept(self.widget)

    def reject(self):
        ""

        self.storeSize()
        QtGui.QDialog.reject(self.widget)

    def storeSize(self):
        "stores the widget size"
        # store widths
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material")
        p.SetInt("ValueEditorWidth", self.widget.width())
        p.SetInt("ValueEditorHeight", self.widget.height())

    def show(self):
        return self.widget.show()

    def exec_(self):
        return self.widget.exec_()

def openEditor():
    """openEditor([obj,prop]): opens the editor, optionally with
    an object name and material property name to edit"""
    editor = ValueEditor()
    editor.exec_()
