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
# *   Version: Rev. 3m - Usable                                             *
# *                                                                         *
# ***************************************************************************
# Revision Notes
### - Implement fixed step over for rotational surface operation.
### - Prepare to implement necessary tilt of 18 deg, recommended by JoshM, for ball end mills
### - Implement rotational angle start/stop values
### - Begin to implement cut patterns: zigzag, line (line is used to force cut modes: climb, conventional)

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
__contributors__ = "roivai[FreeCAD], russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of Mill Facing operation."
__scriptVersion__ = "3m Usable"
__created__ = "2019-03-29"
__lastModified__ = "2019-03-30 00:19 CST"

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

    initFinalDepth = 0.0
    initStartDepth = 0.0

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
        obj.addProperty("App::PropertyBool", "CutModeForced", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Force specified cut mode. Default is a quicker zig-zag pattern."))
        obj.addProperty("App::PropertyEnumeration", "CutPattern", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Clearing pattern to use"))
        obj.addProperty("App::PropertyEnumeration", "RotationAxis", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "The model will be rotated around this axis."))
        obj.addProperty("App::PropertyEnumeration", "MultipassStepOverSize", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "In mult-pass layer mode: fixed or diminishing step over at each layer depth."))
        obj.addProperty("App::PropertyFloat", "StartIndex", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "Start index(angle) for rotational scan"))
        obj.addProperty("App::PropertyFloat", "StopIndex", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "Stop index(angle) for rotational scan"))
        obj.addProperty("App::PropertyFloat", "CutterTilt", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "Stop index(angle) for rotational scan"))
        obj.addProperty("App::PropertyPercent", "StepOver", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Step over percentage of the drop cutter path"))
        obj.addProperty("App::PropertyDistance", "DepthOffset", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Z-axis offset from the surface of the object"))
        #obj.addProperty("App::PropertyFloatConstraint", "SampleInterval", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "The Sample Interval. Small values cause long wait times"))
        obj.addProperty("App::PropertyFloat", "SampleInterval", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "The Sample Interval. Small values cause long wait times"))
        obj.addProperty("App::PropertyBool", "Optimize", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable optimization which removes unnecessary points from G-Code output"))
        obj.addProperty("App::PropertyBool", "IgnoreWaste", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Ignore areas that proceed below specified depth."))
        obj.addProperty("App::PropertyFloat", "IgnoreWasteDepth", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Depth used to identify ignore area."))

        obj.CutMode = ['Conventional', 'Climb']
        obj.BoundBox = ['BaseBoundBox', 'Stock']
        obj.DropCutterDir = ['X', 'Y']
        obj.Algorithm = ['OCL Dropcutter', 'OCL Waterline']
        #obj.SampleInterval = (0.04, 0.01, 1.0, 0.01)
        obj.LayerMode = ['Single-pass', 'Multi-pass']
        obj.ScanType = ['Planar', 'Rotational']
        obj.RotationAxis = ['X', 'Y']
        obj.MultipassStepOverSize = ['Diminishing', 'Fixed']
        #obj.CutPattern = ['ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']
        obj.CutPattern = ['ZigZag', 'Line']

        if not hasattr(obj, 'DoNotSetDefaultValues'):
            self.setEditorProperties(obj)

    def setEditorProperties(self, obj):
        # Used to hide inputs in properties list
        if obj.Algorithm == 'OCL Dropcutter':
            obj.setEditorMode('DropCutterDir', 0)
            obj.setEditorMode('DropCutterExtraOffset', 0)
            #obj.LayerMode = 'Single-pass'
            '''
            obj.setEditorMode('ScanType', 0)
            obj.setEditorMode('CutPattern', 0)
            if obj.ScanType == 'Rotational':
                obj.setEditorMode('RotationAxis', 0)
                obj.setEditorMode('MultipassStepOverSize', 0)
                obj.setEditorMode('CutterTilt', 0)
                obj.setEditorMode('StopIndex', 0)
                obj.setEditorMode('StartIndex', 0)
                obj.setEditorMode('IgnoreWaste', 2)
                obj.setEditorMode('IgnoreWasteDepth', 2)
            else:
                obj.setEditorMode('IgnoreWaste', 0)
                obj.setEditorMode('IgnoreWasteDepth', 0)
                obj.setEditorMode('RotationAxis', 2)
                obj.setEditorMode('MultipassStepOverSize', 2)
                obj.setEditorMode('CutterTilt', 2)
                obj.setEditorMode('StopIndex', 2)
                obj.setEditorMode('StartIndex', 2)
            '''
            nn=None
        else:
            obj.setEditorMode('DropCutterDir', 2)
            obj.setEditorMode('DropCutterExtraOffset', 2)
            obj.LayerMode = 'Multi-pass'
            '''
            obj.setEditorMode('ScanType', 2)
            obj.setEditorMode('RotationAxis', 2)
            obj.setEditorMode('MultipassStepOverSize', 2)
            obj.setEditorMode('IgnoreWaste', 2)
            obj.setEditorMode('IgnoreWasteDepth', 2)
            obj.setEditorMode('CutterTilt', 2)
            obj.setEditorMode('StopIndex', 2)
            obj.setEditorMode('StartIndex', 2)
            '''
            nn=None

    def onChanged(self, obj, prop):
        if prop == "Algorithm":
            self.setEditorProperties(obj)

    def opOnDocumentRestored(self, obj):
        self.setEditorProperties(obj)

    def opExecute(self, obj):
        '''opExecute(obj) ... process surface operation'''
        PathLog.track()
        # Instantiate additional class operation variables
        rtn = self.resetOpVariables()
        
        self.reportThis("\n-----\n-----\nBegin 3D surface operation")
        self.reportThis("Script version: " + __scriptVersion__ + "  Lm: " + __lastModified__)

        # Each operation within a job needs a dedicated location within the JOB list/dictionary.
        # Each time an operation is changed or recalculated, that operation's entry should be cleared and assigned the new data
        # del self.commandlist
        #self.commandlist = []

        # mark beginning of operation
        self.startTime = time.time()

        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment) + ')\n'

        output += "(" + obj.Label + ")"
        output += "(Compensated Tool Path. Diameter: " + str(obj.ToolController.Tool.Diameter) + ")"
        
        parentJob = PathUtils.findParentJob(obj)
        if parentJob is None:
            self.reportThis("No parentJob")
            return
        
        # Set cutter for OCL based on tool controller properties
        self.setOclCutter(obj)

        if self.initFinalDepth != obj.OpFinalDepth.Value:
            if obj.OpFinalDepth.Value == obj.FinalDepth.Value:
                obj.FinalDepth.Value = self.initFinalDepth
                obj.OpFinalDepth.Value = self.initFinalDepth
        self.reportThis("Final Depth: " + str(obj.FinalDepth.Value))

        # Limit StepOver to natural number percentage
        if obj.StepOver > 100:
            obj.StepOver = 100
        if obj.StepOver < 1:
            obj.StepOver = 1
        
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
        if obj.CutterTilt < -45.0:
            obj.CutterTilt = -45.0
        if obj.CutterTilt > 45.0:
            obj.CutterTilt = 45.0

        # Limit sample interval
        if obj.SampleInterval < 0.01:
            obj.SampleInterval = 0.01
        if obj.SampleInterval > 5.0:
            obj.SampleInterval = 5.0

        self.cutOut = (self.cutter.getDiameter() * (float(obj.StepOver) / 100.0))
        self.reportThis("Cut out: " + str(self.cutOut))

        # Cycle through parts of model
        for base in self.model:
            final = []
            self.reportThis("BASE object: " + str(base.Name))

            if base.TypeId.startswith('Mesh'):
                mesh = base.Mesh
            else:
                # try/except is for Path Jobs created before GeometryTolerance
                try:
                    deflection = parentJob.GeometryTolerance
                except AttributeError:
                    import PathScripts.PathPreferences as PathPreferences
                    deflection = PathPreferences.defaultGeometryTolerance()
                base.Shape.tessellate(0.5)
                mesh = MeshPart.meshFromShape(base.Shape, Deflection=deflection)
            
            # Set cut mode
            bb = parentJob.Stock.Shape.BoundBox
            if hasattr(obj, 'BoundBox'):
                if obj.BoundBox == "BaseBoundBox":
                    bb = mesh.BoundBox
            
            # Objective is to remove material from surface in StepDown layers rather than one pass to FinalDepth
            final = []
            if obj.Algorithm == 'OCL Dropcutter':
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
                        self.FinalDepth = 0.0000001
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

                    if obj.RotationAxis == 'X':
                        if obj.DropCutterDir == 'X':
                            final = self._indexedDropCutterOp(obj, parentJob, stl, bb)
                        else:
                            final = self._rotationalDropCutterOp(obj, parentJob, stl, bb)
                    else:
                        if obj.DropCutterDir == 'Y':
                            final = self._indexedDropCutterOp(obj, parentJob, stl, bb)
                        else:
                            final = self._rotationalDropCutterOp(obj, parentJob, stl, bb)
                    
                elif obj.ScanType == 'Planar':
                    final = self._planarDropCutOp(obj, stl, bb)
            elif obj.Algorithm == 'OCL Waterline':
                self.reportThis("--CutMode: " + str(obj.CutMode))
                final = self._waterlineOp(obj, mesh, bb)
            # End IF
            # Send final list of commands to operation object
            self.commandlist.extend(final)

        self.endTime = time.time()
        self.reportThis("OPERATION time: " + str(self.endTime - self.startTime) + " sec.")
        print(self.opReport)
        # reset operation variables



    def _planarDropCutOp(self, obj, stl, bb):
        t_before = time.time()
        # Determine bounding box length
        bbLength = bb.YLength
        if obj.DropCutterDir == 'Y':
            bbLength = bb.XLength

        # Prepare global holdpoint container
        if self.holdPoint == None:
            self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Set crop cutter extra offset
        cdeoX = 0
        cdeoY = 0
        if hasattr(obj, 'DropCutterExtraOffset'):
            cdeoX = obj.DropCutterExtraOffset.x
            cdeoY = obj.DropCutterExtraOffset.y
        else:
            self.reportThis("DropCutterExtraOffset property does NOT exist.")

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

        # Scan the piece to depth
        self.CLP = self._planarDropCutScan(obj, stl, bbLength, xmin, ymin, xmax, ymax, depthparams[lenDP-1])
        if obj.DepthOffset.Value != 0:
            self.reportThis("--Applying DepthOffest")
            for pt in range(0, len(self.CLP)):
                self.CLP[pt].z += obj.DepthOffset.Value
        self.reportThis("--Elapsed time after OCL scan is " + str(time.time() - t_before) + " s")

        # Extract layers per depthparams
        for lyr in  range(0, lenDP):
            # Determine next depth
            if lyr < lenDP - 1:
                nextDepth = depthparams[lyr + 1]                    
            # Convert current layer data to gcode
            self._planarScanToGcode(obj, lyr, prevDepth, depthparams[lyr], self.CLP)
            prevDepth = depthparams[lyr]
        self.reportThis("--Elapsed time after layers to gcode is " + str(time.time() - t_before) + " s")  # self.keepTime
        
        commands = self._processPlanarHolds(obj)
        self.reportThis("--Elapsed time after processing gcode holds is " + str(time.time() - t_before) + " s")  # self.keepTime
        return commands

    def _planarDropCutScan(self, obj, stl, bbLength, xmin, ymin, xmax, ymax, fd):
        t_before = time.time()
        
        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(stl)
        pdc.setCutter(self.cutter)
        pdc.setZ(fd)  # set minimumZ (final / target depth value)
        pdc.setSampling(obj.SampleInterval)

        path = ocl.Path()                   # create an empty path object

        cOut = self.cutOut
        notCutting = (self.cutter.getDiameter() * ((100.0 - float(obj.StepOver)) / 100.0))
        if obj.DropCutterDir == 'X':
            Nl = int(math.ceil((bbLength + (2 * obj.DropCutterExtraOffset.y)) / cOut)) # Number of lines
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

                lo = self.cutPatternLine(obj, n, p1, p2)
                path.append(lo)        # add the line to the path
        else:
            Nl = int(math.ceil((bbLength + (2 * obj.DropCutterExtraOffset.x)) / cOut)) # Number of lines
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

                lo = self.cutPatternLine(obj, n, p1, p2)
                path.append(lo)        # add the line to the path

        pdc.setPath(path)

        # run drop-cutter on the path
        pdc.run()
        self.reportThis("--OCL scan took " + str(time.time() - t_before) + " s")

        clp = pdc.getCLPoints()
        self.pntsPerLine = len(clp) / Nl
        if self.pntsPerLine - math.floor(self.pntsPerLine) != 0.0:
            self.pntsPerLine = None

        # return the list the points
        return clp

    def _planarScanToGcode(self, obj, lc, prvDep, layDep, CLP):
        output = []
        optimize = obj.Optimize
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
        firstDrop = True
        beginCmdFlag = True

        def makePnt(pnt):
            p = ocl.Point(float("inf"), float("inf"), float("inf"))
            p.x = pnt.x
            p.y = pnt.y
            p.z = pnt.z
            return p
        
        def beginLayerCommand(pnt, lc, firstDrop):
            # Send cutter to starting position(first point)
            begcmd = []
            begcmd.append(Path.Command('N (Beginning of layer ' + str(lc) + ')', {}))
            begcmd.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))  # Z was set to obj.ClearanceHeight.Value
            begcmd.append(Path.Command('G0', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizRapid}))
            if firstDrop == True:
                begcmd.append(Path.Command('G1', {'Z': pnt.z, 'F': self.vertFeed}))
            begcmd.reverse()
            return begcmd
        
        # Create containers for x,y,z points
        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))
        travVect = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Set values for first gcode point in layer
        pnt.x = CLP[0].x
        pnt.y = CLP[0].y
        pnt.z = CLP[0].z
        if CLP[0].z < layDep:
            pnt.z = layDep
        elif CLP[0].z > prvDep:
            firstDrop = False
            if obj.LayerMode == 'Single-pass':
                firstDrop = True

        if lc > -1:
            bLC = beginLayerCommand(pnt, lc, firstDrop)
            for blc in bLC:
                output.insert(0, blc)
            beginCmdFlag = False

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
                lineOfTravel = "O"

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
            
            # Update zMin and zMax values
            if pnt.z < zMin:
                zMin = pnt.z
            if pnt.z > zMax:
                zMax = pnt.z
            
            if obj.LayerMode == 'Multi-pass':
                # if z travels above previous layer, start/continue hold high cycle
                if pnt.z > prvDep:  
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
                    holdLine = self.lineCNT  # remember holdLine
                    self.holdStartPnts.append(makePnt(pnt))
                    self.holdPrevLayerVals.append(prvDep)
                    holdStart = False  # cancel holdStart

                # hold cutter high until Z value drops below prvDep
                if pnt.z <= prvDep:
                    holdStop = True
                
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
        for o in output:
            self.gcodeCmds.append(o)
        self.gcodeCmds.append(Path.Command('N (End of layer ' + str(lc) + ')', {}))
        #return output

    def _processPlanarHolds(self, obj):    
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
                        subCLP = self.subsectionCLP(self.CLP, xmin, ymin, xmax, ymax)
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



    def _indexedDropCutterOp(self, obj, parentJob, stl, bb):
        # Objective is to remove material from surface in StepDown layers rather than one pass to Final Depth
        commands = []
        eT = time.time()
        bbRad = self.bbRadius
        bbCircum = 2 * math.pi * bbRad

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams =  [self.FinalDepth]
        else:
            dep_par = PathUtils.depth_params(self.clearHeight, self.safeHeight, self.bbRadius, obj.StepDown.Value, 0.0, self.FinalDepth)
            depthparams = [i for i in dep_par]
        prevDepth = depthparams[0]
        lenDP = len(depthparams)


        # Set drop cutter extra offset
        cdeoX = 0
        cdeoY = 0
        if hasattr(obj, 'DropCutterExtraOffset'):
            cdeoX = obj.DropCutterExtraOffset.x
            cdeoY = obj.DropCutterExtraOffset.y
        else:
            self.reportThis("DropCutterExtraOffset property does NOT exist.")

        # Set updated bound box values
        # Redefine the new max and min XY area of the operation based on greatest point radius of model
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

        # Begin gcode operation with raising cutter to safe height
        commands.append(Path.Command('G0', {'Z': self.safeHeight, 'F': self.vertRapid}))

        self.reportThis("--Elapsed setup time : " + str(time.time() - eT) + " s")
        lCnt = -1
        # Complete rotational scans at layer and translate into gcode
        for dp in range(0, lenDP):
            layDep = depthparams[dp]
            lCnt += 1

            # Compute circumference and step angles for current layer
            layCircum = 2 * math.pi * layDep  # 2 * math.pi * bbRad
            if lenDP == 1:
                layCircum = 2 * math.pi * bbRad
            self.stepDeg = (self.cutOut / layCircum) * 360.0
            self.stepRads = (self.cutOut / layCircum) * 2.0 * math.pi

            # Limit step angle to 45 degrees
            if self.stepDeg > 45.0:
                self.stepDeg = 45.0
                self.stepRads = 45.0 * math.pi / 180.0

            # Determine rotational index angles [indexes]
            scanAngleIndexes = []  # Reset for each step down layer
            begIdx = obj.StartIndex
            endIdx = obj.StopIndex
            if endIdx < begIdx:
                begIdx -= 360.0
            angIdx = begIdx
            while angIdx < endIdx:
                scanAngleIndexes.append(angIdx)
                angIdx += self.stepDeg
            if endIdx == 360.0:
                scanAngleIndexes.append(360.0)
            else:
                scanAngleIndexes.append(self.stepDeg - (angIdx - endIdx))

            # Set axial feed rates
            self.axialFeed = 360 / layCircum * self.horizFeed
            self.axialRapid = 360 / layCircum * self.horizRapid

            # Perform rotational indexed scans to layer depth
            t_before = time.time()
            iDCS = []
            iSTG = []
            lenSAI = len(scanAngleIndexes)
            prevIndex = 0.0
            resetRads = 0.0
            for sa in range(0, lenSAI):
                radsRot = math.radians((scanAngleIndexes[sa] - prevIndex)) - 0.0000001
                # Rotate STL object using OCL method
                if obj.RotationAxis == 'X':
                    rStl = stl.rotate(radsRot, 0.0, 0.0)
                else:
                    rStl = stl.rotate(0.0, radsRot, 0.0)
                # get scan at index
                iDCS = self._indexedDropCutScan(obj, stl, xmin, ymin, xmax, ymax, layDep, sa)
                
                idx = scanAngleIndexes[sa]
                if obj.DropCutterDir == 'Y':
                    idx = -1.0 * idx

                iSTG = self._indexedScanToGcode(obj, sa, iDCS, idx, prevDepth, layDep, lenDP)
                commands.extend(iSTG)
                resetRads -= radsRot
                prevIndex = scanAngleIndexes[sa]
            # End loop

            # Rotate STL object back to starting position, using OCL method
            if lenDP > 1:
                if obj.RotationAxis == 'X':
                    rStl = stl.rotate(resetRads, 0.0, 0.0)
                else:
                    rStl = stl.rotate(0.0, resetRads, 0.0)

            self.reportThis("--Total time for " + str(lenSAI) + " OCL scans: " + str(time.time() - t_before) + " s")
            prevDepth = layDep
            self.reportThis("--Elapsed time after layer " + str(lCnt) +" is " + str(time.time() - eT) + " s")
        #Eol

        return commands

    def _indexedDropCutScan(self, obj, stl, xmin, ymin, xmax, ymax, ld, n):
        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(stl)
        pdc.setCutter(self.cutter)
        pdc.setZ(ld)  # set minimumZ (final / target depth value)
        pdc.setSampling(obj.SampleInterval)

        path = ocl.Path()                   # create an empty path object

        cutterOfst = 0.0
        if self.useTiltCutter == True:
            cutterOfst = ld * math.sin(obj.CutterTilt) * math.pi / 180.0

        if obj.RotationAxis == 'X':
            # add Line objects to the path in this loop
            p1 = ocl.Point(xmin, cutterOfst, 0.0)   # start-point of line
            p2 = ocl.Point(xmax, cutterOfst, 0.0)   # end-point of line
        else:
            # add Line objects to the path in this loop
            p1 = ocl.Point(cutterOfst, ymin, 0.0)   # start-point of line
            p2 = ocl.Point(cutterOfst, ymax, 0.0)   # end-point of line
        
        lo = self.cutPatternLine(obj, n, p1, p2)
        path.append(lo)        # add the line to the path

        pdc.setPath(path)

        # run drop-cutter on the path
        pdc.run()

        # return the list the points
        return pdc.getCLPoints()

    def _indexedScanToGcode(self, obj, li, CLP, idxAng, prvDep, layerDepth, numDeps):
        # generate the path commands
        output = []
        optimize = obj.Optimize
        onHold = False
        holdCount = 0
        holdLine = 0
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
                if pnt.z > prvDep:  
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



    def _rotationalDropCutterOp(self, obj, parentJob, stl, bb):
        # Objective is to remove material from surface in StepDown layers rather than one pass to Final Depth
        commands = []
        eT = time.time()
        bbRad = self.bbRadius
        bbCircum = 2 * math.pi * bbRad

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams =  [self.FinalDepth]
        else:
            dep_par = PathUtils.depth_params(self.clearHeight, self.safeHeight, self.bbRadius, obj.StepDown.Value, 0.0, self.FinalDepth)
            depthparams = [i for i in dep_par]
        prevDepth = depthparams[0]
        lenDP = len(depthparams)

        # Set drop cutter extra offset
        cdeoX = 0
        cdeoY = 0
        if hasattr(obj, 'DropCutterExtraOffset'):
            cdeoX = obj.DropCutterExtraOffset.x
            cdeoY = obj.DropCutterExtraOffset.y
        else:
            self.reportThis("DropCutterExtraOffset property does NOT exist.")

        # Set updated bound box values
        # Redefine the new max and min XY area of the operation based on greatest point radius of model
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
        self.reportThis("--Elapsed setup time : " + str(time.time() - eT) + " s")

        
        # Complete rotational scans at layer and translate into gcode
        for dp in range(0, lenDP):
            layDep = depthparams[dp]

            # Compute circumference and step angles for current layer
            layCircum = 2 * math.pi * layDep
            if lenDP == 1:
                layCircum = 2 * math.pi * bbRad
            self.stepDeg = (obj.SampleInterval / layCircum) * 360.0
            self.stepRads = (obj.SampleInterval / layCircum) * 2.0 * math.pi

            # Limit step angle to 45 degrees
            #if self.stepDeg > 45.0:
            #    self.stepDeg = 45.0
            #   self.stepRads = 45.0 * math.pi / 180.0

            # Determine rotational index angles [indexes]
            scanAngleIndexes = []  # Reset for each step down layer
            begIdx = obj.StartIndex
            endIdx = obj.StopIndex
            if endIdx < begIdx:
                begIdx -= 360.0
            angIdx = begIdx
            while angIdx < endIdx:
                scanAngleIndexes.append(angIdx)
                angIdx += self.stepDeg
            if endIdx == 360.0:
                scanAngleIndexes.append(360.0)
            else:
                scanAngleIndexes.append(self.stepDeg - (angIdx - endIdx))

            # Set axial feed rates
            self.axialFeed = 360 / layCircum * self.horizFeed
            self.axialRapid = 360 / layCircum * self.horizRapid

            # Perform rotational indexed scans to layer depth
            t_before = time.time()
            rDCS = []
            rotationalScanLines = []
            lenSAI = len(scanAngleIndexes)
            prevIndex = 0.0
            resetRads = 0.0
            for sa in range(0, lenSAI):
                radsRot = math.radians((scanAngleIndexes[sa] - prevIndex)) - 0.0000001
                # Rotate STL object using OCL method
                if obj.RotationAxis == 'X':
                    rStl = stl.rotate(radsRot, 0.0, 0.0)
                else:
                    rStl = stl.rotate(0.0, radsRot, 0.0)
                
                # get scan at index
                rDCS = self._rotationalDropCutScan(obj, stl, xmin, ymin, xmax, ymax, layDep, sa)
                rotationalScanLines.append(rDCS)
                resetRads -= radsRot
                prevIndex = scanAngleIndexes[sa]
                self.keepTime += time.time() - t_before
            # End loop

            # Rotate STL object back to starting position, using OCL method
            if lenDP > 1:
                if obj.RotationAxis == 'X':
                    rStl = stl.rotate(resetRads, 0.0, 0.0)
                else:
                    rStl = stl.rotate(0.0, resetRads, 0.0)

            # Invert angles for Y-axis rotation
            sAI = []
            if obj.RotationAxis == 'Y':
                for a in scanAngleIndexes:
                    sAI.append(-a)
            else:
                sAI = scanAngleIndexes

            # Begin gcode operation with raising cutter to safe height
            commands.append(Path.Command('G0', {'Z': self.safeHeight, 'F': self.vertRapid}))
            
            # Convert rotational scans into gcode
            numPnts = len(rotationalScanLines[0])  # Number of points per line along axis, at obj.SampleInterval spacing
            for np in range(0, numPnts):
                pointRing = []
                for line in rotationalScanLines:  # extract circular set(ring) of points from scan lines
                    pointRing.append(line[np])
                rSTG = self._rotationalScanToGcode(obj, pointRing, np, prevDepth, layDep, sAI)
                commands.extend(rSTG)
            prevDepth = layDep
        #Eol

        return commands

    def _rotationalDropCutScan(self, obj, stl, xmin, ymin, xmax, ymax, fd, idxnum):
        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(stl)
        pdc.setCutter(self.cutter)
        pdc.setZ(fd)  # set minimumZ (final / target depth value)
        # pdc.setSampling(obj.SampleInterval) 
        pdc.setSampling(self.cutOut)  # cutOut is the value to advance along the axis of rotation

        path = ocl.Path()                   # create an empty path object

        if obj.RotationAxis == 'X':
            # add Line objects to the path in this loop
            p1 = ocl.Point(xmin, 0.0, 0.0)   # start-point of line
            p2 = ocl.Point(xmax, 0.0, 0.0)   # end-point of line
        else:
            # add Line objects to the path in this loop
            p1 = ocl.Point(0.0, ymin, 0.0)   # start-point of line
            p2 = ocl.Point(0.0, ymax, 0.0)   # end-point of line
        
        if obj.CutMode == 'Conventional':
            l = ocl.Line(p1, p2)     # create line-object
        else:
            l = ocl.Line(p2, p1)     # line-object

        path.append(l)        # add the line to the path
        pdc.setPath(path)  # set the completed path object

        # run drop-cutter on the path
        pdc.run()
        # return the list the points
        return pdc.getCLPoints()

    def _rotationalScanToGcode(self, obj, PNTS, np, prvDep, layDep, sAI):
        # generate the path commands
        output = []
        nxtAng = 0
        adjRotAng = 0
        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))

        def adjustRotation(ang, adjRotAng):
            ang += adjRotAng
            if ang < 0:
                ang += 360
            elif ang > 360:
                ang -= 360
            return ang

        # Rotate to correct index location
        axisOfRot = 'A'
        if obj.CutMode == 'Climb':  # Change to 'Climb' if actual rotation is reversed
        # if obj.DropCutterDir == 'X':
            axisOfRot = 'B'
        
        # Create frist point
        ang = sAI[0]
        pnt.x = PNTS[0].x
        pnt.y = PNTS[0].y
        pnt.z = PNTS[0].z + float(obj.DepthOffset.Value)
        
        # Adjust feed rate based on radius/circumferance of cutter.
        # Original feed rate based on travel at circumferance.
        if np > 0:
            if pnt.z > self.layerEndPnt.z:
                clrZ = pnt.z + 2.0
                output.append(Path.Command('G1', {'Z': clrZ, 'F': self.vertRapid}))
        else:
            output.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))
        output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, axisOfRot: ang, 'F': self.axialFeed}))
        output.append(Path.Command('G1', {'Z': pnt.z, 'F': self.axialFeed}))
        
        lenPNTS = len(PNTS)
        lastIdx = lenPNTS - 1
        for i in range(0, lenPNTS):
            if i < lastIdx:
                nxtAng = sAI[i + 1]
                nxt.x = PNTS[i + 1].x
                nxt.y = PNTS[i + 1].y
                nxt.z = PNTS[i + 1].z + float(obj.DepthOffset.Value)

            output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, axisOfRot: ang, 'F': self.axialFeed}))
            pnt.x = nxt.x
            pnt.y = nxt.y
            pnt.z = nxt.z
            ang = nxtAng
        # self.reportThis("points after optimization: " + str(len(output)))

        # Save layer end point for use in transitioning to next layer
        endang = sAI[0]
        self.layerEndPnt.x = PNTS[0].x
        self.layerEndPnt.y = PNTS[0].y
        self.layerEndPnt.z = PNTS[0].z
        
        # Move cutter to final point
        # output.append(Path.Command('G1', {'X': self.layerEndPnt.x, 'Y': self.layerEndPnt.y, 'Z': self.layerEndPnt.z, axisOfRot: endang, 'F': self.axialFeed}))

        return output



    def _waterlineOp(self, obj, mesh, bb):
        self.keepTime = time.time()
        commands = []
        # Determine bounding box length
        bbLength = bb.YLength

        # Prepare global holdpoint and layerEndPnt containers
        if self.holdPoint == None:
            self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))
        if self.layerEndPnt == None:
            self.layerEndPnt = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Create stl object for ocl
        stl = ocl.STLSurf()
        for f in mesh.Facets:
            p = f.Points[0]
            q = f.Points[1]
            r = f.Points[2]
            # offset the triangle in Z with DepthOffset
            t = ocl.Triangle(ocl.Point(p[0], p[1], p[2] + obj.DepthOffset.Value),
                            ocl.Point(q[0], q[1], q[2] + obj.DepthOffset.Value),
                            ocl.Point(r[0], r[1], r[2] + obj.DepthOffset.Value))
            aT = stl.addTriangle(t)

        # Set crop cutter extra offset
        cdeoX = 0
        cdeoY = 0
        if hasattr(obj, 'DropCutterExtraOffset'):
            cdeoX = obj.DropCutterExtraOffset.x
            cdeoY = obj.DropCutterExtraOffset.y
        else:
            self.reportThis("DropCutterExtraOffset property does NOT exist.")

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams =  [obj.FinalDepth.Value]
        else:
            dep_par = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown.Value, 0.0, obj.FinalDepth.Value)
            depthparams = [i for i in dep_par]
        lenDP = len(depthparams)
        prevDepth = depthparams[0]

        # the max and min XY area of the operation
        xmin = bb.XMin - cdeoX
        xmax = bb.XMax + cdeoX
        ymin = bb.YMin - cdeoY
        ymax = bb.YMax + cdeoY

        smplInt = obj.SampleInterval
        minSampInt = 0.001  # value is mm
        if smplInt < minSampInt:
            smplInt = minSampInt
        self.numScanLines = int(math.ceil(bbLength / smplInt) + 1) # Number of lines

        # Scan the piece to depth at obj.SampleInterval
        self.CLP = self._waterlineDropCutScan(obj, stl, smplInt, xmin, ymin, xmax, ymax, depthparams[lenDP-1])
        self.reportThis("--The scan contains " + str(self.numScanLines) + " lines, with " + str(len(self.CLP[0])) + " points per line.")
        self.reportThis("--The scan and its setup took " + str(time.time() - self.keepTime) + " s.")

        # Extract Wl layers per depthparams
        cmds = []
        loopList = []
        layerFlags = []
        sumTime = time.time()
        # self.reportThis("--Cycle through Wl layers")
        for lyr in  range(0, lenDP):
            # Determine next depth
            if lyr < lenDP - 1:
                nextDepth = depthparams[lyr + 1]
            # Identify layer waterline from OCL scan
            layerFlags = self._identifyWaterline(prevDepth, depthparams[lyr])
            time.sleep(0.1)
            # Extract waterline and convert to gcode
            cmds = self._extractWaterlines(obj, lyr, prevDepth, depthparams[lyr], layerFlags)
            #save commands
            time.sleep(0.1)
            commands.extend(cmds)
            prevDepth = depthparams[lyr]
        self.reportThis("--All layer scans combined took " + str(time.time() - sumTime) + " s")
        return commands

    def _waterlineDropCutScan(self, obj, stl, smplInt, xmin, ymin, xmax, ymax, fd):
        t_before = time.time()
        wSL = []
        #wslCnt = -1

        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(stl)
        pdc.setCutter(self.cutter)
        pdc.setZ(fd)  # set minimumZ (final / target depth value)
        pdc.setSampling(smplInt)

        # add Line objects to the path in this loop
        for n in range(0, self.numScanLines):
            path = ocl.Path()                   # create an empty path object
            y = ymin + (n * smplInt)
            p1 = ocl.Point(xmin, y, 0)   # start-point of line
            p2 = ocl.Point(xmax, y, 0)   # end-point of line

            l = ocl.Line(p1, p2)     # line-object
            path.append(l)        # add the line to the path

            pdc.setPath(path)

            # run drop-cutter on the path
            pdc.run()

            #wslCnt += 1
            #wSL.append([1])
            #wSL[wslCnt] = pdc.getCLPoints()
            wSL.append(pdc.getCLPoints())
        self.keepTime += time.time() - t_before

        # return the list the points
        return wSL

    def _identifyWaterline(self, prevDepth, layDep):
        layerFlags = []                                # Reset flag list
        wlAreas = []
        pntsPerLine = len(self.CLP[0])

        #self.reportThis("--Identify waterline highs and lows")
        for l in range(0, self.numScanLines):
            layerFlags.append([9]) # Add blank line list
            for p in range(0, pntsPerLine):
                if self.CLP[l][p].z > layDep:
                    layerFlags[l].append(2)
                else:
                    layerFlags[l].append(0)
            N = layerFlags[l].pop(0)
        
        # add buffer boarder of zeros to all sides to CLP data
        pre = [0,0]
        post = [0,0]
        for p in range(0, pntsPerLine):
            pre.append(0)
            post.append(0)
        for l in range(0, len(layerFlags)):
            layerFlags[l].insert(0, 0)
            layerFlags[l].append(0)
        layerFlags.insert(0, pre)
        layerFlags.append(post)

        lastPnt = len(layerFlags[1]) - 1
        lastLn = len(layerFlags) - 1

        #self.reportThis("--Convert parallel data to ridges")
        for lin in range(1, lastLn):
            for pt in range(1, lastPnt):  # Ignore first and last points
                if layerFlags[lin][pt] == 0:
                    if layerFlags[lin][pt + 1] == 2:  # step up
                        layerFlags[lin][pt] = 1
                    if layerFlags[lin][pt - 1] == 2: #step down
                        layerFlags[lin][pt] = 1

        highFlag = 0
        extraMaterial = 4
        #self.reportThis("--Convert perpendicular data to ridges and highlight ridges")
        for pt in range(1, lastPnt):  # Ignore first and last points
            for lin in range(1, lastLn):
                if layerFlags[lin][pt] == 0:
                    highFlag = 0
                    if layerFlags[lin + 1][pt] == 2:  # step up
                        layerFlags[lin][pt] = 1
                    if layerFlags[lin - 1][pt] == 2: #step down
                        layerFlags[lin][pt] = 1
                elif layerFlags[lin][pt] == 2:
                    highFlag += 1
                    if highFlag == 3:
                        if layerFlags[lin - 1][pt - 1] < 2 or layerFlags[lin - 1][pt + 1] < 2:
                            highFlag = 2
                        else:
                            layerFlags[lin - 1][pt] = extraMaterial
                            highFlag = 2
                
        # Square corners
        #self.reportThis("--Square corners")
        for pt in range(1, lastPnt):
            for lin in range(1, lastLn):
                if layerFlags[lin][pt] == 1:                    # point == 1
                    cont = True
                    if layerFlags[lin + 1][pt] == 0:            # forward == 0
                        if layerFlags[lin + 1][pt - 1] == 1:    # forward left == 1
                            if layerFlags[lin][pt - 1] == 2:    # left == 2
                                layerFlags[lin + 1][pt] = 1     # square the corner
                                cont = False
                        
                        if cont == True and layerFlags[lin + 1][pt + 1] == 1:  # forward right == 1
                            if layerFlags[lin][pt + 1] == 2:    # right == 2
                                layerFlags[lin + 1][pt] = 1     # square the corner
                        cont = True
                    
                    if layerFlags[lin - 1][pt] == 0:          # back == 0
                        if layerFlags[lin - 1][pt - 1] == 1:    # back left == 1
                            if layerFlags[lin][pt - 1] == 2:    # left == 2
                                layerFlags[lin - 1][pt] = 1     # square the corner
                                cont = False
                        
                        if cont == True and layerFlags[lin - 1][pt + 1] == 1:  # back right == 1
                            if layerFlags[lin][pt + 1] == 2:    # right == 2
                                layerFlags[lin - 1][pt] = 1     # square the corner

        # remove inside corners
        #self.reportThis("--Remove inside corners")
        for pt in range(1, lastPnt):
            for lin in range(1, lastLn):
                if layerFlags[lin][pt] == 1:                    # point == 1
                    if layerFlags[lin][pt + 1] == 1:
                        if layerFlags[lin - 1][pt + 1] == 1 or layerFlags[lin + 1][pt + 1] == 1:
                            layerFlags[lin][pt + 1] = 9
                    elif layerFlags[lin][pt - 1] == 1:
                        if layerFlags[lin - 1][pt - 1] == 1 or layerFlags[lin + 1][pt - 1] == 1:
                            layerFlags[lin][pt - 1] = 9
                        
        #print("\n-------------")
        #for li in layerFlags:
        #    print("Line: " + str(li))

        return layerFlags

    def _extractWaterlines(self, obj, lyr, prvDep, layDep, layerFlags):
        srch = True
        lastPnt = len(layerFlags[1]) - 1
        lastLn = len(layerFlags) - 1
        waterlines = []
        maxSrchs = 10
        srchCnt = 1
        cmds =[]
        loopNum = 0

        while srch == True:
            srch = False
            if srchCnt > maxSrchs:
                self.reportThis("Max search scans, " + str(maxSrchs) + " reached\nPossible incomplete waterline result!")
                break
            if obj.CutMode == 'Climb':
                lC =   [ 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0]
                pC =   [-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1]
            else:
                lC =   [-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0]
                pC =   [-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1,-1, 0, 1, 1, 1, 0,-1,-1]
            
            for L in range(1, lastLn):
                for P in range(1, lastPnt):
                    if layerFlags[L][P] == 1:
                        # start loop follow
                        srch = True
                        loopNum += 1
                        loop = [self.CLP[L - 1][P - 1]]  # Start loop point list
                        cur = [L, P]
                        prv = [L, P - 1]
                        follow = True
                        ptc = 0
                        ptLmt = 200000
                        while follow == True:
                            ptc += 1
                            if ptc > ptLmt:
                                self.reportThis("Loop number " + str(loopNum) + " at [" + str(nxt[0]) + ", " + str(nxt[1]) + "] pnt count exceeds, " + str(ptLmt) + ".  Stopped following loop.")
                                break
                            nxt = self._findNextWlPoint(layerFlags, lC, pC, cur[0], cur[1], prv[0], prv[1])  # get next point
                            loop.append(self.CLP[nxt[0] - 1][nxt[1] - 1])  # add it to loop point list
                            layerFlags[nxt[0]][nxt[1]] = nxt[2]  # Mute the point, if not Y stem
                            if nxt[0] == L and nxt[1] == P:  # check if loop complete
                                follow = False
                            elif nxt[0] == cur[0] and nxt[1] == cur[1]:  # check if line cannot be detected
                                follow = False
                            prv = cur
                            cur = nxt
                        layerFlags[L][P] = 0  # Mute the starting point
                        cmds = self._loopListToGcode(obj, prvDep, layDep, loop)
                        waterlines.extend(cmds)
            srchCnt += 1
        self.reportThis("Search count for layer " + str(lyr) + " is " + str(srchCnt) + ", with " + str(loopNum) + " loops.")
        return waterlines

    def _findNextWlPoint(self, layerFlags, lC, pC, cl, cp, pl, pp):
        dl = cl - pl # diff in Line number
        dp = cp - pp # diff in Point number
        num = 0
        i = 3
        s = 0
        mtch = 0
        found = False
        # Check all 8 points around current point to determine direction from prev pnt to cur pnt
        #   This direction is used to identify starting point for circular scan of surrounding 8 points
        while mtch < 8:  
            if lC[i] == dl:
                if pC[i] == dp:
                    s = i - 3
                    found = True
                    # Check for y branch where current point is connection between branches
                    for y in range(1, mtch):
                        if lC[i+y] == dl:
                            if pC[i+y] == dp:
                                num = 1 # Y scenario, so leave base point as means to return through base point
                                break
                    break
            i += 1
            mtch += 1
        if found == False:
            return [cl, cp, num]

        for r in range(0, 8):
            l = cl + lC[s + r]
            p = cp + pC[s + r]
            if layerFlags[l][p] == 1:
                return [l, p, num]
        
        return [cl, cp, num]

    def _loopListToGcode(self, obj, prvDep, layDep, loop):
        # generate the path commands
        output = []
        optimize = obj.Optimize
        onOpti = False

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
        output.append(Path.Command('G0', {'Z': prvDep, 'F': self.vertRapid}))
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
                output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizRapid}))
                # output.append(Path.Command('G1', {'Z': pnt.z, 'F': self.vertFeed}))

            # Rotate point data
            prev.x = pnt.x
            prev.y = pnt.y
            prev.z = pnt.z
            pnt.x = nxt.x
            pnt.y = nxt.y
            pnt.z = nxt.z
        # output.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        # output.append(Path.Command('N (End of line ' + str(line) + ')', {}))
        # self.reportThis("points after optimization: " + str(len(output)))

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt.x = pnt.x
        self.layerEndPnt.y = pnt.y
        self.layerEndPnt.z = pnt.z
        
        return output



    def cutPatternLine(self, obj, n, p1, p2):
        if obj.CutPattern == 'ZigZag':
            if obj.CutMode == 'Conventional':
                if (n % 2 == 0.0):  # even
                    lo = ocl.Line(p1, p2)     # line-object
                else:  # odd
                    lo = ocl.Line(p2, p1)     # line-object
            else:
                if (n % 2 == 0.0):  # even
                    lo = ocl.Line(p2, p1)     # line-object
                else:  # odd
                    lo = ocl.Line(p1, p2)     # line-object
        elif obj.CutPattern == 'Line':
            if obj.CutMode == 'Conventional':
                lo = ocl.Line(p1, p2)     # line-object
            else:  # odd
                lo = ocl.Line(p2, p1)     # line-object
        return lo

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
        self.layerFlags = []
        self.holdPntCnt = 0
        self.startTime = 0.0
        self.endTime = 0.0
        self.lineCNT = 0
        self.pntsPerLine = 0
        self.keepTime = 0.0
        self.lineScanTime = 0.0
        self.bbRadius = 0.0
        self.targetDepth = 0.0
        self.stepDeg = 0.0
        self.stepRads = 0.0
        self.axialFeed = 0.0
        self.axialRapid = 0.0
        self.numScanLines = 0
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
        case Tool::DRILL:
            return "Drill";
        case Tool::CENTERDRILL:
            return "CenterDrill";
        case Tool::COUNTERSINK:
            return "CounterSink";
        case Tool::COUNTERBORE:
            return "CounterBore";
        case Tool::FLYCUTTER:
            return "FlyCutter";
        case Tool::REAMER:
            return "Reamer";
        case Tool::TAP:
            return "Tap";
        case Tool::ENDMILL:
            return "EndMill";
        case Tool::SLOTCUTTER:
            return "SlotCutter";
        case Tool::BALLENDMILL:
            return "BallEndMill";
        case Tool::CHAMFERMILL:
            return "ChamferMill";
        case Tool::CORNERROUND:
            return "CornerRound";
        case Tool::ENGRAVER:
            return "Engraver";
        case Tool::UNDEFINED:
            return "Undefined";
        '''

    def pocketInvertExtraOffset(self):
        return True

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ... initialize defaults'''

        # obj.ZigZagAngle = 45.0
        obj.StepOver = 50
        obj.Optimize = True
        obj.CutModeForced = False
        obj.IgnoreWaste = False
        obj.LayerMode = 'Single-pass'
        obj.ScanType = 'Planar'
        obj.RotationAxis = 'X'
        obj.CutMode = 'Conventional'
        obj.MultipassStepOverSize = 'Diminishing'
        obj.CutPattern = 'ZigZag'
        obj.CutterTilt = 18.0
        obj.StartIndex = 0.0
        obj.StopIndex = 360.0
        obj.SampleInterval = 1.0

        # need to overwrite the default depth calculations for facing
        job = PathUtils.findParentJob(obj)
        if job and job.Stock:
            d = PathUtils.guessDepths(job.Stock.Shape, None)
            obj.OpStartDepth.Value = d.start_depth
            obj.OpFinalDepth.Value = d.final_depth
            obj.IgnoreWasteDepth = d.final_depth + 0.001
            self.initFinalDepth = d.final_depth
            self.initStartDepth = d.start_depth



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
    setup.append("MultipassStepOverSize")
    setup.append("CutModeForced")
    setup.append("CutPattern")
    setup.append("IgnoreWasteDepth")
    setup.append("IgnoreWaste")
    return setup

def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Surface operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectSurface(obj, name)
    return obj
