# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 Russell Johnson (russ4262) <russ4262@gmail.com>    *
# *   Copyright (c) 2021 Markus Lampert (mlampert)                          *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__Title__   = "Tile Job Utility"
__Author__  = "Russell Johnson (russ4262); Markus Lampert (mlampert)"
__Version__ = "6.1"
__Date__    = "2021-05-17"
__Comment__ = "This macro takes the operations within a selected Path job and \
               creates a set of gcode tile objects, placing the code for each \
               operation within each tile into a Custom gcode object in a new job."
__Usage__   = "Prepare the target job in the object tree by ensuring only the desired \
               operations are active.\
               GUI Use:\
                   Start the GUI utility.  Select the desired job \
                   from the list.  Set the X Shift and Y Shift values accordingly. \
                   A zero value in either will assign the full job stock length \
                   of that axis to the tile dimensions.  A new job object will contain \
                   all the CustomTile operation objects with labels for organization. \
               CL Use:\
                   Import this module.  Create an instance of the TileJob class. \
                   Call the execute() method on the instance object, passing three \
                   arguments: source job, X Shift, Y Shift. \
               This new job is ready for simulation."

import math
import time
from pivy import coin
import FreeCAD
import PySide
import Path
import Part
import PathScripts.PathLog as PathLog
import PathScripts.PathDressupPathBoundary as BoundaryDressup
import PathScripts.PathStock as PathStock
import PathScripts.PathCustom as PathCustom
import PathScripts.PathJob as PathJob
import PathScripts.PathToolController as PathToolController
# import PathScripts.PathToolBit as PathToolBit

if FreeCAD.GuiUp:
    import FreeCADGui
    import PathScripts.PathJobGui as PathJobGui
    import PathScripts.PathCustomGui as PathCustomGui
    import PathScripts.PathOpGui as PathOpGui


# Qt translation handling
def translate(context, text, disambig=None):
    return PySide.QtCore.QCoreApplication.translate(context, text, disambig)


class TileJobGui():
    """class TileJobGui...
    This class provides a GUI frontend to the primary 'TileJob' class backend,
    also located within this `PathTileJob` module.  This class contains a method
    to create the popup window with user inputs, as well as methods to provide
    visualizations of the user-requested tiles for the utility operation.
    """

    def __init__(self):
        self._initiateVariables()

    # Private methods
    def _initiateVariables(self):
        """_initiateVariables() ...
        This method initiates and resets instance variables."""
        # Reset instance variables
        self.dialog = None
        self.setJobIdx = 0
        self.xLen = None
        self.yLen = None
        self.jobs = list()
        self.tileJobs = None
        self.unitLength = ''
        self.visuals = list()
        self.switch = None
        self.horizTileCnt = 1
        self.vertTileCnt = 1

    def _makePopupWindow(self):
        """_makePopupWindow() ...
        This method issues instructions to build and create the Qt popup window with user inputs."""
        firstJobBB = None

        # Make dialog box and get the scale size
        winMsg = translate('PathTileJob', 'Tile Job Utility')
        self.dialog = PySide.QtGui.QDialog()
        self.dialog.resize(350,100)
        self.dialog.setWindowTitle(winMsg)
        mainLayout = PySide.QtGui.QVBoxLayout(self.dialog)
        formLayout = PySide.QtGui.QFormLayout()
        jobMsg = translate('PathTileJob', 'Job')
        jobSelect_label = PySide.QtGui.QLabel(jobMsg + ": ")

        # Make Job selection combobox
        self.jobSelect = PySide.QtGui.QComboBox()
        jobName = ''
        guiSelection = FreeCADGui.Selection.getSelectionEx()
        if guiSelection:  #  Identify job selected by user
            sel = guiSelection[0]
            if hasattr(sel.Object, "Proxy") and isinstance(sel.Object.Proxy, PathJob.ObjectJob):
                jobName = sel.Object.Name
                FreeCADGui.Selection.clearSelection()
        jIdx = 0
        jCnt = len(self.jobs)
        for j in self.jobs:
            sBB = j.Stock.Shape.BoundBox
            if j.Name == jobName or jCnt == 1:
                self.setJobIdx = jIdx
                firstJobBB = sBB
            if not firstJobBB:
                firstJobBB = sBB
            x = FreeCAD.Units.Quantity(sBB.XLength, FreeCAD.Units.Length).getUserPreferred()[0]
            y = FreeCAD.Units.Quantity(sBB.YLength, FreeCAD.Units.Length).getUserPreferred()[0]
            entry = "{}  :  {} x {}".format(j.Label, x, y)
            self.jobSelect.addItem(entry)
            jIdx += 1
        if jobName or jCnt == 1:
            # Pre-select GUI-selected job in the now disabled combobox
            self.jobSelect.setCurrentIndex(self.setJobIdx)
            self.jobSelect.setEnabled(False)
        formLayout.addRow(jobSelect_label, self.jobSelect)

        # Add double spinbox inputs for X and Y dimensions of tiles
        xMsg = translate('PathTileJob', 'X tile length')
        yMsg = translate('PathTileJob', 'Y tile length')
        zeroMsg = translate('PathTileJob', 'A zero value will indicate full length for the indicated axis.')
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
        precision = params.GetInt('Decimals') # returns an int
        # qnty = FreeCAD.Units.Quantity("500 mm")
        qnty = FreeCAD.Units.Quantity("{} mm".format(math.ceil(firstJobBB.XLength)))
        xLen_label = PySide.QtGui.QLabel(xMsg + " ({}): ".format(self.unitLength))
        yLen_label = PySide.QtGui.QLabel(yMsg + " ({}): ".format(self.unitLength))
        zeroValue_label = PySide.QtGui.QLabel(zeroMsg)
        self.xLen = PySide.QtGui.QDoubleSpinBox()  # QtGui.QLineEdit()
        self.xLen.setDecimals(precision)
        self.xLen.setMinimum(0.0)
        self.xLen.setMaximum(1000000.0)
        self.xLen.setValue(qnty.getValueAs(self.unitLength))  # set value after declaring min and max values
        self.yLen = PySide.QtGui.QDoubleSpinBox()  # QtGui.QLineEdit()
        self.yLen.setDecimals(precision)
        self.yLen.setMinimum(0.0)
        self.yLen.setMaximum(1000000.0)

        formLayout.addRow(xLen_label, self.xLen)
        formLayout.addRow(yLen_label, self.yLen)
        formLayout.addRow(zeroValue_label)

        # Add processing direction change toggle
        prcsMsg = translate('PathTileJob', 'Process Y axis first')
        self.yFirst = PySide.QtGui.QCheckBox(prcsMsg)
        self.yFirst.setCheckable(True)
        formLayout.addRow(self.yFirst)

        # Add processing direction change toggle
        crtMsg = translate('PathTileJob', 'Create one job per tile')
        self.jobPerTile = PySide.QtGui.QCheckBox(crtMsg)
        self.jobPerTile.setCheckable(True)
        formLayout.addRow(self.jobPerTile)

        # Add OK / Cancel buttons
        okbox = PySide.QtGui.QDialogButtonBox(self.dialog)
        okbox.setOrientation(PySide.QtCore.Qt.Horizontal)
        okbox.setStandardButtons(PySide.QtGui.QDialogButtonBox.Cancel|PySide.QtGui.QDialogButtonBox.Ok)
        mainLayout.addLayout(formLayout)
        mainLayout.addWidget(okbox)

        # Connect slot signals to internal methods
        okbox.accepted.connect(self._proceed)
        okbox.rejected.connect(self._closeWindow)
        self.jobSelect.currentIndexChanged.connect(self._visualizeTiles)
        self.xLen.valueChanged.connect(self._visualizeTiles)
        self.yLen.valueChanged.connect(self._visualizeTiles)
        self.dialog.rejected.connect(self._closeWindow)

    def _proceed(self):
        """_proceed() ...
        This method initiates the execution of the primary tiling operation. It is triggered
        when the user clicks the OK button."""
        xQty = FreeCAD.Units.Quantity("{} {}".format(self.xLen.value(), self.unitLength))
        yQty = FreeCAD.Units.Quantity("{} {}".format(self.yLen.value(), self.unitLength))
        xLen = xQty.getValueAs('mm')
        yLen = yQty.getValueAs('mm')

        # Get gob object per user input
        jobIdx = self.jobSelect.currentIndex()
        job = self.jobs[jobIdx]

        self._closeWindow()    # close the window

        # Tile the selected job
        tj = TileJob()
        tj.yFirst = self.yFirst.isChecked()
        tj.jobPerTile = self.jobPerTile.isChecked()
        self.tileJobs = tj.execute(job, xLen, yLen)

    def _closeWindow(self):
        """_closeWindow() ...
        Close the popup window created and reset instance variables. It is triggered
        when the user clicks the Cancel button."""
        if self.dialog:
            # self.dialog.hide()
            self.dialog.close()
        for __, v in self.visuals:
            self.switch.removeChild(v.root)
        self._initiateVariables()

    def _visualizeTiles(self):
        """_visualizeTiles() ...
        Internal method triggered with Qt signals from GUI popup window interactions.
        This method initiates the process of creating or updating the preview tiles
        in the viewport."""
        PathLog.debug("_visualizeTiles()")

        # Clear current visuals
        if self.visuals:
            for __, v in self.visuals:
                self.switch.removeChild(v.root)

        # Get gob object per user input
        jobIdx = self.jobSelect.currentIndex()
        job = self.jobs[jobIdx]
        stockBB = job.Stock.Shape.BoundBox

        # Get X and Y values
        xQty = FreeCAD.Units.Quantity("{} {}".format(self.xLen.value(), self.unitLength))
        yQty = FreeCAD.Units.Quantity("{} {}".format(self.yLen.value(), self.unitLength))
        xLen = xQty.getValueAs('mm')
        yLen = yQty.getValueAs('mm')
        if xLen == 0.0:
            xLen = stockBB.XLength
        if yLen == 0.0:
            yLen = stockBB.YLength
        # Calculate horizontal and vertical tile counts
        if xLen > 0:
            self.horizTileCnt = math.ceil(stockBB.XLength / xLen)
        if yLen > 0:
            self.vertTileCnt = math.ceil(stockBB.YLength / yLen)

        xmin = stockBB.XMin
        ymin = stockBB.YMin
        z = stockBB.ZMax

        self._makeAllVisuals(xmin, ymin, xLen, yLen, z)

    def _makeAllVisuals(self, xmin, ymin, xLen, yLen, z):
        """_makeAllVisuals() ...
        Primary control method to make visual representations of the tiles within the viewport."""
        PathLog.debug("_makeAllVisuals()")
        xShift = 0.0
        yShift = 0.0

        def _makeVisualWire(xmin, ymin, xLen, yLen, z):
            a = Part.makeLine(FreeCAD.Vector(xmin, ymin, z),
                            FreeCAD.Vector(xmin + xLen, ymin, z))
            b = Part.makeLine(FreeCAD.Vector(xmin + xLen, ymin, z),
                            FreeCAD.Vector(xmin + xLen, ymin + yLen, z))
            c = Part.makeLine(FreeCAD.Vector(xmin + xLen, ymin + yLen, z),
                            FreeCAD.Vector(xmin, ymin + yLen, z))
            d = Part.makeLine(FreeCAD.Vector(xmin, ymin + yLen, z),
                            FreeCAD.Vector(xmin, ymin, z))
            return Part.Wire([a, b, c, d])

        def _makeWireLabel(xLen, yLen):
            # Make label for visual
            xStr = str(math.floor(xLen * 1000.0) / 1000.0)
            yStr = str(math.floor(yLen * 1000.0) / 1000.0)
            return "x{}_y{}".format(xStr, yStr)

        def tileColor(hi, vi):
            if vi % 2 == 0:  # even row
                if hi % 2 == 0:  # even column
                    return 1
                return 0
            else:  # odd row
                if hi % 2 == 0:  # even column
                    return 0
                return 1

        if self.yFirst:
            for hi in range(0, self.horizTileCnt):
                xShift = hi * (xLen)
                for vi in range(0, self.vertTileCnt):
                    yShift = vi * (yLen)
                    x = xmin + xShift
                    y = ymin + yShift
                    wire = _makeVisualWire(x, y, xLen, yLen, z)
                    label = _makeWireLabel(xLen, yLen)
                    visTile = self._makeVisualTile(wire, label, tileColor(vi, hi))
                    self.visuals.append((label, visTile))
        else:
            for vi in range(0, self.vertTileCnt):
                yShift = vi * (yLen)
                for hi in range(0, self.horizTileCnt):
                    xShift = hi * (xLen)
                    x = xmin + xShift
                    y = ymin + yShift
                    wire = _makeVisualWire(x, y, xLen, yLen, z)
                    label = _makeWireLabel(xLen, yLen)
                    visTile = self._makeVisualTile(wire, label, tileColor(vi, hi))
                    self.visuals.append((label, visTile))

    def _makeVisualTile(self, wire, label, color):
        """_makeVisualTile() ...
        Create and return Coin3D object representation for viewport."""
        viewShape = ViewportShape(wire, color)
        self.switch.addChild(viewShape.root)
        return viewShape

    # Public method
    def execute(self):
        """execute() ...
        Public method called to begin GUI-based Tile Job Utility."""
        if not FreeCAD.GuiUp:
            msg = translate('PathTileJob',
                            'FreeCAD GUI not active. Use command line access with TileJob class.')
            PathLog.error(msg)
            return

        # Reset instance variables
        self._initiateVariables()
        # Get reference to current doccument's scenegraph for tile previews with Coin3D
        self.switch = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()

        # Get user preferred unit of length
        self.unitLength = FreeCAD.Units.Quantity(100, FreeCAD.Units.Length).getUserPreferred()[2]

        # Get current jobs in active document
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    self.jobs.append(o)

        if not self.jobs:
            msg = translate('PathTileJob',
                            'No jobs available in active document.')
            PathLog.error(msg)
            return
        
        self._makePopupWindow()

        # Load initial tile preview
        self._visualizeTiles()

        # display popup window
        self.dialog.show()
        self.dialog.exec_()
# Eclass


class TileJob():
    """TileJob() class...
    This class performs a job tiling operation when provided a source Job object,
    X Shift value, and Y Shift value.  The new TileJob is returned from the puclic
    `execute(job, xLen, yLen)` method.
    If you wish to change the tile process direction, set the `yFirst` attribute
    of your class instance object to True before calling `execute()` method.
    If you want to create on job object per tile, then set the `jobPertile` attribute
    of your class instance object to True before calling `execute()` method.
    """

    def __init__(self):
        self.yFirst = False
        self.jobPerTile = False
        self.useJobGui = False  # FreeCAD version 0.19 does not allow for use of this flag set to True
        self.isMacro = False  # Remove with references when integrated
        self._resetInstanceVariables()

    # Private methods
    def _resetInstanceVariables(self):
        """_resetInstanceVariables() ...
        Reset class instance variables."""
        self.job = None
        self.xLen = 100.0
        self.yLen = 100.0
        self.newJobs = list()
        self.tileStock = None
        self.horizTileCnt = 1
        self.vertTileCnt = 1
        self.opList = list()
        self.clearanceHeight = 0.0
        self.stockBB = None
        self.stockOrigin = None
        self.tileShiftList = list()
        self.tileIdList = list()
        self.stockNames = list()
        self.boundaryAdjustment = 0.0
        self.tileDataTups = list()
        self.tempOp = None
        self.success = False
        self.tileCount = 0

    def _initializeVariables(self, job, xLen, yLen):
        """_initializeVariables() ...
        Initialize class instance variables."""
        self.job = job
        self.stockBB = job.Stock.Shape.BoundBox
        self.stockOrigin = FreeCAD.Vector(self.stockBB.XMin, self.stockBB.YMin, self.stockBB.ZMin)
        msg = translate('PathTileJob',
                        'Length must be zero or greater. Setting to 500 mm.')

        if xLen < 0.0:
            self.xLen = 500.0
            PathLog.error("X " + msg)
            return False
        elif xLen == 0.0:
            self.xLen = 0.0
        else:
            self.xLen = xLen

        if yLen < 0.0:
            self.yLen = 500.0
            PathLog.error("Y " + msg)
            return False
        elif yLen == 0.0:
            self.yLen = 0.0
        else:
            self.yLen = yLen

        sBB = job.Stock.Shape.BoundBox
        if xLen > 0:
            self.horizTileCnt = math.ceil(sBB.XLength / xLen)
        if yLen > 0:
            self.vertTileCnt = math.ceil(sBB.YLength / yLen)

        return True

    def _getOpList(self):
        """_getOpList() ...
        Identify the list of operations within the selected job
        to be included in the tiling operation."""
        ops = self.job.Operations.Group

        def _adjustBoundary(ba, op):
            offset = abs(op.OffsetExtra.Value)
            toolDiam = op.ToolController.Tool.Diameter.Value * 1.1
            return max(offset + toolDiam, ba)

        for op in ops:
            if (hasattr(op, 'Base') and
                isinstance(op.Base, list) and
                op.isDerivedFrom('Path::Feature') and
                op.Active):
                self.opList.append(op)
                if op.Name[:7] == "Profile":
                    self.boundaryAdjustment = _adjustBoundary(self.boundaryAdjustment, op)
                self.clearanceHeight = max(self.clearanceHeight, op.ClearanceHeight.Value)

    def _makeTiles(self):
        """_makeTiles() ...
        Primary control method to create the custom tile operations that contain
        the gcode for each operation within each tile.  This method controls
        the grid order for the tiling operation.  The model is shifted into position
        for each tile.  The actual tile region to be sampled by the PathDressupBoundary
        operation is fixed at the stock origin and sized to the user-provided tile size."""
        PathLog.debug("_makeTiles()")
        xShift = 0.0
        yShift = 0.0

        if self.xLen == 0.0:
            xLen = self.stockBB.XLength * 1.01
        else:
            xLen = self.xLen
        if self.yLen == 0.0:
            yLen = self.stockBB.YLength * 1.01
        else:
            yLen = self.yLen

        if self.yFirst:
            for hi in range(0, self.horizTileCnt):
                cont = False
                xShift = hi * (self.xLen)
                for vi in range(0, self.vertTileCnt):
                    yShift = vi * (self.yLen)
                    # apply boundary dressup to get tile path commands
                    baseShift = FreeCAD.Vector(xShift, yShift, 0.0)
                    cont = self._applyDressups(baseShift, hi, vi, xLen, yLen)
                    if not cont:
                        break
                    else:
                        self.tileCount +=1
                if not cont:
                    return False
        else:
            for vi in range(0, self.vertTileCnt):
                cont = False
                yShift = vi * (self.yLen)
                for hi in range(0, self.horizTileCnt):
                    xShift = hi * (self.xLen)
                    # apply boundary dressup to get tile path commands
                    baseShift = FreeCAD.Vector(xShift, yShift, 0.0)
                    cont = self._applyDressups(baseShift, hi, vi, xLen, yLen)
                    if not cont:
                        break
                    else:
                        self.tileCount +=1
                if not cont:
                    return False

        return True

    def _applyDressups(self, shiftVector, hi, vi, xLen, yLen):
        """_applyDressups(shiftVector, vhIdxTup) ...
        This method applies a PathDressupBoundary operation with the user-provided
        tile dimensions at the designated grid location.  The dressup and other
        details are saved to a class variable for use in the saving method."""

        lastVert = self.vertTileCnt - 1
        lastHoriz = self.horizTileCnt - 1
        stockBase = self.stockOrigin

        xDim = xLen
        yDim = yLen

        # make boundary adjustments for dressup - some operations cut outside stock
        if vi == 0:
            yDim += self.boundaryAdjustment  # extra length for bottom of boundary
            # Shift stock base down to account for extra bottom length
            stockBase = stockBase.sub(FreeCAD.Vector(0.0, self.boundaryAdjustment, 0.0))
        if vi == lastVert:
            yDim += self.boundaryAdjustment  # extra length for top of boundary
        if hi == 0:
            xDim += self.boundaryAdjustment  # extra length for left of boundary
            # Shift stock base left to account for extra bottom length
            stockBase = stockBase.sub(FreeCAD.Vector(self.boundaryAdjustment, 0.0, 0.0))
        if hi == lastHoriz:
            xDim += self.boundaryAdjustment  # extra length for right of boundary

        dimensions = FreeCAD.Vector(xDim, yDim, self.clearanceHeight)  # self.stockBB.ZLength)


        opId = 0
        if self.isMacro:
            place = FreeCAD.Placement()
            place.Rotation = self.job.Stock.Placement.Rotation
            place.Base = stockBase.add(shiftVector)
            stock = PathStock.CreateBox(self.job, dimensions, placement=place)
            if not stock:
                return False
            for op in self.opList:
                opId += 1
                obj = BoundaryDressup.Create(op)
                self.stockNames.append(obj.Stock.Name)
                # Replace dressup stock with simple box
                obj.Stock = stock
                self.stockNames.append(obj.Stock.Name)
                obj.recompute()
                dataTup = (obj, hi, vi, shiftVector, op.ToolController.Label, self.tileCount, opId, op.Label)
                self.tileDataTups.append(dataTup)
        else:
            stockShape = Part.makeBox(xDim, yDim, self.clearanceHeight, stockBase.add(shiftVector))
            for op in self.opList:
                opId += 1
                pb = BoundaryDressup.PathBoundary(op, stockShape, inside=True)
                path = pb.execute()
                obj = Object(op, path)
                dataTup = (obj, hi, vi, shiftVector, op.ToolController.Label, self.tileCount, opId, op.Label)
                self.tileDataTups.append(dataTup)

        return True

    def _makeTileJobStock(self):
        """_makeTileJobStock() ...
        Create a custom stock shape using the tile dimensions."""
        PathLog.debug("_makeTileJobStock()")
        xLen = self.xLen
        yLen = self.yLen
        place = FreeCAD.Placement()
        place.Rotation = self.job.Stock.Placement.Rotation
        place.Base = self.stockOrigin

        if xLen == 0.0:
            xLen = self.stockBB.XLength * 1.01
        else:
            xLen = xLen
        if yLen == 0.0:
            yLen = self.stockBB.YLength * 1.01
        else:
            yLen = yLen
        dimensions = FreeCAD.Vector(xLen, yLen, self.stockBB.ZLength)  # self.stockBB.ZLength)
        return PathStock.CreateBox(self.job, dimensions, placement=place)

    def _createTileJob(self, tileStock):
        """_createTileJob(tileStock)...
        Create the new job container for the custom tile operations"""
        PathLog.debug("_createTileJob()")

        base = self.job.Model.Group
        if FreeCAD.GuiUp and not self.isMacro:
            PathJobGui.createWithTaskPanel = False
            newJob = PathJobGui.Create(base, template=None, useGui=self.useJobGui)
        else:
            newJob = PathJob.Create("Job", base, templateFile=None)
        newJob.Label = "Tile_" + self.job.Label

        # Add temp non-GUI operation to pacify need for tool controller selection with first operation.
        if not self.tempOp:
            self.tempOp = self._createCustomOperation(parentJob=newJob)

        # Swap regular stock with stock based on tile size
        # self._makeTileJobStock()
        origStock = newJob.Stock
        newJob.Stock = tileStock
        if FreeCAD.GuiUp:
            tileStock.Visibility = False
        
        # Remove regular stock clone
        FreeCAD.ActiveDocument.removeObject(origStock.Name)

        newJob.recompute()

        # Copy tool controllers
        self._copyToolControllers(newJob)

        return newJob

    def _createCustomOperation(self, parentJob=None):
        """_createCustomOperation(parentJob=None) ...
        This method returns a PathCustom operation with additional properties
        to contain details about tile location, size, and order."""
        PathLog.debug("_createCustomOperation()")
        if FreeCAD.GuiUp:
            res = PathOpGui.CommandResources(
                    'Custom',
                    PathCustom.Create,
                    PathCustomGui.TaskPanelOpPage,
                    'Path_Custom',
                    PySide.QtCore.QT_TRANSLATE_NOOP("Path_Custom", "Custom"),
                    PySide.QtCore.QT_TRANSLATE_NOOP("Path_Custom", "Create custom gcode snippet"),
                    PathCustom.SetupProperties)
            if not self.useJobGui:
                res.editMode = 5  # No initial Task Panel setup
            if parentJob:
                res.parentJob = parentJob
            op = PathOpGui.Create(res)
        else:
            op = PathCustom.Create('Custom', obj=None)
        op.addProperty("App::PropertyVectorDistance", "ShiftVector", "Tile",
                        PySide.QtCore.QT_TRANSLATE_NOOP("App::Property", "The shift vector applied to the model before sampling the paths for this tile."))
        op.addProperty("App::PropertyVectorDistance", "TileSize", "Tile",
                        PySide.QtCore.QT_TRANSLATE_NOOP("App::Property", "The dimensions of this tile."))
        op.addProperty("App::PropertyString", "SourceOperation", "Tile",
                        PySide.QtCore.QT_TRANSLATE_NOOP("App::Property", "The tile grid ID as X and Y values, regardless of processing direction."))
        op.addProperty("App::PropertyString", "TileDetails", "Tile",
                        PySide.QtCore.QT_TRANSLATE_NOOP("App::Property", "The tile grid ID as X and Y values, regardless of processing direction."))
        op.recompute()
        return op

    def _copyToolControllers(self, newJob):
        """_copyToolControllers() ...
        Copy tool controllers from source job to target tile job."""
        avp = False

        def _transferTCSettings(sourceTC, targetTC):
            properties = ['HorizFeed', 'HorizRapid', 'SpindleDir',
                          'SpindleSpeed', 'ToolNumber', 'VertFeed',
                          'VertRapid']
            for p in properties:
                s = getattr(sourceTC, p)
                t = getattr(targetTC, p)
                if hasattr(s, "Value"):
                    t.Value = s.Value
                else:
                    t = s

        if FreeCAD.GuiUp:
            avp = True

        # Copy tool controllers from source job to new job
        for tc in self.job.Tools.Group:
            tcName = tc.Label + "_tile"
            tcTool = tc.Tool
            tcNumber = tc.ToolNumber
            # tcToolAttrs = tc.Tool.Proxy.templateAttrs(tc.Tool)  ####################
            # newTool = PathToolBit.Factory.CreateFromAttrs(tcToolAttrs, name='ToolBit', path=None) ####################
            newTool = FreeCAD.ActiveDocument.copyObject(tcTool)
            if avp:
                newTool.ViewObject.Visibility = False
            newTC = PathToolController.Create(name=tcName, tool=newTool, toolNumber=tcNumber, assignViewProvider=avp, assignTool=True)
            newJob.Proxy.addToolController(newTC)
            _transferTCSettings(tc, newTC)

    def _saveCommands(self):
        """_saveCommands() ...
        This method saves the dressup gcode for each operation within each tile.
        A PathCustom operation is created for each source operation for each tile.
        So, a source job of three operations and settings for two tiles will create
        a set of six PathCustom operations in the new TileJob object."""
        PathLog.debug("_saveCommands()")
        msg = translate('PathTileJob',
                        'Error creating new Tile Job object.')

        if self.jobPerTile:
            for i in range(0, self.tileCount):
                tileStock = self._makeTileJobStock()
                newJob = self._createTileJob(tileStock)
                if not newJob:
                    PathLog.error(msg)
                    return None
                newJob.Label = "Tile_{}_".format(i + 1) + self.job.Label
                self.newJobs.append(newJob)
                for opDataTup in self.tileDataTups:
                    if opDataTup[5] == i:
                        self._saveTileOperation(opDataTup, newJob)

                if self.useJobGui:
                    self.job.recompute()
                else:
                    FreeCAD.ActiveDocument.recompute()
        else:
            tileStock = self._makeTileJobStock()
            newJob = self._createTileJob(tileStock)
            if not newJob:
                PathLog.error(msg)
                return None
            newJob.Label = "Tile_" + self.job.Label
            self.newJobs.append(newJob)
            for opDataTup in self.tileDataTups:
                self._saveTileOperation(opDataTup, newJob)

    def _saveTileOperation(self, opDataTup, newJob):
        (obj, hi, vi, shift, tcLabel, tileIdx, opNum, opLabel) = opDataTup
        tileNum = tileIdx + 1
        inverseShift = FreeCAD.Vector(0.0, 0.0, 0.0).sub(shift)
        label = "CustomTile {}.{}_".format(tileNum, opNum)
        custom = FreeCAD.ActiveDocument.copyObject(self.tempOp, False, True)
        newJob.Proxy.addOperation(custom)
        custom.Label = label
        custom.CoolantMode = obj.Base.CoolantMode
        custom.Gcode = [c.toGCode() for c in self._applyGcodeShift(obj.Path.Commands, inverseShift)]  # assign gcode for tile operation
        # Assign tool controller
        for tc in newJob.Tools.Group:
            if tc.Label.startswith(tcLabel + "_tile"):
                custom.ToolController = tc
                break
        # Save additional tile and op related information
        custom.ShiftVector = shift
        custom.TileSize = FreeCAD.Vector(self.xLen, self.yLen, 0.0)
        custom.TileDetails = "Tile number: {};  Grid X,Y: ({}, {});  Operation number:{}".format(tileNum, hi + 1, vi + 1, opNum)
        custom.SourceOperation = opLabel
        # Set properties as read-only
        custom.setEditorMode('ShiftVector', 1)
        custom.setEditorMode('TileSize', 1)
        # custom.setEditorMode('TileDetails', 1)
        custom.setEditorMode('SourceOperation', 1)
        custom.recompute()

    def _applyGcodeShift(self, cmds, shift):
        """_applyGcodeShift(cmds, shift) ...
        This method applies the inverse tiling translation to the boundary dressup output."""
        x = shift.x
        y = shift.y
        z = shift.z

        # Apply Gcode Shift as requested
        commands = list()
        for cmd in cmds:
            name = cmd.Name
            if name in ["G0", "G00", "G1", "G01", "G2", "G02", "G3", "G03"]:
                p = cmd.Parameters
                keys = p.keys()
                newCmd = dict()
                for k in keys:
                    if k == "X":
                        newCmd["X"] = p["X"] + x
                    elif k == "Y":
                        newCmd["Y"] = p["Y"] + y
                    elif k == "Z":
                        # z = p["Z"] + self.zShift                
                        # newCmd["Z"] = min(min(z, self.safeHeight), self.clearanceHeight)
                        newCmd["Z"] = p["Z"] + z
                    else:
                        newCmd[k] = p[k]
                commands.append(Path.Command(cmd.Name, newCmd))
            else:
                commands.append(cmd)
        return commands

    def _cleanupObjects(self):
        """_cleanupObjects() ...
        This method cleans up temporary tiling operation objects, deleting them."""
        PathLog.debug("_cleanupObjects()")
        if self.isMacro:
            for tup in self.tileDataTups:
                obj = tup[0]
                name = obj.Name
                obj.Proxy.onDelete(obj, {})
                FreeCAD.ActiveDocument.removeObject(name)
        for n in self.stockNames:
            if FreeCAD.ActiveDocument.getObject(n):
                FreeCAD.ActiveDocument.removeObject(n)
        FreeCAD.ActiveDocument.removeObject(self.tempOp.Name)  

    # Public method
    def execute(self, job, xLen, yLen):
        """execute(job, xLen, yLen) ...
        This is the public method to be called with required arguments
        to create a TileJob object in the active document."""
        PathLog.debug("TileJob.execute()")
        if not job:
            return None
        startTime = time.time()

        # initialize variables
        self._resetInstanceVariables()
        if not self._initializeVariables(job, xLen, yLen):
            return None

        # execute job tiling process
        self._getOpList()
        if not self.opList:
            msg = translate('PathTileJob',
                            'No active operations to tile in source job.')
            PathLog.error(msg)
            return None

        # Initiate methods to generate all tile gcode on per-operation basis
        self.success = self._makeTiles()
        if self.success:
            self._saveCommands()

        # Clean up temporary objects
        self._cleanupObjects()

        if self.useJobGui:
            self.job.recompute()
        else:
            FreeCAD.ActiveDocument.recompute()

        secs = time.time() - startTime
        timeStr = time.strftime('%H:%M:%S', time.gmtime(secs))
        PathLog.info("Tiling time: " + timeStr + "\n")

        if self.success:
            return self.newJobs

        return None
# Eclass


class Object:
    """class Object...
    This is a temporary dummy class for internal use only.
    """

    def __init__(self, base, path):
        self.Path = path
        self.Base = base
# Eclass


class ViewportShape(object):
    ColorYellow = (1.0,  1.0, 0.6)
    ColorBlue = (0.4,  0.9, 1.0)
    ColorRed = (1.0,  0.3, 0.3)
    TransparencyForeground = 0.7
    TransparencyBackground = 0.6

    def __init__(self, wire, color=1):
        self.wire = wire
        self.color = color

        sep = coin.SoSeparator()
        pos = coin.SoTranslation()
        mat = coin.SoMaterial()
        crd = coin.SoCoordinate3()
        fce = coin.SoFaceSet()
        hnt = coin.SoShapeHints()

        poly = [p for p in wire.discretize(Deflection=0.02)][:-1]
        # poly = [p for p in wire.discretize(Deflection=0.02)]
        polygon = [(p.x, p.y, p.z) for p in poly]
        crd.point.setValues(polygon)

        if color == 1:
            mat.diffuseColor = self.ColorYellow
        else:
            mat.diffuseColor = self.ColorBlue
        mat.transparency = self.TransparencyBackground
        
        hnt.faceType = coin.SoShapeHints.UNKNOWN_FACE_TYPE
        hnt.vertexOrdering = coin.SoShapeHints.CLOCKWISE

        sep.addChild(pos)
        sep.addChild(mat)
        sep.addChild(hnt)
        sep.addChild(crd)
        sep.addChild(fce)

        switch = coin.SoSwitch()
        switch.addChild(sep)
        switch.whichChild = coin.SO_SWITCH_NONE

        self.material = mat

        self.switch = switch
        self.root = switch
        switch.whichChild = coin.SO_SWITCH_ALL
# Eclass


if FreeCAD.GuiUp:
    TileJobUtility = TileJobGui()
    # jobs = TileJobUtility.execute()  # Uncomment for GUI macro and set TileJob.isMacro=True
