#/******************************************************************************
# *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
# *                                                                            *
# *   This file is part of the FreeCAD CAx development system.                 *
# *                                                                            *
# *   This library is free software; you can redistribute it and/or            *
# *   modify it under the terms of the GNU Library General Public              *
# *   License as published by the Free Software Foundation; either             *
# *   version 2 of the License, or (at your option) any later version.         *
# *                                                                            *
# *   This library  is distributed in the hope that it will be useful,         *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
# *   GNU Library General Public License for more details.                     *
# *                                                                            *
# *   You should have received a copy of the GNU Library General Public        *
# *   License along with this library; see the file COPYING.LIB. If not,       *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
# *   Suite 330, Boston, MA  02111-1307, USA                                   *
# *                                                                            *
# ******************************************************************************/

from PySide import QtCore, QtGui
import FreeCAD # Just for debug printing...

class WizardShaftTable:
    "The table widget that contains all the data of the shaft"
    # Dictionary to access different parameters without using a numeric row index that might change
    # as the code evolves
    rowDict = {
        "Length"        : 0,
        "Diameter"      : 1,
        "InnerDiameter" : 2,
        "ConstraintType"      : 3,
        "StartEdgeType" : 4,
        "StartEdgeSize" : 5,
        "EndEdgeType"   : 6,
        "EndEdgeSize"   : 7
    }
    rowDictReverse = {}
    headers = [
                "Length [mm]",
               "Diameter [mm]",
               "Inner diameter [mm]",
               "Constraint type",
               "Start edge type",
               "Start edge size",
               "End edge type",
               "End edge size"
              ]    

    def __init__(self, w, s):
        for key in iter(self.rowDict.keys()):
            self.rowDictReverse[self.rowDict[key]] = key
        # Set parent wizard (for connecting slots)
        self.wizard = w
        self.shaft = s
        # Create table widget
        self.widget = QtGui.QTableWidget(len(self.rowDict), 0)     
        self.widget.setObjectName("ShaftWizardTable") # Do not change or translate: Used in ViewProviderFemConstraintXXX
        self.widget.setWindowTitle("Shaft wizard")
        self.widget.resize(QtCore.QSize(300,200))
        self.editedRow = None
        self.editedColumn = None

        # Label rows and columns
        self.widget.setVerticalHeaderLabels(self.headers)
        self.widget.setHorizontalHeaderLabels(["Section 1", "Section 2"])
        #self.widget.columnMoved.connect(column, oldIndex, newIndex)

        # Create context menu
        action = QtGui.QAction("Add column", self.widget)
        action.triggered.connect(self.slotInsertColumn)
        self.widget.addAction(action)
        self.widget.setContextMenuPolicy(QtCore.Qt.ActionsContextMenu)
        self.widget.horizontalHeader().addAction(action)
        self.widget.horizontalHeader().setContextMenuPolicy(QtCore.Qt.ActionsContextMenu)

        # Set some default data
        # Section 1
        self.addColumn()
        self.setLength(0, 40.0)
        self.setDiameter(0, 50.0)
        self.setConstraintType(0, "Fixed")
        # Section 2
        self.addColumn()
        self.setLength(1, 80.0)
        self.setDiameter(1, 60.0)
        self.setConstraintType(1, "Force")

    def slotInsertColumn(self):
        # FIXME: Allow inserting columns, not just adding at the end
        # Note: need to re-name all the following column headers then
        # if (column == self.tableWidget.columnCount()):
        self.addColumn()
        # else:
        #   self.insertColumn(index)

    def addColumn(self):
        "Add a new column, fill it with widgets, and connect the signals"
        index = self.widget.columnCount()
        # Make an intelligent guess at the length/dia of the next segment
        if index > 0:
            length = self.shaft.segments[index-1].length            
            diameter = self.shaft.segments[index-1].diameter
            if index > 2:
                diameter -= 5.0
            else:
                diameter += 5.0
            innerdiameter = self.shaft.segments[index-1].innerdiameter
        else:
            length = 20.0
            diameter = 10.0
            innerdiameter = 0.0
        self.shaft.addSegment(length, diameter, innerdiameter)

        self.widget.insertColumn(index)
        self.widget.setHorizontalHeaderItem(index + 1, QtGui.QTableWidgetItem("Section %s" % (index + 1)))

        # Length
        widget = QtGui.QDoubleSpinBox(self.widget)
        widget.setMinimum(0)
        widget.setMaximum(1E9)
        self.widget.setCellWidget(self.rowDict["Length"], index, widget)
        widget.setValue(length)
        widget.valueChanged.connect(self.slotValueChanged)
        widget.editingFinished.connect(self.slotEditingFinished)
        # Diameter
        widget = QtGui.QDoubleSpinBox(self.widget)
        widget.setMinimum(0)
        widget.setMaximum(1E9)
        self.widget.setCellWidget(self.rowDict["Diameter"], index, widget)
        widget.setValue(diameter)
        widget.valueChanged.connect(self.slotValueChanged)
        widget.editingFinished.connect(self.slotEditingFinished)
        # inner Diameter
        widget = QtGui.QDoubleSpinBox(self.widget)
        widget.setMinimum(0)
        widget.setMaximum(1E9)
        self.widget.setCellWidget(self.rowDict["InnerDiameter"], index, widget)
        widget.setValue(innerdiameter)
        widget.valueChanged.connect(self.slotValueChanged)
        widget.editingFinished.connect(self.slotEditingFinished)
        # Constraint type
        widget = QtGui.QComboBox(self.widget)
        widget.insertItem(0, "None")
        widget.insertItem(1, "Fixed")
        widget.insertItem(2, "Force")
        widget.insertItem(3, "Bearing")
        widget.insertItem(4, "Gear")
        widget.insertItem(5, "Pulley")
        action = QtGui.QAction("Edit constraint", widget)
        action.triggered.connect(self.slotEditConstraint)
        widget.addAction(action)
        widget.setContextMenuPolicy(QtCore.Qt.ActionsContextMenu)
        self.widget.setCellWidget(self.rowDict["ConstraintType"], index, widget)
        widget.setCurrentIndex(0)
        self.widget.connect(widget, QtCore.SIGNAL("currentIndexChanged(const QString&)"), self.slotConstraintType)      
        # Start edge type
        widget = QtGui.QComboBox(self.widget)
        widget.insertItem(0, "None",)
        widget.insertItem(1, "Chamfer")
        widget.insertItem(2, "Fillet")
        self.widget.setCellWidget(self.rowDict["StartEdgeType"],index, widget)
        widget.setCurrentIndex(0)
        widget.setEnabled(False)
        #self.widget.connect(widget, QtCore.SIGNAL("currentIndexChanged(const QString&)"), self.slotLoadType)
        # Start edge size
        widget = QtGui.QDoubleSpinBox(self.widget)
        widget.setMinimum(0)
        widget.setMaximum(1E9)
        self.widget.setCellWidget(self.rowDict["StartEdgeSize"],index, widget)
        widget.setValue(1)
        widget.valueChanged.connect(self.slotValueChanged)
        widget.editingFinished.connect(self.slotEditingFinished)
        widget.setEnabled(False)
        # End edge type
        widget = QtGui.QComboBox(self.widget)
        widget.insertItem(0, "None",)
        widget.insertItem(1, "Chamfer")
        widget.insertItem(2, "Fillet")
        self.widget.setCellWidget(self.rowDict["EndEdgeType"],index, widget)
        widget.setCurrentIndex(0)
        widget.setEnabled(False)
        #self.widget.connect(widget, QtCore.SIGNAL("currentIndexChanged(const QString&)"), self.slotLoadType)
        # End edge size
        widget = QtGui.QDoubleSpinBox(self.widget)
        widget.setMinimum(0)
        widget.setMaximum(1E9)
        self.widget.setCellWidget(self.rowDict["EndEdgeSize"],index, widget)
        widget.setValue(1)
        widget.valueChanged.connect(self.slotValueChanged)
        widget.editingFinished.connect(self.slotEditingFinished)
        widget.setEnabled(False)

    def slotValueChanged(self, value):
        (self.editedRow, self.editedColumn) = self.getFocusedCell()
        self.editedValue = value

    def slotEditingFinished(self):
        if self.editedRow == None:
            return
        rowName = self.rowDictReverse[self.editedRow]
        if rowName is None:
            return
        if rowName == "Length":
            self.shaft.updateSegment(self.editedColumn, length = self.getDoubleValue(rowName, self.editedColumn))
        elif rowName == "Diameter":
            self.shaft.updateSegment(self.editedColumn, diameter = self.getDoubleValue(rowName, self.editedColumn))
        elif rowName == "InnerDiameter":
            self.shaft.updateSegment(self.editedColumn, innerdiameter = self.getDoubleValue(rowName, self.editedColumn))
        elif rowName == "Constraintype":
            self.shaft.updateConstraint(self.editedColumn, self.getListValue(rowName, self.editedColumn))
        elif rowName == "StartEdgeType":
            pass
        elif rowName == "StartEdgeSize":
            pass
        elif rowName == "EndEdgeType":
            pass
        elif rowName == "EndEdgeSize":
            pass

    def slotEditConstraint(self):        
        (self.editedRow, self.editedColumn) = self.getFocusedCell() # Because finishEditConstraint() will trigger slotEditingFinished() which requires this information
        self.shaft.editConstraint(self.editedColumn)
        
    def finishEditConstraint(self):
        self.shaft.updateConstraint(self.editedColumn, self.getConstraintType(self.editedColumn))
        
    def setLength(self, column, l):
        self.setDoubleValue("Length", column, l)
        self.shaft.updateSegment(column, length = l)

    def getLength(self, column):
        return self.getDoubleValue("Length", column)

    def setDiameter(self, column, d):
        self.setDoubleValue("Diameter", column, d)
        self.shaft.updateSegment(column, diameter = d)

    def getDiameter(self, column):
        return self.getDoubleValue("Diameter", column)

    def setInnerDiameter(self, column, d):
        self.setDoubleValue("InnerDiameter", column, d)
        self.shaft.updateSegment(column, innerdiameter = d)

    def getInnerDiameter(self, column):
        return self.getDoubleValue("InnerDiameter", column)

    @QtCore.Slot('QString')
    def slotConstraintType(self, text):
        self.shaft.updateConstraint(self.getFocusedColumn(), text)

    def setConstraintType(self, column, t):
        self.setListValue("ConstraintType", column, t)
        self.shaft.updateConstraint(column, t)

    def getConstraintType(self, column):
        return self.getListValue("ConstraintType", column)

    def slotStartEdgeType(self, old, new):
        pass

    def setStartEdgeType(self, column, t):
        self.setListValue("StartEdgeType", column, t)

    def getStartEdgeType(self, column):
        return self.getListValue("StartEdgeType", column)

    def setStartEdgeSize(self, column, s):
        self.setDoubleValue("StartEdgeSize", column, s)

    def getStartEdgeSize(self, column):
        return self.getDoubleValue("StartEdgeSize", column)

    def slotEndEdgeType(self, old, new):
        pass

    def setEndEdgeType(self, column, t):
        self.setListValue("EndEdgeType", column, t)

    def getEndEdgeType(self, column):
        return self.getListValue("EndEdgeType", column)

    def setEndEdgeSize(self, column, s):
        self.setDoubleValue("EndEdgeSize", column, s)

    def getEndEdgeSize(self, column):
        return self.getDoubleValue("EndEdgeSize", column)

    def setDoubleValue(self, row, column, v):
        widget = self.widget.cellWidget(self.rowDict[row], column)
        # Avoid triggering a signal, because the slot will work on the focused cell, not the current one
        widget.blockSignals(True)
        widget.setValue(v)
        widget.blockSignals(False)

    def getDoubleValue(self, row, column):
        widget = self.widget.cellWidget(self.rowDict[row], column)
        if widget is not None:
            return widget.value()
        else:
            return None

    def setListValue(self, row, column, v):
        widget = self.widget.cellWidget(self.rowDict[row], column)
        widget.blockSignals(True)
        widget.setCurrentIndex(widget.findText(v, QtCore.Qt.MatchExactly))
        widget.blockSignals(False)

    def getListValue(self, row, column):
        widget = self.widget.cellWidget(self.rowDict[row], column)
        if widget is not None:
            return widget.currentText() #[0].upper()
        else:
            return None

    def getFocusedColumn(self):
        # Make the focused cell also the current one in the table
        widget = QtGui.QApplication.focusWidget()
        if widget is not None:
            index = self.widget.indexAt(widget.pos())
            self.widget.setCurrentCell(index.row(), index.column())
        return self.widget.currentColumn()

    def getFocusedCell(self):
        # Make the focused cell also the current one in the table
        widget = QtGui.QApplication.focusWidget()
        if widget is not None:
            index = self.widget.indexAt(widget.pos())
            self.widget.setCurrentCell(index.row(), index.column())
        return (self.widget.currentRow(), self.widget.currentColumn())
