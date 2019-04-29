# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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
# *                                                                         *
# *   Additional modifications and contributions beginning 2019             *
# *   by Russell Johnson  <russ4262@gmail.com>                              *
# *   Version: Rev. 3t Usable                                               *
# *                                                                         *
# ***************************************************************************
# Revision Notes
### - Continue implementation of cut patterns: zigzag, line (line is used to force cut modes: climb, conventional)
### - Continue implementation of ignore waste feature for planar scans
### - Planar Op: move scan result(self.CLP) to local scope(CLP)

### RELEASE NOTES
# Only G0 and G1 gcode commands are used throughout.
# CutMode: Climb, Conventional is only functional for some operations, like waterline and rotational scans
# CutPattern: only functional for some operations
# IgnoreWaste: not implemented yet - target op is for planar dropcutter
# High density/resolution, mult-layer scans require significantly more processing time.
# Rotational scans are very processor intensive. 
# Rotational scans take a longer time to complete.
# Multi-pass rotational scans require a very long time to complete, 
#       even at larger SampleInterval values - ex: 1mm, 0.5mm
# Remember, the larger the model, the more time to complete an operation! 
#       Rotational require even more time. Multi-pass require much, much more time.
# This release is NOT bug-free.
# After changing an operational value in the Properties list, press the ENTER key.
#       Change all desired values, one at a time, pressing ENTER key after each change.
#       When all property changes complete, click the blue recompute icon under user menus.
# If you have an existing 3D Surface Op in your Job(s), you will need to delete it and recreate
#       with this updated script installed because it may contain additional properties not
#       created with the original version. 


from __future__ import print_function

import FreeCAD
import MeshPart
# import Part
import Path
import PathScripts.PathLog as PathLog
# import PathScripts.PathPocketBase as PathPocketBase
import PathScripts.PathUtils as PathUtils
import PathScripts.PathOp as PathOp

from PySide import QtCore
import time
import math


__title__ = "Path Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of Mill Facing operation."
__contributors__ = "roivai[FreeCAD], russ4262 (Russell Johnson)"
__scriptVersion__ = "3t Usable"
__created__ = "2016"
__lastModified__ = "2019-04-29 15:31 CST"

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

# OCL must be installed
try:
    import ocl
except:
    FreeCAD.Console.PrintError(
        translate("Path_Surface", "This operation requires OpenCamLib to be installed.") + "\n")
    import sys
    sys.exit(translate("Path_Surface", "This operation requires OpenCamLib to be installed."))

class ObjectSurface(PathOp.ObjectOp):
    '''Proxy object for Surfacing operation.'''

    # These are static while document is open, if it contains a 3D Surface Op
    initFinalDepth = None
    initOpFinalDepth = None
    initOpStartDepth = None
    docRestored = False

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(__class__, self)

    # def OpFeatures(self, obj):
    #     '''areaOpFeatures(obj) ... returns 0, Contour only requires the base profile features.'''
    #     return 0
    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geomtries'''
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureStepDown

    def initOperation(self, obj):
        '''initPocketOp(obj) ... create facing specific properties'''
        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "The library to use to generate the path"))
        obj.addProperty("App::PropertyEnumeration", "DropCutterDir", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction along which dropcutter lines are created"))
        obj.addProperty("App::PropertyEnumeration", "BoundBox", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "Should the operation be limited by the stock object or by the bounding box of the base object"))
        obj.addProperty("App::PropertyVectorDistance", "DropCutterExtraOffset", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "Additional offset to the selected bounding box"))
        obj.addProperty("App::PropertyEnumeration", "ScanType", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "Planar: Flat, 3D surface scan.  Rotational: 4th-axis rotational scan."))
        obj.addProperty("App::PropertyEnumeration", "LayerMode", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "The completion mode for the operation: single or multi-pass"))
        obj.addProperty("App::PropertyEnumeration", "CutMode", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part: Climb(ClockWise) or Conventional(CounterClockWise)"))
        obj.addProperty("App::PropertyEnumeration", "CutPattern", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Clearing pattern to use"))
        obj.addProperty("App::PropertyEnumeration", "RotationAxis", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "The model will be rotated around this axis."))
        obj.addProperty("App::PropertyFloat", "StartIndex", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "Start index(angle) for rotational scan"))
        obj.addProperty("App::PropertyFloat", "StopIndex", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "Stop index(angle) for rotational scan"))
        obj.addProperty("App::PropertyFloat", "CutterTilt", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "Stop index(angle) for rotational scan"))
        obj.addProperty("App::PropertyPercent", "StepOver", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Step over percentage of the drop cutter path"))
        obj.addProperty("App::PropertyDistance", "DepthOffset", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Z-axis offset from the surface of the object"))
        #obj.addProperty("App::PropertyFloatConstraint", "SampleInterval", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "The Sample Interval. Small values cause long wait times"))
        obj.addProperty("App::PropertyFloat", "SampleInterval", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "The Sample Interval. Small values cause long wait times"))
        obj.addProperty("App::PropertyBool", "Optimize", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable optimization which removes unnecessary points from G-Code output"))
        obj.addProperty("App::PropertyBool", "IgnoreWaste", "Waste", QtCore.QT_TRANSLATE_NOOP("App::Property", "Ignore areas that proceed below specified depth."))
        obj.addProperty("App::PropertyFloat", "IgnoreWasteDepth", "Waste", QtCore.QT_TRANSLATE_NOOP("App::Property", "Depth used to identify waste areas to ignore."))
        obj.addProperty("App::PropertyBool", "ReleaseFromWaste", "Waste", QtCore.QT_TRANSLATE_NOOP("App::Property", "Cut through waste to depth at model edge, releasing the model."))

        obj.CutMode = ['Conventional', 'Climb']
        obj.BoundBox = ['BaseBoundBox', 'Stock']
        obj.DropCutterDir = ['X', 'Y']
        obj.Algorithm = ['OCL Dropcutter', 'OCL Waterline']
        #obj.SampleInterval = (0.04, 0.01, 1.0, 0.01)
        obj.LayerMode = ['Single-pass', 'Multi-pass']
        obj.ScanType = ['Planar', 'Rotational']
        obj.RotationAxis = ['X', 'Y']
        #obj.CutPattern = ['ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']
        obj.CutPattern = ['ZigZag', 'Line']

        if not hasattr(obj, 'DoNotSetDefaultValues'):
            self.setEditorProperties(obj)

    def setEditorProperties(self, obj):
        # Used to hide inputs in properties list
        if obj.Algorithm == 'OCL Dropcutter':
            obj.setEditorMode('DropCutterDir', 0)
            #obj.setEditorMode('DropCutterExtraOffset', 0)
        else:
            obj.setEditorMode('DropCutterDir', 2)
            #obj.setEditorMode('DropCutterExtraOffset', 2)

    def onChanged(self, obj, prop):
        if prop == "Algorithm":
            self.setEditorProperties(obj)

    def opOnDocumentRestored(self, obj):
        self.setEditorProperties(obj)
        # Import FinalDepth from existing operation for use in recompute() operations
        self.initFinalDepth = obj.FinalDepth.Value
        self.initOpFinalDepth = obj.OpFinalDepth.Value
        self.docRestored = True
        print("Imported existing OpFinalDepth of " + str(self.initOpFinalDepth) + " for recompute() purposes.")
        print("Imported existing FinalDepth of " + str(self.initFinalDepth) + " for recompute() purposes.")

    def opExecute(self, obj):
        '''opExecute(obj) ... process surface operation'''
        PathLog.track()
        initIdx = 0.0

        # Instantiate additional class operation variables
        rtn = self.resetOpVariables()
        
        # mark beginning of operation
        self.startTime = time.time()

        # Set cutter for OCL based on tool controller properties
        rtn = self.setOclCutter(obj)

        self.reportThis("\n-----\n-----\nBegin 3D surface operation")
        self.reportThis("Script version: " + __scriptVersion__ + "  Lm: " + __lastModified__)

        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment) + ')\n'

        output += "(" + obj.Label + ")"
        output += "(Compensated Tool Path. Diameter: " + str(obj.ToolController.Tool.Diameter) + ")"
        
        parentJob = PathUtils.findParentJob(obj)
        if parentJob is None:
            self.reportThis("No parentJob")
            return
        self.SafeHeightOffset = parentJob.SetupSheet.SafeHeightOffset.Value
        self.ClearHeightOffset = parentJob.SetupSheet.ClearanceHeightOffset.Value

        # Import OpFinalDepth from pre-existing operation for recompute() scenarios
        if obj.OpFinalDepth.Value != self.initOpFinalDepth:
            if obj.OpFinalDepth.Value == obj.FinalDepth.Value:
                obj.FinalDepth.Value = self.initOpFinalDepth
                obj.OpFinalDepth.Value = self.initOpFinalDepth
            if self.initOpFinalDepth != None:
                obj.OpFinalDepth.Value = self.initOpFinalDepth


        # Limit start index
        if obj.StartIndex < 0.0:
            obj.StartIndex = 0.0
        if obj.StartIndex > 360.0:
            obj.StartIndex = 360.0

        # Limit stop index
        if obj.StopIndex > 360.0:
            obj.StopIndex = 360.0
        if obj.StopIndex < 0.0:
            obj.StopIndex = 0.0
       
        # Limit cutter tilt
        if obj.CutterTilt < -90.0:
            obj.CutterTilt = -90.0
        if obj.CutterTilt > 90.0:
            obj.CutterTilt = 90.0

        # Limit sample interval
        if obj.SampleInterval < 0.001:
            obj.SampleInterval = 0.001
        if obj.SampleInterval > 25.4:
            obj.SampleInterval = 25.4

        # Limit StepOver to natural number percentage
        if obj.Algorithm == 'OCL Dropcutter':
            if obj.StepOver > 100:
                obj.StepOver = 100
            if obj.StepOver < 1:
                obj.StepOver = 1        
            self.cutOut = (self.cutter.getDiameter() * (float(obj.StepOver) / 100.0))
            self.reportThis("Cut out: " + str(self.cutOut) + " mm")

        # Cycle through parts of model
        for base in self.model:
            self.reportThis("BASE object: " + str(base.Name))

            # Rotate model to initial index
            if obj.ScanType == 'Rotational':
                initIdx = obj.CutterTilt + obj.StartIndex
                if initIdx != 0.0:
                    self.basePlacement = FreeCAD.ActiveDocument.getObject(base.Name).Placement
                    if obj.RotationAxis == 'X':
                        #FreeCAD.ActiveDocument.getObject(base.Name).Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(1,0,0), initIdx))
                        base.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(1,0,0), initIdx))
                    else:
                        #FreeCAD.ActiveDocument.getObject(base.Name).Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(0,1,0), initIdx))
                        base.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(0,1,0), initIdx))

            if base.TypeId.startswith('Mesh'):
                mesh = base.Mesh
            else:
                # try/except is for Path Jobs created before GeometryTolerance
                try:
                    deflection = parentJob.GeometryTolerance
                except AttributeError:
                    import PathScripts.PathPreferences as PathPreferences
                    deflection = PathPreferences.defaultGeometryTolerance()
                base.Shape.tessellate(0.05) # 0.5 original value
                mesh = MeshPart.meshFromShape(base.Shape, Deflection=deflection)
            
            # Set bound box
            if obj.BoundBox == "BaseBoundBox":
                bb = mesh.BoundBox
            else:
                bb = parentJob.Stock.Shape.BoundBox
            
            # Objective is to remove material from surface in StepDown layers rather than one pass to FinalDepth
            final = []
            if obj.Algorithm == 'OCL Waterline':
                self.reportThis("--CutMode: " + str(obj.CutMode))
                stl = ocl.STLSurf()
                for f in mesh.Facets:
                    p = f.Points[0]
                    q = f.Points[1]
                    r = f.Points[2]
                    t = ocl.Triangle(ocl.Point(p[0], p[1], p[2] + obj.DepthOffset.Value),
                                    ocl.Point(q[0], q[1], q[2] + obj.DepthOffset.Value),
                                    ocl.Point(r[0], r[1], r[2] + obj.DepthOffset.Value))
                    aT = stl.addTriangle(t)
                final = self._waterlineOp(obj, stl, bb)        
            elif obj.Algorithm == 'OCL Dropcutter':
                # Create stl object via OCL
                stl = ocl.STLSurf()
                for f in mesh.Facets:
                    p = f.Points[0]
                    q = f.Points[1]
                    r = f.Points[2]
                    t = ocl.Triangle(ocl.Point(p[0], p[1], p[2]),
                                    ocl.Point(q[0], q[1], q[2]),
                                    ocl.Point(r[0], r[1], r[2]))
                    aT = stl.addTriangle(t)

                # Rotate model back to original index
                if obj.ScanType == 'Rotational':
                    if initIdx != 0.0:
                        initIdx = 0.0
                        base.Placement = self.basePlacement
                        #if obj.RotationAxis == 'X':
                        #    FreeCAD.ActiveDocument.getObject(base.Name).Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(1,0,0), initIdx))
                        #else:
                        #    FreeCAD.ActiveDocument.getObject(base.Name).Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(0,1,0), initIdx))

                # Prepare global holdpoint container
                if self.holdPoint == None:
                    self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))
                if self.layerEndPnt == None:
                    self.layerEndPnt = ocl.Point(float("inf"), float("inf"), float("inf"))

                if obj.ScanType == 'Rotational':
                    # Remove extended material from Stock and re-assign bb
                    if hasattr(parentJob.Stock, 'ExtXneg'):
                        parentJob.Stock.ExtXneg = 0
                        parentJob.Stock.ExtXpos = 0
                        parentJob.Stock.ExtYneg = 0
                        parentJob.Stock.ExtYpos = 0
                        parentJob.Stock.ExtZneg = 0
                        parentJob.Stock.ExtZpos = 0
                    
                    # Avoid division by zero in rotational scan calculations
                    if obj.FinalDepth.Value <= 0.0:
                        zero = obj.SampleInterval #0.00001
                        self.FinalDepth = zero
                        obj.FinalDepth.Value = 0.0
                    else:
                        self.FinalDepth = obj.FinalDepth.Value

                    # Determine boundbox radius based upon xzy limits data
                    if math.fabs(bb.ZMin) > math.fabs(bb.ZMax):
                        vlim = bb.ZMin
                    else:
                        vlim = bb.ZMax                    
                    if obj.RotationAxis == 'X':
                        # Rotation is around X-axis, cutter moves along same axis
                        if math.fabs(bb.YMin) > math.fabs(bb.YMax):
                            hlim = bb.YMin
                        else:
                            hlim = bb.YMax
                    else:
                        # Rotation is around Y-axis, cutter moves along same axis
                        if math.fabs(bb.XMin) > math.fabs(bb.XMax):
                            hlim = bb.XMin
                        else:
                            hlim = bb.XMax
                    
                    # Compute max radius of stock, as it rotates, and rotational clearance & safe heights
                    self.bbRadius = math.sqrt(hlim**2 + vlim**2)
                    self.clearHeight = self.bbRadius + parentJob.SetupSheet.ClearanceHeightOffset.Value
                    self.safeHeight = self.bbRadius + parentJob.SetupSheet.ClearanceHeightOffset.Value

                    final = self._rotationalDropCutterOp(obj, stl, bb)
                elif obj.ScanType == 'Planar':
                    final = self._planarDropCutOp(obj, stl, bb)
            # End IF
            # Send final list of commands to operation object
            self.commandlist.extend(final)
        # End IF

        self.endTime = time.time()
        self.reportThis("OPERATION time: " + str(self.endTime - self.startTime) + " sec.")
        
        print(self.opReport)



    def _planarDropCutOp(self, obj, stl, bb):
        t_before = time.time()
        pntsPerLine = 0
        ignoreWasteFlag = obj.IgnoreWaste
        ignoreMap = [1]
        
        def createTopoMap(scanCLP, ignoreDepth):
            topoMap = []
            for pt in scanCLP:
                if pt.z < ignoreDepth:
                    topoMap.append(0)
                else:
                    topoMap.append(2)
            return topoMap

        # Convert linear list of points to multi-dimensional list
        def listToMultiDimensional(points, nL, pPL):
            multiDim = []
            for L in range(0, nL):
                multiDim.append([])
                for P in range(0, pPL):
                    pi = L * pPL + P
                    multiDim[L].append(points[pi])
            return multiDim

        # De-buffer multi dimensional list
        def debufferMultiDimenList(multi):
            multi.pop(0)
            multi.pop()
            for i in range(0, len(multi)):
                multi[i].pop(0)
                multi[i].pop()
            return multi

        # Convert multi-dimensional list to linear list of points
        def multiDimensionalToList(multi):
            points = []
            for L in multi:
                points.extend(L)
            return points

        # Prepare global holdpoint container
        if self.holdPoint == None:
            self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Set crop cutter extra offset
        cdeoX = obj.DropCutterExtraOffset.x
        cdeoY = obj.DropCutterExtraOffset.y

        # the max and min XY area of the operation
        xmin = bb.XMin - cdeoX
        xmax = bb.XMax + cdeoX
        ymin = bb.YMin - cdeoY
        ymax = bb.YMax + cdeoY

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams =  [obj.FinalDepth.Value]
        else:
            dep_par = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown.Value, 0.0, obj.FinalDepth.Value)
            depthparams = [i for i in dep_par]
        # self.reportThis("--depthparams:" + str(depthparams))
        lenDP = len(depthparams)
        prevDepth = depthparams[0]

        # Determine bounding box length
        bbLength = bb.YLength
        if obj.DropCutterDir == 'Y':
            bbLength = bb.XLength

        # Determine number of lines for OCL scan
        if obj.DropCutterDir == 'X':
            exOff = obj.DropCutterExtraOffset.y
        else:
            exOff = obj.DropCutterExtraOffset.x
        numLines = int(math.ceil((bbLength + (2 * exOff)) / self.cutOut)) # Number of lines
        
        # Scan the piece to depth
        scanCLP = self._planarDropCutScan(obj, stl, bbLength, xmin, ymin, xmax, ymax, depthparams[lenDP-1], numLines, self.cutOut)
        
        # Apply depth offset
        if obj.DepthOffset.Value != 0:
            self.reportThis("--Applying DepthOffset")
            for pt in range(0, len(scanCLP)):
                scanCLP[pt].z += obj.DepthOffset.Value
        
        numPts = len(scanCLP)
        pntsPerLine = numPts / numLines
        #self.reportThis("--points: " + str(numPts))
        #self.reportThis("--lines: " + str(numLines))
        #self.reportThis("--pntsPerLine: " + str(pntsPerLine))
        if math.ceil(pntsPerLine) != math.floor(pntsPerLine):
            pntsPerLine = None

        # Create topo map for ignoring waste material
        if ignoreWasteFlag == True:
            topoMap = createTopoMap(scanCLP, obj.IgnoreWasteDepth)
            self.topoMap = listToMultiDimensional(topoMap, numLines, pntsPerLine)
            rtnA = self._bufferTopoMap(numLines, pntsPerLine)
            rtnB = self._highlightWaterline_NEW(4, 1)
            self.topoMap = debufferMultiDimenList(self.topoMap)
            ignoreMap = multiDimensionalToList(self.topoMap)

        # Extract layers per depthparams
        for lyr in  range(0, lenDP):
            # Convert current layer data to gcode
            self._planarScanToGcode(obj, lyr, prevDepth, depthparams[lyr], scanCLP, pntsPerLine, ignoreMap)
            prevDepth = depthparams[lyr]
        
        commands = self._processPlanarHolds(obj, scanCLP)
        #self.reportThis("--Elapsed time after processing gcode holds is " + str(time.time() - t_before) + " s")  # self.keepTime
        return commands

    def _planarDropCutScan(self, obj, stl, bbLength, xmin, ymin, xmax, ymax, fd, Nl, cOut):
        t_before = time.time()
        
        def cutPatternLine(obj, n, p1, p2):
            if obj.CutPattern == 'ZigZag':
                if (n % 2 == 0.0):  # even
                    lo = ocl.Line(p1, p2)     # line-object
                else:  # odd
                    lo = ocl.Line(p2, p1)     # line-object
            elif obj.CutPattern == 'Line':
                if obj.CutMode == 'Conventional':
                    lo = ocl.Line(p1, p2)     # line-object
                else:  # odd
                    lo = ocl.Line(p2, p1)     # line-object
            return lo

        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(stl)
        pdc.setCutter(self.cutter)
        pdc.setZ(fd)  # set minimumZ (final / target depth value)
        pdc.setSampling(obj.SampleInterval)

        path = ocl.Path()                   # create an empty path object

        if obj.DropCutterDir == 'X':
            # add Line objects to the path in this loop
            for n in range(0, Nl):
                if n == Nl - 1:
                    if obj.StepOver > 50:
                        cOut = (self.cutter.getDiameter() / 2)
                    y = ymax - cOut
                else:
                    y = ymin - (self.cutter.getDiameter() / 2) + ((n+1) * cOut) # all lines are offest by 1/2 cutter diameter
                p1 = ocl.Point(xmin, y, 0)   # start-point of line
                p2 = ocl.Point(xmax, y, 0)   # end-point of line

                lo = cutPatternLine(obj, n, p1, p2)
                path.append(lo)        # add the line to the path
        else:
            # add Line objects to the path in this loop
            for n in range(0, Nl):
                if n == Nl - 1:
                    if obj.StepOver > 50:
                        cOut = (self.cutter.getDiameter() / 2)
                    x = xmax - cOut
                else:
                    x = xmin - (self.cutter.getDiameter() / 2) + ((n+1) * cOut) # all lines are offest by 1/2 cutter diameter
                p1 = ocl.Point(x, ymin, 0)   # start-point of line
                p2 = ocl.Point(x, ymax, 0)   # end-point of line

                lo = cutPatternLine(obj, n, p1, p2)
                path.append(lo)        # add the line to the path

        pdc.setPath(path)

        # run drop-cutter on the path
        pdc.run()
        self.reportThis("--OCL scan took " + str(time.time() - t_before) + " s")

        clp = pdc.getCLPoints()
        # return the list the points
        return clp

    def _planarScanToGcode(self, obj, lc, prvDep, layDep, CLP, pntsPerLine, ignoreMap):
        output = []
        optimize = obj.Optimize
        ignWF = obj.IgnoreWaste
        lenCLP = len(CLP)
        lastCLP = len(CLP) - 1
        holdStart = False
        holdStop = False
        holdCount = 0
        holdLine = 0
        onLine = False
        lineOfTravel = "X"
        pointsOnLine = 0
        zMin = prvDep
        zMax = prvDep
        prcs = False
        prcsCnt = 0
        pntCount = 0
        rowCount = 1
        begLayCmds = ["aaa", "bbb"]
        minIgnVal = 1

        def makePnt(pnt):
            p = ocl.Point(float("inf"), float("inf"), float("inf"))
            p.x = pnt.x
            p.y = pnt.y
            p.z = pnt.z
            return p
        
        def beginLayerCommand(pnt, lc):
            # Send cutter to starting position(first point)
            begcmd = []
            begcmd.append(Path.Command('N (Beginning of layer ' + str(lc) + ')', {}))
            begcmd.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
            begcmd.append(Path.Command('G0', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizRapid}))
            begcmd.append(Path.Command('G1', {'Z': pnt.z, 'F': self.vertFeed}))
            begcmd.reverse()
            return begcmd
        
        # Create containers for x,y,z points
        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))
        travVect = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Determine if releasing model from ignore waste areas
        if obj.ReleaseFromWaste == True:
            minIgnVal = 0
        
        # Set values for first gcode point in layer
        pnt.x = CLP[0].x
        pnt.y = CLP[0].y
        pnt.z = CLP[0].z
        if CLP[0].z < layDep:
            pnt.z = layDep
        
        # generate the path commands
        # Begin processing ocl points list into gcode
        for i in range(0, lenCLP):
            # Calculate next point for consideration of next point
            if i < lastCLP:
                nxt.x = CLP[i + 1].x
                nxt.y = CLP[i + 1].y
                if CLP[i + 1].z < layDep:
                    nxt.z = layDep
                else:
                    nxt.z = CLP[i + 1].z
            else:
                optimize = False

            pntCount += 1
            if pntCount == 1:
                #print("--Start row: " + str(rowCount))
                nn = 1
            elif pntCount == pntsPerLine:
                #print("--End row: " + str(rowCount))
                pntCount = 0
                rowCount += 1
                # Add rise to clear height before beginning next row in CutPattern: Line
                #if obj.CutPattern == 'Line':
                #    output.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))

            # determine vector direction for each axis
            if nxt.x == pnt.x:
                travVect.x = 0
            elif nxt.x < pnt.x:
                travVect.x = -1
            else:
                travVect.x = 1

            if nxt.y == pnt.y:
                travVect.y = 0
            elif nxt.y < pnt.y:
                travVect.y = -1
            else:
                travVect.y = 1
            
            # Determine cutter line of travel
            if travVect.x == 0 and travVect.y != 0:
                lineOfTravel = "Y"
            elif travVect.y == 0 and travVect.x != 0:
                lineOfTravel = "X"
            else:
                lineOfTravel = "O"  # used for turns

            # determine if lineOfTravel is same as obj.DropCutterDir line
            if onLine == False:
                if lineOfTravel == obj.DropCutterDir:
                    onLine = True
                    self.lineCNT += 1  # increment line count
                    pointsOnLine += 1
            else:
                if lineOfTravel != obj.DropCutterDir:
                    if self.onHold == False:
                        zMax = prvDep
                    onLine = False
                    pointsOnLine = 0
                else:
                    pointsOnLine += 1
            
            # Ignore waste operation triggers
            if ignWF == False:
                prcs = True
            else:  # ignWF is TRUE
                if obj.LayerMode == 'Single-pass':
                    if ignoreMap[i] > minIgnVal:
                        if ignoreMap[i] == 1:
                            pnt.z = obj.IgnoreWasteDepth
                        if prcs == False:
                            # Move cutter to current xy position
                            output.append(Path.Command('G0', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizRapid}))
                        prcs = True
                    else:
                        if prcs == True:
                            # Raise cutter to safe height
                            output.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
                        prcs = False
                else:  # Multi-pass mode
                    if ignoreMap[i] > minIgnVal:
                        if prcs == False:
                            # Move cutter to current xy position
                            output.append(Path.Command('G0', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizRapid}))
                        prcs = True
                    else:
                        prcs = False
            # End of ignWF
            
            if prcs == True:
                prcsCnt += 1
                if prcsCnt == 1:
                    begLayCmds = beginLayerCommand(pnt, lc)  # start layer at this point

                if obj.LayerMode == 'Multi-pass':
                    # if z travels above previous layer, start/continue hold high cycle
                    if pnt.z > prvDep:
                        if self.onHold == False:
                            holdStart = True
                        self.onHold = True
                    
                # Update zMin and zMax values
                if pnt.z < zMin:
                    zMin = pnt.z
                if pnt.z > zMax:
                    zMax = pnt.z
                
                if self.onHold == True:
                    if holdStart == True:
                        # go to current coordinate
                        output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))
                        # Save holdStart coordinate and prvDep values
                        self.holdPoint.x = pnt.x
                        self.holdPoint.y = pnt.y
                        self.holdPoint.z = pnt.z
                        
                        holdCount += 1  # Increment hold count
                        holdLine = self.lineCNT  # remember holdLine
                        self.holdStartPnts.append(makePnt(pnt))
                        self.holdPrevLayerVals.append(prvDep)
                        holdStart = False  # cancel holdStart

                    # hold cutter high until Z value drops below prvDep
                    if pnt.z <= prvDep:
                        holdStop = True
                # End of onHold
                    
                if holdStop == True:
                    # Send hold and current points to 
                    if holdLine == self.lineCNT:
                        # if  start and stop points on same line, process has simple hold
                        self.holdStartPnts.pop()
                        self.holdPrevLayerVals.pop()
                        zMax += 2.0
                        for cmd in self.holdStopCmds(obj, zMax, prvDep, pnt, "Hold Stop: in-line"):
                            output.append(cmd)
                    else:
                        # if  start and stop points on different lines, process has complex hold
                        self.holdStopPnts.append(makePnt(pnt))
                        self.holdZMaxVals.append(zMax)
                        self.holdStopTypes.append("Mid")
                        output.append("HD")  # add placeholder for processing of hold
                        self.holdPntCnt += 1
                    
                    # reset necessary hold related settings
                    zMax = prvDep
                    holdStop = False
                    self.onHold = False
                    self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))
                # End of holdStop

                if self.onHold == False:
                    if not optimize or not self.isPointOnLine(FreeCAD.Vector(prev.x, prev.y, prev.z), FreeCAD.Vector(nxt.x, nxt.y, nxt.z), FreeCAD.Vector(pnt.x, pnt.y, pnt.z)):
                        output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))
                    elif i == lastCLP:
                        output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))

                # Rotate point data
                prev.x = pnt.x
                prev.y = pnt.y
                prev.z = pnt.z
            # End of prcs
            pnt.x = nxt.x
            pnt.y = nxt.y
            pnt.z = nxt.z


        #save current layer end point
        if self.onHold == True:
            if holdLine == self.lineCNT:
                self.holdStartPnts.pop()
                self.holdPrevLayerVals.pop()
                for cmd in self.holdStopCmds(obj, obj.SafeHeight.Value, obj.SafeHeight.Value, pnt, "Hold Stop: Layer endpoint online"):  #zMax as prvDep removes drop to prvDep
                    output.append(cmd)
            else:
                self.holdStopPnts.append(makePnt(pnt))
                self.holdZMaxVals.append(zMax)
                output.append("HD")  # add placeholder for processing of hold
                self.holdStopTypes.append("End")  # tag hold type
                self.holdPntCnt += 1
            self.onHold = False
        
        # save last point for insertion into next layer CLP as start point
        endPnt = pnt
        endPnt.z = obj.OpStockZMax.Value + obj.DepthOffset.Value
        self.layerEndPnt = endPnt
        # self.reportThis("----Points after linear optimization: " + str(len(output)))
        # append layer commands to operation command list
        for cmd in begLayCmds:
            output.insert(0, cmd)
        for o in output:
            self.gcodeCmds.append(o)
        self.gcodeCmds.append(Path.Command('N (End of layer ' + str(lc) + ')', {}))
        #return output

    def _processPlanarHolds(self, obj, scanCLP):    
        # Process all HOLDs in gcode command list
        self.keepTime = time.time()
        hldcnt = 0
        hpCmds = []
        lenHP = len(self.holdStartPnts)
        commands = [Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid})]
        self.reportThis("--Processing " + str(lenHP)  + " HOLD optimizations---")
        # cycle throug hold points
        if lenHP > 0:
            hdCnt = 0
            lenGC = len(self.gcodeCmds)
            for idx in range(0, lenGC):
                if self.gcodeCmds[idx] == "HD":
                    hdCnt += 1
                    # process hold here
                    p1 = self.holdStartPnts.pop(0)
                    p2 = self.holdStopPnts.pop(0)
                    pd = self.holdPrevLayerVals.pop(0)
                    hscType = self.holdStopTypes.pop(0)

                    if hscType == "End":
                        N = None
                        # Create gcode commands to connect p1 and p2
                        hpCmds = self.holdStopEndCmds(obj, p2, "Hold Stop: End point")
                    elif hscType == "Mid":
                        # Set the max and min XY boundaries of the HOLD connection operation
                        cutterClearance = self.cutter.getDiameter() / 1.25
                        if p1.x < p2.x:
                            xmin = p1.x - cutterClearance
                            xmax = p2.x + cutterClearance
                        else: 
                            xmin = p2.x - cutterClearance
                            xmax = p1.x + cutterClearance

                        if p1.y < p2.y:
                            ymin = p1.y - cutterClearance
                            ymax = p2.y + cutterClearance
                        else: 
                            ymin = p2.y - cutterClearance
                            ymax = p1.y + cutterClearance
                        # get focused list of points based on bound box with p1 and p2 as corners, with cutter diam. as additional buffer
                        subCLP = self.subsectionCLP(scanCLP, xmin, ymin, xmax, ymax)
                        # Determine max z height for clearance between p1 and p2
                        zMax = self.getMaxHeight(self.targetDepth, p1, p2, self.cutter.getDiameter(), subCLP)
                        # Create gcode commands to connect p1 and p2
                        hpCmds = self.holdStopCmds(obj, zMax, pd, p2, "Hold Stop: Group processed")
                    # Add commands to list
                    for cmd in hpCmds:
                        commands.append(cmd)
                    hldcnt += 1
                else:
                    commands.append(self.gcodeCmds[idx])
        else:
            commands = self.gcodeCmds
        return commands



    def _rotationalDropCutterOp(self, obj, stl, bb):
        eT = time.time()
        self.resetTolerance = 0.0000001 # degrees
        self.layerEndzMax = 0.0
        commands = []
        scanLines = []
        advances = []
        iSTG = []
        rSTG = []
        rings = []
        lCnt = 0
        rNum = 0
        stepDeg = 1.1
        layCircum = 1.1
        begIdx = 0.0
        endIdx = 0.0
        arc = 0.0
        sumAdv = 0.0
        bbRad = self.bbRadius

        def invertAdvances(advances):
            idxs = [1.1]
            for adv in advances:
                idxs.append(-1 * adv)
            idxs.pop(0)
            return idxs

        def linesToPointRings(scanLines):
            rngs = []
            numPnts = len(scanLines[0])  # Number of points per line along axis, at obj.SampleInterval spacing
            for line in scanLines:  # extract circular set(ring) of points from scan lines
                if len(line) != numPnts:
                    print("Error: line lengths not equal")
                    return rngs

            for num in range(0, numPnts):
                rngs.append([1.1]) # Initiate new ring
                for line in scanLines:  # extract circular set(ring) of points from scan lines
                    rngs[num].append(line[num])
                nn = rngs[num].pop(0)
            return rngs

        def indexAdvances(arc, stepDeg):
            indexes = [0.0]
            numSteps = int(math.floor(arc / stepDeg))
            for ns in range(0, numSteps):
                indexes.append(stepDeg)
            
            travel = sum(indexes)
            if arc == 360.0:
                indexes.insert(0, 0.0)
            else:
                indexes.append(arc - travel)

            return indexes

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams =  [self.FinalDepth]
        else:
            dep_par = PathUtils.depth_params(self.clearHeight, self.safeHeight, self.bbRadius, obj.StepDown.Value, 0.0, self.FinalDepth)
            depthparams = [i for i in dep_par]
        prevDepth = depthparams[0]
        lenDP = len(depthparams)

        # Set drop cutter extra offset
        cdeoX = obj.DropCutterExtraOffset.x
        cdeoY = obj.DropCutterExtraOffset.y

        # Set updated bound box values and redefine the new min/mas XY area of the operation based on greatest point radius of model
        bb.ZMin = -1 * bbRad
        bb.ZMax = bbRad
        if obj.RotationAxis == 'X':
            bb.YMin = -1 * bbRad
            bb.YMax = bbRad
            ymin = 0.0
            ymax = 0.0
            xmin = bb.XMin - cdeoX
            xmax = bb.XMax + cdeoX
        else:
            bb.XMin = -1 * bbRad
            bb.XMax = bbRad
            ymin = bb.YMin - cdeoY
            ymax = bb.YMax + cdeoY
            xmin = 0.0
            xmax = 0.0

        # Calculate arc
        begIdx = obj.StartIndex
        endIdx = obj.StopIndex
        if endIdx < begIdx:
            begIdx -= 360.0        
        arc = endIdx - begIdx

        # Begin gcode operation with raising cutter to safe height
        commands.append(Path.Command('G0', {'Z': self.safeHeight, 'F': self.vertRapid}))

        # Complete rotational scans at layer and translate into gcode
        for layDep in depthparams:
            t_before = time.time()
            self.reportThis("--layDep " + str(layDep))

            # Compute circumference and step angles for current layer
            layCircum = 2 * math.pi * layDep
            if lenDP == 1:
                layCircum = 2 * math.pi * bbRad

            # Set axial feed rates
            self.axialFeed = 360 / layCircum * self.horizFeed
            self.axialRapid = 360 / layCircum * self.horizRapid

            # Determine step angle.
            if obj.RotationAxis == obj.DropCutterDir:  # Same == indexed
                stepDeg = (self.cutOut / layCircum) * 360.0
            else:
                stepDeg = (obj.SampleInterval / layCircum) * 360.0

            # Limit step angle and determine rotational index angles [indexes].
            if stepDeg > 120.0:
                stepDeg = 120.0
            advances = indexAdvances(arc, stepDeg)  # Reset for each step down layer

            # Perform rotational indexed scans to layer depth
            if obj.RotationAxis == obj.DropCutterDir:  # Same == indexed OR parallel
                sample = obj.SampleInterval
            else:
                sample = self.cutOut
            scanLines = self._indexedDropCutScan(obj, stl, advances, xmin, ymin, xmax, ymax, layDep, sample)
            
            # Complete rotation if necessary
            if arc == 360.0:
                advances.append(360.0 - sum(advances))
                advances.pop(0)
                zero = scanLines.pop(0)
                scanLines.append(zero)            
            
            # Translate OCL scans into gcode
            if obj.RotationAxis == obj.DropCutterDir:  # Same == indexed (cutter runs parallel to axis)
                # Invert advances if RotationAxis == Y
                if obj.RotationAxis == 'Y':
                    advances = invertAdvances(advances)

                #Translate scan to gcode
                #sumAdv = 0.0
                sumAdv = begIdx
                for sl in range(0, len(scanLines)):
                    sumAdv += advances[sl]
                    # Translate scan to gcode
                    iSTG = self._indexedScanToGcode(obj, sl, scanLines[sl], sumAdv, prevDepth, layDep, lenDP)
                    commands.extend(iSTG)
                    
                    # Add rise to clear height before beginning next index in CutPattern: Line
                    #if obj.CutPattern == 'Line':
                    #    commands.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))
                    
                    # Raise cutter to safe height after each index cut
                    commands.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))
                #Eol
            else:
                if obj.CutMode == 'Conventional':
                    advances = invertAdvances(advances)
                    rt = advances.reverse()
                    rtn = scanLines.reverse()

                # Invert advances if RotationAxis == Y
                if obj.RotationAxis == 'Y':
                    advances = invertAdvances(advances)

                # Begin gcode operation with raising cutter to safe height
                commands.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))                
                
                # Convert rotational scans into gcode
                rings = linesToPointRings(scanLines)
                rNum = 0
                for rng in rings:
                    rSTG = self._rotationalScanToGcode(obj, rng, rNum, prevDepth, layDep, advances)
                    commands.extend(rSTG)
                    if arc != 360.0:
                        clrZ = self.layerEndzMax + self.SafeHeightOffset
                        commands.append(Path.Command('G0', {'Z': clrZ, 'F': self.vertRapid}))
                    rNum += 1
                #Eol

                # Add rise to clear height before beginning next index in CutPattern: Line
                #if obj.CutPattern == 'Line':
                #    commands.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))                
            
            prevDepth = layDep
            lCnt += 1  # increment layer count
            self.reportThis("--Layer " + str(lCnt) + ": " + str(len(advances)) + " OCL scans and gcode in " + str(time.time() - t_before) + " s")
            time.sleep(0.2)
        #Eol
        return commands

    def _indexedDropCutScan(self, obj, stl, advances, xmin, ymin, xmax, ymax, layDep, sample):
        cutterOfst = 0.0
        radsRot = 0.0
        reset = 0.0
        iCnt = 0
        Lines = []
        result = None

        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setCutter(self.cutter)
        pdc.setZ(layDep)  # set minimumZ (final / ta9rget depth value)
        pdc.setSampling(sample)

        #if self.useTiltCutter == True:
        if obj.CutterTilt != 0.0:
            cutterOfst = layDep * math.sin(obj.CutterTilt * math.pi / 180.0)
            self.reportThis("CutterTilt: cutterOfst is " + str(cutterOfst))

        sumAdv = 0.0
        for adv in advances:
            sumAdv += adv
            if adv > 0.0:
                # Rotate STL object using OCL method
                radsRot = math.radians(adv)
                if obj.RotationAxis == 'X':
                    rStl = stl.rotate(radsRot, 0.0, 0.0)
                else:
                    rStl = stl.rotate(0.0, radsRot, 0.0)
            
            # Set STL after rotation is made
            pdc.setSTL(stl)

            # add Line objects to the path in this loop
            if obj.RotationAxis == 'X':
                p1 = ocl.Point(xmin, cutterOfst, 0.0)   # start-point of line
                p2 = ocl.Point(xmax, cutterOfst, 0.0)   # end-point of line
            else:
                p1 = ocl.Point(cutterOfst, ymin, 0.0)   # start-point of line
                p2 = ocl.Point(cutterOfst, ymax, 0.0)   # end-point of line  
    
            # Create line object
            if obj.RotationAxis == obj.DropCutterDir:  # parallel cut
                if obj.CutPattern == 'ZigZag':
                    if (iCnt % 2 == 0.0):  # even
                        lo = ocl.Line(p1, p2)
                    else:  # odd
                        lo = ocl.Line(p2, p1)
                elif obj.CutPattern == 'Line':
                    if obj.CutMode == 'Conventional':
                        lo = ocl.Line(p1, p2)
                    else:
                        lo = ocl.Line(p2, p1)
            else:
                lo = ocl.Line(p1, p2)   # line-object

            path = ocl.Path()                   # create an empty path object
            path.append(lo)         # add the line to the path
            pdc.setPath(path)       # set path
            pdc.run()               # run drop-cutter on the path
            result = pdc.getCLPoints()
            Lines.append(result)  # request the list of points

            iCnt += 1
        # End loop
        # Rotate STL object back to original position using OCL method
        reset = -1 * math.radians(sumAdv - self.resetTolerance)
        if obj.RotationAxis == 'X':
            rStl = stl.rotate(reset, 0.0, 0.0)
        else:
            rStl = stl.rotate(0.0, reset, 0.0)
        self.resetTolerance = 0.0

        return Lines

    def _indexedScanToGcode(self, obj, li, CLP, idxAng, prvDep, layerDepth, numDeps):
        # generate the path commands
        output = []
        optimize = obj.Optimize
        holdCount = 0
        holdStart = False
        holdStop = False
        zMax = prvDep
        lenCLP = len(CLP)
        lastCLP = lenCLP - 1
        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))
        
        # Create frist point
        pnt.x = CLP[0].x
        pnt.y = CLP[0].y
        pnt.z = CLP[0].z + float(obj.DepthOffset.Value)

        # Rotate to correct index location
        if obj.RotationAxis == 'X':
            output.append(Path.Command('G0', {'A': idxAng, 'F': self.axialFeed}))
        else:
            output.append(Path.Command('G0', {'B': idxAng, 'F': self.axialFeed}))

        if li > 0:
            if pnt.z > self.layerEndPnt.z:
                clrZ = pnt.z + 2.0
                output.append(Path.Command('G1', {'Z': clrZ, 'F': self.vertRapid}))
        else:
            output.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))

        output.append(Path.Command('G0', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizRapid}))
        output.append(Path.Command('G1', {'Z': pnt.z, 'F': self.vertFeed}))

        for i in range(0, lenCLP):
            if i < lastCLP:
                nxt.x = CLP[i + 1].x
                nxt.y = CLP[i + 1].y
                nxt.z = CLP[i + 1].z + float(obj.DepthOffset.Value)
            else:
                optimize = False

            # Update zMax values
            if pnt.z > zMax:
                zMax = pnt.z

            if obj.LayerMode == 'Multi-pass':
                # if z travels above previous layer, start/continue hold high cycle
                if pnt.z > prvDep and optimize == True:  
                    if self.onHold == False:
                        holdStart = True
                    self.onHold = True
                
                if self.onHold == True:
                    if holdStart == True:
                        # go to current coordinate
                        output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))
                        # Save holdStart coordinate and prvDep values
                        self.holdPoint.x = pnt.x
                        self.holdPoint.y = pnt.y
                        self.holdPoint.z = pnt.z                    
                        holdCount += 1  # Increment hold count
                        holdStart = False  # cancel holdStart

                    # hold cutter high until Z value drops below prvDep
                    if pnt.z <= prvDep:
                        holdStop = True
                    
                if holdStop == True:
                    # Send hold and current points to 
                    zMax += 2.0
                    for cmd in self.holdStopCmds(obj, zMax, prvDep, pnt, "Hold Stop: in-line"):
                        output.append(cmd)
                    # reset necessary hold related settings
                    zMax = prvDep
                    holdStop = False
                    self.onHold = False
                    self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))

            if self.onHold == False:
                if not optimize or not self.isPointOnLine(FreeCAD.Vector(prev.x, prev.y, prev.z), FreeCAD.Vector(nxt.x, nxt.y, nxt.z), FreeCAD.Vector(pnt.x, pnt.y, pnt.z)):
                    output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))
                elif i == lastCLP:
                    output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))
 
            # Rotate point data
            prev.x = pnt.x
            prev.y = pnt.y
            prev.z = pnt.z
            pnt.x = nxt.x
            pnt.y = nxt.y
            pnt.z = nxt.z
        # self.reportThis("points after optimization: " + str(len(output)))
        output.append(Path.Command('N (End index angle ' + str(round(idxAng, 4)) + ')', {}))

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt.x = pnt.x
        self.layerEndPnt.y = pnt.y
        self.layerEndPnt.z = pnt.z

        return output

    def _rotationalScanToGcode(self, obj, RNG, rN, prvDep, layDep, advances):
        # generate the path commands
        output = []
        nxtAng = 0
        zMax = 0.0
        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))

        begIdx = obj.StartIndex
        endIdx = obj.StopIndex
        if endIdx < begIdx:
            begIdx -= 360.0   
             
        # Rotate to correct index location
        axisOfRot = 'A'
        if obj.RotationAxis == 'Y':
            axisOfRot = 'B'
        
        # Create frist point
        ang = 0.0 + obj.CutterTilt
        pnt.x = RNG[0].x
        pnt.y = RNG[0].y
        pnt.z = RNG[0].z + float(obj.DepthOffset.Value)
        
        # Adjust feed rate based on radius/circumferance of cutter.
        # Original feed rate based on travel at circumferance.
        if rN > 0:
            #if pnt.z > self.layerEndPnt.z:
            if pnt.z >= self.layerEndzMax:
                clrZ = pnt.z + 5.0
                output.append(Path.Command('G1', {'Z': clrZ, 'F': self.vertRapid}))
        else:
            output.append(Path.Command('G1', {'Z': self.clearHeight, 'F': self.vertRapid}))

        output.append(Path.Command('G0', {axisOfRot: ang, 'F': self.axialFeed}))
        output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'F': self.axialFeed}))
        output.append(Path.Command('G1', {'Z': pnt.z, 'F': self.axialFeed}))
        
        lenRNG = len(RNG)
        lastIdx = lenRNG - 1
        for i in range(0, lenRNG):
            if i < lastIdx:
                nxtAng = ang + advances[i + 1]
                nxt.x = RNG[i + 1].x
                nxt.y = RNG[i + 1].y
                nxt.z = RNG[i + 1].z + float(obj.DepthOffset.Value)
            
            if pnt.z > zMax:
                zMax = pnt.z

            output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, axisOfRot: ang, 'F': self.axialFeed}))
            pnt.x = nxt.x
            pnt.y = nxt.y
            pnt.z = nxt.z
            ang = nxtAng
        # self.reportThis("points after optimization: " + str(len(output)))

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt.x = RNG[0].x
        self.layerEndPnt.y = RNG[0].y
        self.layerEndPnt.z = RNG[0].z
        self.layerEndIdx = ang
        self.layerEndzMax = zMax
        
        # Move cutter to final point
        # output.append(Path.Command('G1', {'X': self.layerEndPnt.x, 'Y': self.layerEndPnt.y, 'Z': self.layerEndPnt.z, axisOfRot: endang, 'F': self.axialFeed}))

        return output



    def _waterlineOp(self, obj, stl, bb):
        t_begin = time.time() #self.keepTime = time.time()
        commands = []

        # Prepare global holdpoint and layerEndPnt containers
        if self.holdPoint == None:
            self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))
        if self.layerEndPnt == None:
            self.layerEndPnt = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Set extra offset to diameter of cutter to allow cutter to move around perimeter of model
        # Need to make DropCutterExtraOffset available for waterline algorithm
        #cdeoX = obj.DropCutterExtraOffset.x
        #cdeoY = obj.DropCutterExtraOffset.y
        cdeoX = 0.6 * self.cutter.getDiameter()
        cdeoY = 0.6 * self.cutter.getDiameter()

        # the max and min XY area of the operation
        xmin = bb.XMin - cdeoX
        xmax = bb.XMax + cdeoX
        ymin = bb.YMin - cdeoY
        ymax = bb.YMax + cdeoY
        
        smplInt = obj.SampleInterval
        minSampInt = 0.001  # value is mm
        if smplInt < minSampInt:
            smplInt = minSampInt
        
        # Determine bounding box length for the OCL scan
        bbLength = math.fabs(ymax -  ymin)
        numScanLines = int(math.ceil(bbLength / smplInt) + 1) # Number of lines

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams = [obj.FinalDepth.Value]
        else:
            dep_par = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown.Value, 0.0, obj.FinalDepth.Value)
            depthparams = [dp for dp in dep_par]
        lenDP = len(depthparams)

        # Scan the piece to depth at smplInt
        oclScan = []
        oclScan = self._waterlineDropCutScan(stl, smplInt, xmin, xmax, ymin, depthparams[lenDP-1], numScanLines)
        lenOS = len(oclScan)
        ptPrLn = int(lenOS / numScanLines)
        
        # Convert oclScan list of points to multi-dimensional list
        scanLines = []
        for L in range(0, numScanLines):
            scanLines.append([])
            for P in range(0, ptPrLn):
                pi = L * ptPrLn + P
                scanLines[L].append(oclScan[pi])
        lenSL = len(scanLines)
        pntsPerLine = len(scanLines[0])
        self.reportThis("--OCL scan: " + str(lenSL * pntsPerLine) + " points, with " + str(numScanLines) + " lines and " + str(pntsPerLine) + " pts/line")
        self.reportThis("--Setup, OCL scan, and scan conversion to multi-dimen. list took " + str(time.time() - t_begin) + " s")

        # Extract Wl layers per depthparams
        lyr = 0
        cmds = []
        layTime = time.time()
        self.topoMap = []
        for layDep in depthparams:
            cmds = self._getWaterline(obj, scanLines, layDep, lyr, lenSL, pntsPerLine)
            commands.extend(cmds)
            lyr += 1
        self.reportThis("--All layer scans combined took " + str(time.time() - layTime) + " s")
        return commands

    def _waterlineDropCutScan(self, stl, smplInt, xmin, xmax, ymin, fd, numScanLines):
        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(stl)
        pdc.setCutter(self.cutter)
        pdc.setZ(fd)  # set minimumZ (final / target depth value)
        pdc.setSampling(smplInt)

        # Create line object as path
        path = ocl.Path()                   # create an empty path object
        for nSL in range(0, numScanLines):
            yVal = ymin + (nSL * smplInt)
            p1 = ocl.Point(xmin, yVal, fd)   # start-point of line
            p2 = ocl.Point(xmax, yVal, fd)   # end-point of line
            l = ocl.Line(p1, p2)     # line-object
            path.append(l)        # add the line to the path
        pdc.setPath(path)        
        pdc.run() # run drop-cutter on the path
        
        # return the list the points
        return pdc.getCLPoints()

    def _getWaterline(self, obj, scanLines, layDep, lyr, lenSL, pntsPerLine):
        commands = []
        cmds = []
        loopList = []
        self.topoMap = []
        # Create topo map from scanLines (highs and lows)
        self.topoMap = self._createTopoMap(scanLines, layDep, lenSL, pntsPerLine)
        # Add buffer lines and columns to topo map
        rtn = self._bufferTopoMap(lenSL, pntsPerLine)
        # Identify layer waterline from OCL scan
        rtn = self._highlightWaterline(4)
        # Extract waterline and convert to gcode
        loopList = self._extractWaterlines(obj, scanLines, lyr, layDep)
        time.sleep(0.1)
        #save commands
        for loop in loopList:
            cmds = self._loopToGcode(obj, layDep, loop)
            commands.extend(cmds)
        return commands

    def _createTopoMap(self, scanLines, layDep, lenSL, pntsPerLine):
        topoMap = []
        for L in range(0, lenSL):
            topoMap.append([])
            for P in range(0, pntsPerLine):
                if scanLines[L][P].z > layDep:
                    topoMap[L].append(2)
                else:
                    topoMap[L].append(0)
        return topoMap
        
    def _bufferTopoMap(self, lenSL, pntsPerLine):
        # add buffer boarder of zeros to all sides to topoMap data
        pre = [0,0]
        post = [0,0]
        for p in range(0, pntsPerLine):
            pre.append(0)
            post.append(0)
        for l in range(0, lenSL):
            self.topoMap[l].insert(0, 0)
            self.topoMap[l].append(0)
        self.topoMap.insert(0, pre)
        self.topoMap.append(post)
        return True

    def _highlightWaterline(self, extraMaterial):
        TM = self.topoMap
        lastPnt = len(TM[1]) - 1
        lastLn = len(TM) - 1
        highFlag = 0

        #self.reportThis("--Convert parallel data to ridges")
        for lin in range(1, lastLn):
            for pt in range(1, lastPnt):  # Ignore first and last points
                if TM[lin][pt] == 0:
                    if TM[lin][pt + 1] == 2:  # step up
                        TM[lin][pt] = 1
                    if TM[lin][pt - 1] == 2: #step down
                        TM[lin][pt] = 1

        #self.reportThis("--Convert perpendicular data to ridges and highlight ridges")
        for pt in range(1, lastPnt):  # Ignore first and last points
            for lin in range(1, lastLn):
                if TM[lin][pt] == 0:
                    highFlag = 0
                    if TM[lin + 1][pt] == 2:  # step up
                        TM[lin][pt] = 1
                    if TM[lin - 1][pt] == 2: #step down
                        TM[lin][pt] = 1
                elif TM[lin][pt] == 2:
                    highFlag += 1
                    if highFlag == 3:
                        if TM[lin - 1][pt - 1] < 2 or TM[lin - 1][pt + 1] < 2:
                            highFlag = 2
                        else:
                            TM[lin - 1][pt] = extraMaterial
                            highFlag = 2
                
        # Square corners
        #self.reportThis("--Square corners")
        for pt in range(1, lastPnt):
            for lin in range(1, lastLn):
                if TM[lin][pt] == 1:                    # point == 1
                    cont = True
                    if TM[lin + 1][pt] == 0:            # forward == 0
                        if TM[lin + 1][pt - 1] == 1:    # forward left == 1
                            if TM[lin][pt - 1] == 2:    # left == 2
                                TM[lin + 1][pt] = 1     # square the corner
                                cont = False
                        
                        if cont == True and TM[lin + 1][pt + 1] == 1:  # forward right == 1
                            if TM[lin][pt + 1] == 2:    # right == 2
                                TM[lin + 1][pt] = 1     # square the corner
                        cont = True
                    
                    if TM[lin - 1][pt] == 0:          # back == 0
                        if TM[lin - 1][pt - 1] == 1:    # back left == 1
                            if TM[lin][pt - 1] == 2:    # left == 2
                                TM[lin - 1][pt] = 1     # square the corner
                                cont = False
                        
                        if cont == True and TM[lin - 1][pt + 1] == 1:  # back right == 1
                            if TM[lin][pt + 1] == 2:    # right == 2
                                TM[lin - 1][pt] = 1     # square the corner

        # remove inside corners
        #self.reportThis("--Remove inside corners")
        for pt in range(1, lastPnt):
            for lin in range(1, lastLn):
                if TM[lin][pt] == 1:                    # point == 1
                    if TM[lin][pt + 1] == 1:
                        if TM[lin - 1][pt + 1] == 1 or TM[lin + 1][pt + 1] == 1:
                            TM[lin][pt + 1] = 9
                    elif TM[lin][pt - 1] == 1:
                        if TM[lin - 1][pt - 1] == 1 or TM[lin + 1][pt - 1] == 1:
                            TM[lin][pt - 1] = 9
                        
        #print("\n-------------")
        #for li in TM:
        #    print("Line: " + str(li))
        return True

    def _highlightWaterline_NEW(self, extraMaterial, insCorn):
        TM = self.topoMap
        lastPnt = len(TM[1]) - 1
        lastLn = len(TM) - 1
        highFlag = 0

        #self.reportThis("--Convert parallel data to ridges")
        for lin in range(1, lastLn):
            for pt in range(1, lastPnt):  # Ignore first and last points
                if TM[lin][pt] == 0:
                    if TM[lin][pt + 1] == 2:  # step up
                        TM[lin][pt] = 1
                    if TM[lin][pt - 1] == 2: #step down
                        TM[lin][pt] = 1

        #self.reportThis("--Convert perpendicular data to ridges and highlight ridges")
        for pt in range(1, lastPnt):  # Ignore first and last points
            for lin in range(1, lastLn):
                if TM[lin][pt] == 0:
                    highFlag = 0
                    if TM[lin + 1][pt] == 2:  # step up
                        TM[lin][pt] = 1
                    if TM[lin - 1][pt] == 2: #step down
                        TM[lin][pt] = 1
                elif TM[lin][pt] == 2:
                    highFlag += 1
                    if highFlag == 3:
                        if TM[lin - 1][pt - 1] < 2 or TM[lin - 1][pt + 1] < 2:
                            highFlag = 2
                        else:
                            TM[lin - 1][pt] = extraMaterial
                            highFlag = 2
                
        # Square corners
        #self.reportThis("--Square corners")
        for pt in range(1, lastPnt):
            for lin in range(1, lastLn):
                if TM[lin][pt] == 1:                    # point == 1
                    cont = True
                    if TM[lin + 1][pt] == 0:            # forward == 0
                        if TM[lin + 1][pt - 1] == 1:    # forward left == 1
                            if TM[lin][pt - 1] == 2:    # left == 2
                                TM[lin + 1][pt] = 1     # square the corner
                                cont = False
                        
                        if cont == True and TM[lin + 1][pt + 1] == 1:  # forward right == 1
                            if TM[lin][pt + 1] == 2:    # right == 2
                                TM[lin + 1][pt] = 1     # square the corner
                        cont = True
                    
                    if TM[lin - 1][pt] == 0:          # back == 0
                        if TM[lin - 1][pt - 1] == 1:    # back left == 1
                            if TM[lin][pt - 1] == 2:    # left == 2
                                TM[lin - 1][pt] = 1     # square the corner
                                cont = False
                        
                        if cont == True and TM[lin - 1][pt + 1] == 1:  # back right == 1
                            if TM[lin][pt + 1] == 2:    # right == 2
                                TM[lin - 1][pt] = 1     # square the corner

        # remove inside corners
        #self.reportThis("--Remove inside corners")
        for pt in range(1, lastPnt):
            for lin in range(1, lastLn):
                if TM[lin][pt] == 1:                    # point == 1
                    if TM[lin][pt + 1] == 1:
                        if TM[lin - 1][pt + 1] == 1 or TM[lin + 1][pt + 1] == 1:
                            TM[lin][pt + 1] = insCorn
                    elif TM[lin][pt - 1] == 1:
                        if TM[lin - 1][pt - 1] == 1 or TM[lin + 1][pt - 1] == 1:
                            TM[lin][pt - 1] = insCorn
                        
        #print("\n-------------")
        #for li in TM:
        #    print("Line: " + str(li))
        return True

    def _extractWaterlines(self, obj, oclScan, lyr, layDep):
        srch = True
        lastPnt = len(self.topoMap[0]) - 1
        lastLn = len(self.topoMap) - 1
        maxSrchs = 5
        srchCnt = 1
        loopList = []
        loop = []
        loopNum = 0

        if obj.CutMode == 'Conventional':
            lC =   [ 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0]
            pC =   [-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1]
        else:
            lC =   [-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0]
            pC =   [-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1]
        
        while srch == True:
            srch = False
            if srchCnt > maxSrchs:
                self.reportThis("Max search scans, " + str(maxSrchs) + " reached\nPossible incomplete waterline result!")
                break
            for L in range(1, lastLn):
                for P in range(1, lastPnt):
                    if self.topoMap[L][P] == 1:
                        # start loop follow
                        srch = True
                        loopNum += 1
                        loop = self._trackLoop(oclScan, lC, pC, L, P, loopNum)
                        self.topoMap[L][P] = 0  # Mute the starting point
                        loopList.append(loop)
            srchCnt += 1
        #self.reportThis("Search count for layer " + str(lyr) + " is " + str(srchCnt) + ", with " + str(loopNum) + " loops.")
        return loopList

    def _trackLoop(self, oclScan, lC, pC, L, P, loopNum):
        loop = [oclScan[L - 1][P - 1]]  # Start loop point list
        cur = [L, P, 1]
        prv = [L, P - 1, 1]
        nxt = [L, P + 1, 1]
        follow = True
        ptc = 0
        ptLmt = 200000
        while follow == True:
            ptc += 1
            if ptc > ptLmt:
                self.reportThis("Loop number " + str(loopNum) + " at [" + str(nxt[0]) + ", " + str(nxt[1]) + "] pnt count exceeds, " + str(ptLmt) + ".  Stopped following loop.")
                break
            nxt = self._findNextWlPoint(lC, pC, cur[0], cur[1], prv[0], prv[1])  # get next point
            loop.append(oclScan[nxt[0] - 1][nxt[1] - 1])  # add it to loop point list
            self.topoMap[nxt[0]][nxt[1]] = nxt[2]  # Mute the point, if not Y stem
            if nxt[0] == L and nxt[1] == P:  # check if loop complete
                follow = False
            elif nxt[0] == cur[0] and nxt[1] == cur[1]:  # check if line cannot be detected
                follow = False
            prv = cur
            cur = nxt
        return loop

    def _findNextWlPoint(self, lC, pC, cl, cp, pl, pp):
        dl = cl - pl
        dp = cp - pp
        num = 0
        i = 3
        s = 0
        mtch = 0
        found = False
        while mtch < 8:  # check all 8 points around current point
            if lC[i] == dl:
                if pC[i] == dp:
                    s = i - 3
                    found = True
                    # Check for y branch where current point is connection between branches
                    for y in range(1, mtch):
                        if lC[i+y] == dl:
                            if pC[i+y] == dp:
                                num = 1
                                break
                    break
            i += 1
            mtch += 1
        if found == False:
            #self.reportThis("_findNext: No start point found.")
            return [cl, cp, num]

        for r in range(0, 8):
            l = cl + lC[s + r]
            p = cp + pC[s + r]
            if self.topoMap[l][p] == 1:
                return [l, p, num]
        
        #self.reportThis("_findNext: No next pnt found")
        return [cl, cp, num]

    def _loopToGcode(self, obj, layDep, loop):
        # generate the path commands
        output = []
        optimize = obj.Optimize

        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))
        
        # Create frist point
        pnt.x = loop[0].x
        pnt.y = loop[0].y
        pnt.z = layDep
        
        # Position cutter to begin loop
        output.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        output.append(Path.Command('G0', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizRapid}))
        output.append(Path.Command('G1', {'Z': pnt.z, 'F': self.vertFeed}))
        
        lenCLP = len(loop)
        lastIdx = lenCLP - 1
        # Cycle through each point on loop
        for i in range(0, lenCLP):
            if i < lastIdx:
                nxt.x = loop[i + 1].x
                nxt.y = loop[i + 1].y
                nxt.z = layDep
            else:
                optimize = False

            if not optimize or not self.isPointOnLine(FreeCAD.Vector(prev.x, prev.y, prev.z), FreeCAD.Vector(nxt.x, nxt.y, nxt.z), FreeCAD.Vector(pnt.x, pnt.y, pnt.z)):
                output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizFeed}))

            # Rotate point data
            prev.x = pnt.x
            prev.y = pnt.y
            prev.z = pnt.z
            pnt.x = nxt.x
            pnt.y = nxt.y
            pnt.z = nxt.z
        # self.reportThis("points after optimization: " + str(len(output)))

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt.x = pnt.x
        self.layerEndPnt.y = pnt.y
        self.layerEndPnt.z = pnt.z
        
        return output



    def isPointOnLine(self, lineA, lineB, pointP):
        tolerance = 1e-6
        vectorAB = lineB - lineA
        vectorAC = pointP - lineA
        crossproduct = vectorAB.cross(vectorAC)
        dotproduct = vectorAB.dot(vectorAC)

        if crossproduct.Length > tolerance:
            return False

        if dotproduct < 0:
            return False

        if dotproduct > vectorAB.Length * vectorAB.Length:
            return False

        return True

    def holdStopCmds(self, obj, zMax, pd, p2, txt):
        cmds = []
        msg = 'N (' + txt + ')'
        cmds.append(Path.Command(msg, {}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'Z': zMax, 'F': self.vertRapid}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'X': p2.x, 'Y': p2.y, 'F': self.horizRapid}))  # horizontal rapid to current XY coordinate
        if zMax != pd:
            cmds.append(Path.Command('G0', {'Z': pd, 'F': self.vertRapid}))  # drop cutter down rapidly to prevDepth depth
            cmds.append(Path.Command('G0', {'Z': p2.z, 'F': self.vertFeed}))  # drop cutter down to current Z depth, returning to normal cut path and speed
        return cmds

    def holdStopEndCmds(self, obj, p2, txt):
        cmds = []
        msg = 'N (' + txt + ')'
        cmds.append(Path.Command(msg, {}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))  # Raise cutter rapid to zMax in line of travel
        # cmds.append(Path.Command('G0', {'X': p2.x, 'Y': p2.y, 'F': self.horizRapid}))  # horizontal rapid to current XY coordinate
        return cmds

    def subsectionCLP(self, CLP, xmin, ymin, xmax, ymax):
        # This function returns a subsection of the CLP scan, limited to the min/max values supplied
        section = []
        lenCLP = len(CLP)
        for i in range(0, lenCLP):
            if CLP[i].x < xmax:
                if CLP[i].y < ymax:
                    if CLP[i].x > xmin:
                        if CLP[i].y > ymin:
                            section.append(CLP[i])
        return section

    def getMaxHeight(self, finalDepth, p1, p2, cutter, CLP):
        # This function connects two HOLD points with line
        # Each point within the subsection point list is tested to determinie if it is under cutter
        # Points determined to be under the cutter on line are tested for z height
        # The highest z point is the requirement for clearance between p1 and p2, and returned as zMax with 2 mm extra
        dx = (p2.x-p1.x)
        if dx == 0.0:
            dx = 0.00001
        m = (p2.y-p1.y) / dx
        b = p1.y-(m*p1.x)

        avoidTool = round(cutter * 0.75, 1)  # 1/2 diam. of cutter is theoretically safe, but 3/4 diam is used for extra clearance
        zMax = finalDepth
        lenCLP = len(CLP)
        for i in range(0, lenCLP):
            mSqrd = m**2
            if mSqrd < 0.0000001:
                mSqrd = 0.0000001
            perpDist = math.sqrt((CLP[i].y - (m * CLP[i].x) - b)**2 / (1 + 1 / (mSqrd)))
            if perpDist < avoidTool: # if point within cutter reach on line of travel, test z height and update as needed
                if CLP[i].z > zMax:
                    zMax = CLP[i].z
        return zMax + 2.0

    def holdStopPerpCmds(self, obj, zMax, pd, p2, aor, ang, txt):
        cmds = []
        msg = 'N (' + txt + ')'
        cmds.append(Path.Command(msg, {}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'Z': zMax, 'F': self.vertRapid}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'X': p2.x, 'Y': p2.y, 'F': self.horizRapid}))  # horizontal rapid to current XY coordinate
        if zMax != pd:
            cmds.append(Path.Command('G0', {'Z': pd, 'F': self.vertRapid}))  # drop cutter down rapidly to prevDepth depth
            cmds.append(Path.Command('G0', {'Z': p2.z, aor: ang, 'F': self.vertFeed}))  # drop cutter down to current Z depth, returning to normal cut path and speed
        return cmds

    def reportThis(self, txt):
        self.opReport += "\n" + txt

    def resetOpVariables(self):
        # reset operation variables
        self.opReport = ""
        self.cutter = None
        self.holdPoint = None
        self.stl = None
        self.layerEndPnt = None
        self.onHold = False
        self.useTiltCutter = False
        self.holdStartPnts = []
        self.holdStopPnts = []
        self.holdStopTypes = []
        self.holdZMaxVals = []
        self.holdPrevLayerVals = []
        self.gcodeCmds = []
        self.CLP = []
        self.SafeHeightOffset = 2.0
        self.ClearHeightOffset = 4.0
        self.layerEndIdx = 0.0
        self.layerEndzMax = 0.0
        self.resetTolerance = 0.0
        self.holdPntCnt = 0
        self.startTime = 0.0
        self.endTime = 0.0
        self.lineCNT = 0
        self.keepTime = 0.0
        self.lineScanTime = 0.0
        self.bbRadius = 0.0
        self.targetDepth = 0.0
        self.stepDeg = 0.0
        self.stepRads = 0.0
        self.axialFeed = 0.0
        self.axialRapid = 0.0
        self.FinalDepth = 0.0
        self.cutOut = 0.0
        self.clearHeight = 0.0
        self.safeHeight = 0.0
        return True

    def setOclCutter(self, obj):
        # Set cutter details
        #  https://www.freecadweb.org/api/dd/dfe/classPath_1_1Tool.html#details
        diam_1 = obj.ToolController.Tool.Diameter
        lenOfst = obj.ToolController.Tool.LengthOffset
        FR = obj.ToolController.Tool.FlatRadius
        CEH = obj.ToolController.Tool.CuttingEdgeHeight

        if obj.ToolController.Tool.ToolType == 'EndMill':
            # Standard End Mill
            self.cutter = ocl.CylCutter(diam_1, (CEH + lenOfst))
        
        elif obj.ToolController.Tool.ToolType == 'BallEndMill' and FR == 0.0:
            # Standard Ball End Mill
            # OCL -> BallCutter::BallCutter(diameter, length)
            self.cutter = ocl.BallCutter(diam_1, (diam_1 / 2 + lenOfst))
            self.useTiltCutter = True
        
        elif obj.ToolController.Tool.ToolType == 'BallEndMill' and FR > 0.0:
            # Bull Nose or Corner Radius cutter
            # Reference: https://www.fine-tools.com/halbstabfraeser.html
            # OCL -> BallCutter::BallCutter(diameter, length)
            self.cutter = ocl.BullCutter(diam_1, FR, (CEH + lenOfst))
        
        elif obj.ToolController.Tool.ToolType == 'Engraver' and FR > 0.0:
            # Bull Nose or Corner Radius cutter
            # Reference: https://www.fine-tools.com/halbstabfraeser.html
            # OCL -> ConeCutter::ConeCutter(diameter, angle, lengthOffset)
            self.cutter = ocl.ConeCutter(diam_1, (obj.ToolController.Tool.CuttingEdgeAngle / 2), lenOfst)

        elif obj.ToolController.Tool.ToolType == 'ChamferMill':
            # Bull Nose or Corner Radius cutter
            # Reference: https://www.fine-tools.com/halbstabfraeser.html
            # OCL -> ConeCutter::ConeCutter(diameter, angle, lengthOffset)
            self.cutter = ocl.ConeCutter(diam_1, (obj.ToolController.Tool.CuttingEdgeAngle / 2), lenOfst)

        # http://www.carbidecutter.net/products/carbide-burr-cone-shape-sm.html
        '''
            return "Drill";
            return "CenterDrill";
            return "CounterSink";
            return "CounterBore";
            return "FlyCutter";
            return "Reamer";
            return "Tap";
            return "EndMill";
            return "SlotCutter";
            return "BallEndMill";
            return "ChamferMill";
            return "CornerRound";
            return "Engraver";
            return "Undefined";
        '''
        return True

    def pocketInvertExtraOffset(self):
        return True

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ... initialize defaults'''

        obj.StepOver = 100
        obj.Optimize = True
        obj.IgnoreWaste = False
        obj.ReleaseFromWaste = False
        obj.LayerMode = 'Single-pass'
        obj.ScanType = 'Planar'
        obj.RotationAxis = 'X'
        obj.CutMode = 'Conventional'
        obj.CutPattern = 'ZigZag'
        obj.CutterTilt = 0.0
        obj.StartIndex = 0.0
        obj.StopIndex = 360.0
        obj.SampleInterval = 1.0

        # need to overwrite the default depth calculations for facing
        job = PathUtils.findParentJob(obj)
        if job:
            if job.Stock:
                d = PathUtils.guessDepths(job.Stock.Shape, None)
                print("job.Stock exists")
            else:
                print("job.Stock NOT exist")
        else:
            print("job NOT exist")
        
        if self.docRestored == True:  # This op is NOT the first in the Operations list
            print("doc restored")
            obj.FinalDepth.Value = obj.OpFinalDepth.Value
        else:
            print("new operation")
            obj.OpFinalDepth.Value = d.final_depth
            obj.OpStartDepth.Value = d.start_depth
            if self.initOpFinalDepth == None and self.initFinalDepth == None:
                 self.initFinalDepth = d.final_depth
                 self.initOpFinalDepth = d.final_depth
            else:
                print("-initFinalDepth" + str(self.initFinalDepth))
                print("-initOpFinalDepth" + str(self.initOpFinalDepth))
        obj.IgnoreWasteDepth = obj.FinalDepth.Value + 0.001



def SetupProperties():
    setup = []
    setup.append("Algorithm")
    setup.append("DropCutterDir")
    setup.append("BoundBox")
    setup.append("StepOver")
    setup.append("DepthOffset")
    setup.append("LayerMode")
    setup.append("ScanType")
    setup.append("RotationAxis")
    setup.append("CutMode")
    setup.append("SampleInterval")
    setup.append("StartIndex")
    setup.append("StopIndex")
    setup.append("CutterTilt")
    setup.append("CutPattern")
    setup.append("IgnoreWasteDepth")
    setup.append("IgnoreWaste")
    setup.append("ReleaseFromWaste")
    return setup

def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Surface operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectSurface(obj, name)
    return obj
