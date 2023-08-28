#/******************************************************************************
# *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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

import FreeCAD, FreeCADGui
import Part,  PartDesignGui
from PySide import QtCore, QtGui
import Standards
import os

class TaskHole:
    "Hole hole feature"
    types = ["Linear", "Coaxial"]
    typestr = ["Linear to two lines/planes", "Coaxial to a circle/cylinder"]

    def __init__(self, feature):
        self.form = None
        self.extraStandards = []
        self.feature = feature
        p=os.path.realpath(__file__)
        p=os.path.dirname(p)
        self.ui = os.path.join(p, "TaskHole.ui")

    def accept(self):
        self.feature.touch()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):
        if (self.feature is not None):
            self.hideFeature() # Show the support again
            document = self.feature.Document
            body = FreeCADGui.activeView().getActiveObject("pdbody");
            groove = self.feature.HoleGroove
            sketch = groove.Sketch
            plane = sketch.Support[0]
            axis = plane.References[0][0]
            body.removeObject(self.feature)
            document.removeObject(self.feature.Name)
            body.removeObject(groove)
            document.removeObject(groove.Name)
            body.removeObject(sketch)
            try:
                document.removeObject(sketch.Name)
            except Exception:
                pass # This always throws an exception: "Sketch support has been deleted" from SketchObject::execute()
            body.removeObject(plane)
            document.removeObject(plane.Name)
            body.removeObject(axis)
            document.removeObject(axis.Name)
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog(self)
        return True

    def isAllowedAlterDocument(self):
        return False

    def isAllowedAlterView(self):
        return False

    def isAllowedAlterSelection(self):
        return True

    def getMainWindow(self):
        "returns the main window"
        # using QtGui.QApplication.activeWindow() isn't very reliable because if another
        # widget than the mainwindow is active (e.g. a dialog) the wrong widget is
        # returned
        toplevel = QtGui.QApplication.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise Exception("No main window found")

    def setupUi(self):
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskHole")
        if form is None:
            return
        form.tabWidget = form.findChild(QtGui.QTabWidget, "tabWidget")
        # Type
        form.tabType = form.tabWidget.findChild(QtGui.QWidget, "tab_type")
        form.buttonThru = form.tabType.findChild(QtGui.QRadioButton, "buttonThru")
        form.buttonDepth = form.tabType.findChild(QtGui.QRadioButton, "buttonDepth")
        form.checkThreaded = form.tabType.findChild(QtGui.QCheckBox, "checkThreaded")
        form.checkCounterbore = form.tabType.findChild(QtGui.QCheckBox, "checkCounterbore")
        form.checkCountersink = form.tabType.findChild(QtGui.QCheckBox, "checkCountersink")
        # Norm
        form.tabNorm = form.tabWidget.findChild(QtGui.QWidget, "tab_norm")
        form.checkCustom = form.tabNorm.findChild(QtGui.QCheckBox, "checkCustom")
        form.comboNorm = form.tabNorm.findChild(QtGui.QComboBox, "comboNorm")
        for std in Standards.getStandards("through"):
            form.comboNorm.addItem(std)
        form.comboTolerance = form.tabNorm.findChild(QtGui.QComboBox, "comboTolerance")
        for tol in Standards.standards_tolerance:
            form.comboTolerance.addItem(tol)
        form.comboNormDia = form.tabNorm.findChild(QtGui.QComboBox, "comboNormDia")
        form.comboNormBoltWasher = form.tabNorm.findChild(QtGui.QComboBox,  "comboNormBoltWasher")
        # Thread
        form.tabThread = form.tabWidget.findChild(QtGui.QWidget,  "tab_thread")
        form.comboThreadNorm = form.tabThread.findChild(QtGui.QComboBox,  "comboThreadNorm")
        for std in Standards.getStandards("thread"):
            form.comboThreadNorm.addItem(std)
        form.comboThreadDia = form.tabThread.findChild(QtGui.QComboBox,  "comboThreadDia")
        form.checkCustomThreadLength = form.tabThread.findChild(QtGui.QCheckBox,  "checkCustomThreadLength")
        form.comboFinishNorm = form.tabThread.findChild(QtGui.QComboBox,  "comboFinishNorm")
        for std in Standards.getStandards("threaded"):
            form.comboFinishNorm.addItem(std)
        # Data
        form.tabData = form.tabWidget.findChild(QtGui.QWidget, "tab_data")
        form.spinDiameter = form.tabData.findChild(QtGui.QDoubleSpinBox, "spinDiameter")
        form.spinDepth = form.tabData.findChild(QtGui.QDoubleSpinBox, "spinDepth")
        form.spinCounterboreDiameter = form.tabData.findChild(QtGui.QDoubleSpinBox, "spinCounterboreDiameter")
        form.spinCounterboreDepth = form.tabData.findChild(QtGui.QDoubleSpinBox, "spinCounterboreDepth")
        form.spinCountersinkAngle = form.tabData.findChild(QtGui.QDoubleSpinBox, "spinCountersinkAngle")
        form.spinThreadLength = form.tabData.findChild(QtGui.QDoubleSpinBox,  "spinThreadLength")
        # Position
        form.tabPosition = form.tabWidget.findChild(QtGui.QWidget, "tab_position")
        form.comboType = form.tabPosition.findChild(QtGui.QComboBox, "comboType")
        for i in self.typestr:
            form.comboType.addItem(i)
        form.buttonSupport = form.tabPosition.findChild(QtGui.QPushButton, "buttonSupport")
        form.lineSupport = form.tabPosition.findChild(QtGui.QLineEdit, "lineSupport")
        form.buttonRef1 = form.tabPosition.findChild(QtGui.QPushButton, "buttonRef1")
        form.lineRef1 = form.tabPosition.findChild(QtGui.QLineEdit, "lineRef1")
        form.labelRef1 = form.tabPosition.findChild(QtGui.QLabel, "labelRef1")
        form.spinRef1 = form.tabPosition.findChild(QtGui.QDoubleSpinBox, "spinRef1")
        form.buttonRef2 = form.tabPosition.findChild(QtGui.QPushButton, "buttonRef2")
        form.lineRef2 = form.tabPosition.findChild(QtGui.QLineEdit, "lineRef2")
        form.labelRef2 = form.tabPosition.findChild(QtGui.QLabel, "labelRef2")
        form.spinRef2 = form.tabPosition.findChild(QtGui.QDoubleSpinBox, "spinRef2")
        self.form = form

        # Connect Signals and Slots
        # Type
        self.form.buttonThru.toggled.connect(self.buttonThru)
        self.form.buttonDepth.toggled.connect(self.buttonDepth)
        self.form.checkThreaded.toggled.connect(self.checkThreaded)
        self.form.checkCounterbore.toggled.connect(self.checkCounterbore)
        self.form.checkCountersink.toggled.connect(self.checkCountersink)
        # Norm
        self.form.checkCustom.toggled.connect(self.checkCustom)
        self.form.comboNorm.currentIndexChanged.connect(self.comboNorm)
        self.form.comboTolerance.currentIndexChanged.connect(self.comboTolerance)
        self.form.comboNormDia.currentIndexChanged.connect(self.comboNormDia)
        self.form.comboNormBoltWasher.currentIndexChanged.connect(self.comboNormBoltWasher)
        # Thread
        self.form.comboThreadNorm.currentIndexChanged.connect(self.comboThreadNorm)
        self.form.comboThreadDia.currentIndexChanged.connect(self.comboThreadDia)
        self.form.checkCustomThreadLength.toggled.connect(self.checkCustomThreadLength)
        self.form.comboFinishNorm.currentIndexChanged.connect(self.comboFinishNorm)
        # Data
        self.form.spinDiameter.valueChanged.connect(self.spinDiameter)
        self.form.spinDepth.valueChanged.connect(self.spinDepth)
        self.form.spinCounterboreDiameter.valueChanged.connect(self.spinCounterboreDiameter)
        self.form.spinCounterboreDepth.valueChanged.connect(self.spinCounterboreDepth)
        self.form.spinCountersinkAngle.valueChanged.connect(self.spinCountersinkAngle)
        self.form.spinThreadLength.valueChanged.connect(self.spinThreadLength)
        # Position
        self.form.comboType.currentIndexChanged.connect(self.comboType)
        self.form.buttonSupport.clicked.connect(self.buttonSupport)
        self.form.buttonRef1.clicked.connect(self.buttonRef1)
        self.form.spinRef1.valueChanged.connect(self.spinRef1)
        self.form.buttonRef2.clicked.connect(self.buttonRef2)
        self.form.spinRef2.valueChanged.connect(self.spinRef2)

        # Update the UI
        self.updateUI()
        return True

    def getRefText(self,  ref):
        (obj,  element) = ref
        if isinstance(element,  basestring):
            return obj.Name + ":" + element
        elif isinstance(element,  list):
            return obj.Name + ":" + element[0]
        else:
            return obj.Name

    def updateUI(self):
        # Type
        self.form.buttonThru.setChecked(self.feature.HoleType == "Thru")
        self.form.buttonDepth.setChecked(self.feature.HoleType == "Depth")
        self.form.checkThreaded.setChecked(self.feature.Threaded == True)
        self.form.checkCounterbore.setChecked(self.feature.Counterbore == True)
        self.form.checkCountersink.setChecked(self.feature.Countersink == True)
        # Norm
        if self.feature.Norm == "Custom":
            self.form.checkCustom.setChecked(True)
            self.form.comboNorm.setEnabled(False)
            self.form.comboTolerance.setEnabled(False)
            self.form.comboNormDia.setEnabled(False)
            self.form.comboNormBoltWasher.setEnabled(False)
        else:
            if self.feature.Counterbore == True:
                holetype = "counterbore"
            elif self.feature.Countersink == True:
                 holetype = "countersink"
            elif self.feature.Threaded == True:
                holetype = "threaded"
            else:
                holetype = "through"
            self.form.comboNorm.setEnabled(True)
            self.form.comboTolerance.setEnabled(True)
            self.form.comboNormDia.setEnabled(True)
            if holetype == "counterbore":
                self.form.comboNormBoltWasher.setEnabled(True)
            else:
                self.form.comboNormBoltWasher.setEnabled(False)
            # comboNorm
            standards = Standards.getStandards(holetype)
            self.form.comboNorm.blockSignals(True)
            self.form.comboNorm.clear()
            for std in standards:
                self.form.comboNorm.addItem(std)
            if not self.feature.Norm in standards:
                self.feature.Norm = standards[0]
            else:
                self.form.comboNorm.setCurrentIndex(standards.index(self.feature.Norm))
            self.form.comboNorm.blockSignals(False)
            # comboTolerance
            self.form.comboTolerance.blockSignals(True)
            self.form.comboTolerance.setCurrentIndex(Standards.standards_tolerance.index(self.feature.NormTolerance))
            self.form.comboTolerance.blockSignals(False)
            # comboNormDia
            diameters = sorted(Standards.getBaseDiameters(self.feature.Norm))
            self.form.comboNormDia.blockSignals(True)
            self.form.comboNormDia.clear()
            for dia in diameters:
                self.form.comboNormDia.addItem("M%g" % dia)
            if self.feature.NormDiameter in diameters:
                self.form.comboNormDia.setCurrentIndex(diameters.index(self.feature.NormDiameter))
            self.form.comboNormDia.blockSignals(False)
            # comboNormBoltWasher
            if holetype == "counterbore":
                rowStandards = sorted(Standards.getRowStandards(self.feature.Norm))
                self.form.comboNormBoltWasher.blockSignals(True)
                self.form.comboNormBoltWasher.clear()
                for std in rowStandards:
                    self.form.comboNormBoltWasher.addItem(std)
                if self.feature.ExtraNorm in rowStandards:
                    self.form.comboNormBoltWasher.setCurrentIndex(rowStandards.index(self.feature.ExtraNorm))
                self.form.comboNormBoltWasher.blockSignals(False)
            # Dependent values
            if holetype == "through":
                self.feature.Diameter = Standards.getThroughHoleDia(self.feature.Norm, self.feature.NormDiameter, self.feature.NormTolerance)
            elif holetype == "counterbore":
                throughStandard = Standards.getThroughHoleStandard(self.feature.Norm)
                self.feature.Diameter = Standards.getThroughHoleDia(throughStandard,  self.feature.NormDiameter,  self.feature.NormTolerance)
                self.feature.CounterboreDiameter = Standards.getCounterboreDia(self.feature.Norm,  self.feature.NormDiameter,  self.feature.ExtraNorm)
                # TODO: Calculate counter bore depth from standard for bolt and washer(s)
                # Requires accessing all the norms for bolts
                # self.feature.CounterboreDepth = calcCounterboreDepth(...)
            elif holetype == "countersink":
                throughStandard = Standards.getThroughHoleStandard(self.feature.Norm)
                self.feature.Diameter = Standards.getThroughHoleDia(throughStandard,  self.feature.NormDiameter,  self.feature.NormTolerance)
                self.feature.CounterboreDiameter = Standards.getCountersinkDia(self.feature.Norm,  self.feature.NormDiameter)
                self.feature.CountersinkAngle = Standards.getCountersinkAngle(self.feature.Norm,  self.feature.NormDiameter) / 2.0
        # Thread
        if self.feature.Threaded == True:
            if not self.feature.Counterbore and not self.feature.Countersink:
                self.form.comboTolerance.setEnabled(False)
            else:
                self.form.tabNorm.setEnabled(True)
            self.form.comboTolerance.setEnabled(False)
            self.form.tabThread.setEnabled(True)
            self.form.comboThreadNorm.blockSignals(True)
            standards = Standards.getStandards("thread")
            if not self.feature.NormThread in standards:
                self.feature.NormThread = standards[0]
            else:
                self.form.comboThreadNorm.setCurrentIndex(standards.index(self.feature.NormThread))
            self.form.comboThreadNorm.blockSignals(False)
            threadDiameters = sorted(Standards.getBaseDiameters(self.feature.NormThread))
            self.form.comboThreadDia.blockSignals(True)
            self.form.comboThreadDia.clear()
            for dia in threadDiameters:
                self.form.comboThreadDia.addItem("M%g" % dia)
            if self.feature.NormDiameter in threadDiameters:
                self.form.comboThreadDia.setCurrentIndex(threadDiameters.index(self.feature.NormDiameter))
            self.form.comboThreadDia.blockSignals(False)
            if self.feature.NormThreadFinish == "Custom":
                self.form.checkCustomThreadLength.setChecked(True)
                self.form.comboFinishNorm.setEnabled(False)
            else:
                self.form.checkCustomThreadLength.setChecked(False)
                self.form.comboFinishNorm.setEnabled(True)
                self.form.comboFinishNorm.blockSignals(True)
                standards = Standards.getStandards("threaded")
                if not self.feature.NormThreadFinish in standards:
                    self.feature.NormThreadFinish = standards[0]
                else:
                    self.form.comboFinishNorm.setCurrentIndex(standards.index(self.feature.NormThreadFinish))
                self.form.comboFinishNorm.blockSignals(False)
                flength = Standards.getThreadFinishLength(self.feature.NormThreadFinish,  self.feature.NormDiameter)
                tlength = self.feature.Depth - flength
                if tlength > 0:
                    self.feature.ThreadLength = tlength # TODO: Warning message
            # Dependents
            self.feature.Diameter = Standards.getThreadCoreDiameter(self.feature.NormThread,  self.feature.NormDiameter)
        else:
            self.form.tabThread.setEnabled(False)
            # Dependents
            self.form.spinDiameter.setEnabled(True)
        # Data
        self.form.spinDiameter.setValue(self.feature.Diameter)
        self.form.spinDepth.setValue(self.feature.Depth)
        if self.feature.HoleType == "Thru":
            self.form.spinDepth.setEnabled(False)
        else:
            self.form.spinDepth.setEnabled(True)
        if self.feature.Threaded == True:
            self.form.spinThreadLength.setEnabled(True)
        else:
            self.form.spinThreadLength.setEnabled(False)
        if self.feature.Counterbore == True:
            self.form.spinCounterboreDiameter.setEnabled(True)
            self.form.spinCounterboreDiameter.setValue(self.feature.CounterboreDiameter)
            self.form.spinCounterboreDepth.setEnabled(True)
            self.form.spinCounterboreDepth.setValue(self.feature.CounterboreDepth)
            self.form.spinCountersinkAngle.setEnabled(False)
        elif self.feature.Countersink == True:
            self.form.spinCounterboreDiameter.setEnabled(True)
            self.form.spinCounterboreDiameter.setValue(self.feature.CounterboreDiameter)
            self.form.spinCounterboreDepth.setEnabled(False)
            self.form.spinCountersinkAngle.setEnabled(True)
            self.form.spinCountersinkAngle.setValue(self.feature.CountersinkAngle)
        else:
            self.form.spinCounterboreDiameter.setEnabled(False)
            self.form.spinCounterboreDepth.setEnabled(False)
            self.form.spinCountersinkAngle.setEnabled(False)
        if self.feature.Norm == "Custom":
            self.form.spinDiameter.setEnabled(True)
        else:
            self.form.spinDiameter.setEnabled(False)
            if holetype == "counterbore":
                # Diameter is taken from Norm
                self.form.spinCounterboreDiameter.setEnabled(False)
            elif holetype == "countersink":
                # Values are taken from Norm
                self.form.spinCounterboreDiameter.setEnabled(False)
                self.form.spinCounterboreDepth.setEnabled(False)
                self.form.spinCountersinkAngle.setEnabled(False)
        if self.feature.Threaded == True:
            self.form.spinDiameter.setEnabled(False)
        if self.feature.NormThreadFinish != "Custom":
            self.form.spinThreadLength.setEnabled(False)
        self.form.spinThreadLength.setValue(self.feature.ThreadLength)
        # Position
        self.form.buttonSupport.setText("Face")
        if self.feature.Support is None:
            # First-time initialization
            selection = FreeCADGui.Selection.getSelectionEx()
            self.feature.Support = (selection[0].Object, selection[0].SubElementNames)
        self.form.lineSupport.setText(self.getRefText(self.feature.Support))
        if self.feature.PositionType == self.types[0]:
            # Linear
            self.form.buttonRef1.setText("Line/Plane")
            self.form.buttonRef1.setEnabled(True)
            self.form.buttonRef2.setText("Line/Plane")
            self.form.buttonRef2.setEnabled(True)
            self.form.lineRef1.setEnabled(True)
            self.form.lineRef2.setEnabled(True)
            self.form.labelRef1.setEnabled(True)
            self.form.labelRef1.setText("Distance")
            axis = self.feature.HoleGroove.Sketch.Support[0].References[0][0]
            if len(axis.References) > 0 and axis.References[0] is not None:
                if (len(axis.References) == 3):
                    self.form.lineRef1.setText(self.getRefText(axis.References[1]))
                else:
                    self.form.lineRef1.setText(self.getRefText(axis.References[0]))
            self.form.spinRef1.setEnabled(True)
            self.form.spinRef1.setValue(axis.Offset)
            self.form.labelRef2.setEnabled(True)
            self.form.labelRef2.setText("Distance")
            if len(axis.References) > 1 and axis.References[1] is not None:
                if (len(axis.References) == 3):
                    self.form.lineRef2.setText(self.getRefText(axis.References[2]))
                else:
                    self.form.lineRef2.setText(self.getRefText(axis.References[1]))
            self.form.spinRef2.setEnabled(True)
            self.form.spinRef2.setValue(axis.Offset2)
        elif self.feature.PositionType == self.types[1]:
            # Coaxial
            self.form.buttonRef1.setText("Circle/Cylinder")
            self.form.buttonRef1.setEnabled(True)
            self.form.buttonRef2.setEnabled(False)
            self.form.lineRef1.setEnabled(True)
            axis = self.feature.HoleGroove.Sketch.Support[0].References[0][0]
            if len(axis.References) > 0 and axis.References[0] is not None:
                self.form.lineRef1.setText(self.getRefText(axis.References[0]))
            self.form.lineRef2.setEnabled(False)
            self.form.labelRef1.setEnabled(False)
            self.form.spinRef1.setEnabled(False)
            self.form.labelRef2.setEnabled(False)
            self.form.spinRef2.setEnabled(False)
        else:
            # Nothing else defined yet
            pass

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)

    def accept(self):
        return True

    def buttonThru(self, toggle):
        if toggle == True:
            self.feature.HoleType = "Thru"

    def buttonDepth(self, toggle):
        if toggle == True:
            self.feature.HoleType = "Depth"

    def checkThreaded(self, checked):
        self.feature.Threaded = checked
        self.updateUI()

    def checkCounterbore(self, checked):
        if checked == True:
            self.feature.Countersink = False
        self.feature.Counterbore = checked
        self.updateUI()

    def checkCountersink(self, checked):
        if checked == True:
            self.feature.Counterbore = False
        self.feature.Countersink = checked
        self.updateUI()

    def checkCustom(self, checked):
        if checked == True:
            self.feature.Norm = "Custom"
        else:
            self.feature.Norm = str(self.form.comboNorm.currentText())
        self.updateUI()

    def comboNorm(self, index):
        self.feature.Norm = str(self.form.comboNorm.itemText(index))
        self.updateUI()

    def comboTolerance(self, index):
        self.feature.NormTolerance = str(self.form.comboTolerance.itemText(index))
        self.updateUI()

    def comboNormDia(self, index):
        diameter = str(self.form.comboNormDia.itemText(index))
        self.feature.NormDiameter = float(diameter[1:])
        self.updateUI()

    def comboNormBoltWasher(self,  index):
        self.feature.ExtraNorm = str(self.form.comboNormBoltWasher.itemText(index))
        self.updateUI()

    def comboThreadNorm(self,  index):
        self.feature.NormThread = str(self.form.comboThreadNorm.itemText(index))
        self.updateUI()

    def comboThreadDia(self,  index):
        diameter = str(self.form.comboThreadDia.itemText(index))
        self.feature.NormDiameter = float(diameter[1:])
        self.updateUI()

    def checkCustomThreadLength(self, checked):
        if checked == True:
            self.feature.NormThreadFinish = "Custom"
        else:
            self.feature.NormThreadFinish = str(self.form.comboFinishNorm.currentText())
        self.updateUI()

    def comboFinishNorm(self,  index):
        self.feature.NormThreadFinish = str(self.form.comboFinishNorm.itemText(index))
        self.updateUI()

    def spinDiameter(self, val):
        if (val > 0.0):
            self.feature.Diameter = val

    def spinDepth(self, val):
        if (val > 0.0):
            self.feature.Depth = val
        self.updateUI() # required to update the thread length

    def spinCounterboreDiameter(self, val):
        if (val > self.feature.Diameter):
            self.feature.CounterboreDiameter = val

    def spinCounterboreDepth(self, val):
        if (val > 0.0):
            self.feature.CounterboreDepth = val

    def spinCountersinkAngle(self, val):
        if (val > 0.0):
            self.feature.CountersinkAngle = val

    def spinThreadLength(self,  val):
        if (val > 0.0):
            self.feature.ThreadLength = val

    def comboType(self, index):
        self.feature.PositionType = self.types[index]
        self.updateUI()

    def addSelection(self, document, obj, element, position):
        #FreeCAD.Console.PrintMessage("AddSelection() for " + document + "." + obj + "." + element + "\n")
        # TODO: What is the position parameter?
        if document == self.feature.Document.Name:
            axis = self.feature.HoleGroove.Sketch.Support[0].References[0][0]
            refs = axis.References
            feature = eval("FreeCAD.getDocument('" + document + "')." + obj)
            shape = eval("feature.Shape." + element)
            if self.selectionMode == "Plane":
                if shape.Surface.__class__ != Part.Plane:
                    FreeCAD.Console.PrintMessage("Selected face must be planar\n")
                    return
                if self.feature.PositionType == self.types[0]:
                    # The Hole support is also the first reference of the sketch axis in Linear mode with edges selected
                    if len(refs) == 3:
                        refs[0] = (feature, element)
                        axis.References = refs
                self.feature.Support = (feature, [element])
            elif self.selectionMode == "LinearReference":
                if shape.ShapeType == "Edge":
                    if shape.Curve.__class__ != Part.LineSegment:
                        FreeCAD.Console.PrintMessage("Selected edge must be linear\n")
                        return
                    if len(refs) > 1:
                        refs[1] = (feature, element)
                    else:
                        refs.append((feature, element))
                elif shape.ShapeType == "Face":
                    if shape.Surface.__class__ != Part.Plane:
                        FreeCAD.Console.PrintMessage("Selected face must be planar\n")
                        return
                    if len(refs) > 0:
                        if len(refs) > 2:
                            refs = [(feature, element)]
                        else:
                            refs[0] = (feature, element)
                    else:
                        refs = [(feature, element)]
                else:
                    FreeCAD.Console.PrintMessage("Wrong shape type selected\n")
                    return
                axis.References = refs
                axis.Document.recompute()
            elif self.selectionMode == "LinearReference2":
                if shape.ShapeType == "Edge":
                    if shape.Curve.__class__ != Part.LineSegment:
                        FreeCAD.Console.PrintMessage("Selected edge must be linear\n")
                        return
                    if len(refs) > 2:
                        refs[2] = (feature, element)
                    else:
                        refs.append((feature, element))
                elif shape.ShapeType == "Face":
                    if shape.Surface.__class__ != Part.Plane:
                        FreeCAD.Console.PrintMessage("Selected face must be planar\n")
                        return
                    if len(refs) > 1:
                        if len(refs) > 2:
                            del refs[2]
                        refs[1] = (feature, element)
                    else:
                        refs.append((feature, element))
                else:
                    FreeCAD.Console.PrintMessage("Wrong shape type selected\n")
                    return
                axis.References = refs
                axis.Document.recompute()
            elif self.selectionMode == "CircularReference":
                if shape.ShapeType == "Edge":
                    if shape.Curve.__class__ != Part.Circle:
                        FreeCAD.Console.PrintMessage("Selected edge must be arc or circle\n")
                        return
                elif shape.ShapeType == "Face":
                    if shape.Surface.__class__ != Part.Cylinder:
                        FreeCAD.Console.PrintMessage("Selected face must be cylindrical\n")
                        return
                else:
                    FreeCAD.Console.PrintMessage("Wrong shape type selected\n")
                    return
                refs = [(feature, element)]
                axis.References = refs
                axis.Document.recompute()
            else:
                FreeCAD.Console.PrintMessage("Unknown selection mode: " + self.selectionMode + "\n")
                self.selectionMode = ""
                return

            FreeCADGui.Selection.removeObserver(self)
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.removeSelectionGate()
            self.selectionMode = ""
            self.updateUI()
            self.showFeature()

    def hideFeature(self):
        # Make sure selection takes place on support, not on hole feature
        if self.feature.Support is not None:
            FreeCADGui.ActiveDocument.hide(self.feature.Name)
            (support, elements) = self.feature.Support
            FreeCADGui.ActiveDocument.show(support.Name)

    def showFeature(self):
        if self.feature.Support is not None:
            FreeCADGui.ActiveDocument.show(self.feature.Name)
            (support, elements) = self.feature.Support
            FreeCADGui.ActiveDocument.hide(support.Name)

    def buttonSupport(self):
        FreeCADGui.Selection.addSelectionGate("SELECT Part::Feature SUBELEMENT Face COUNT 1")
        FreeCADGui.Selection.addObserver(self)
        # Currently support must be a planar face (but could also be a point or a construction plane in the future)
        self.selectionMode = "Plane"
        self.hideFeature()

    def buttonRef1(self):
        FreeCADGui.Selection.addSelectionGate("SELECT Part::Feature SUBELEMENT Edge COUNT 1 SELECT Part::Feature SUBELEMENT Face COUNT 1")
        FreeCADGui.Selection.addObserver(self)
        if self.feature.PositionType == self.types[0]:
            self.selectionMode = "LinearReference"
        elif self.feature.PositionType == self.types[1]:
            self.selectionMode = "CircularReference"
        self.hideFeature()

    def buttonRef2(self):
        FreeCADGui.Selection.addSelectionGate("SELECT Part::Feature SUBELEMENT Edge COUNT 1 SELECT Part::Feature SUBELEMENT Face COUNT 1")
        FreeCADGui.Selection.addObserver(self)
        self.selectionMode = "LinearReference2"
        self.hideFeature()

    def spinRef1(self, val):
        axis = self.feature.HoleGroove.Sketch.Support[0].References[0][0]
        axis.Offset = val
        axis.Document.recompute()

    def spinRef2(self, val):
        axis = self.feature.HoleGroove.Sketch.Support[0].References[0][0]
        axis.Offset2 = val
        axis.Document.recompute()
