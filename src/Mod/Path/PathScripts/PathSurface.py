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
# *   Additional modifications and contributions in 2019                    *
# *   by Russell Johnson  <russ4262@gmail.com>                              *
# *   Version: Rev. 3e - Usable                                             *
# *                                                                         *
# *   NOTES:  After changing value in property window, only click on        *
# *   blue re-compute icon in menu bar. Do NOT click anywhere outside       *
# *   property window, which normally triggers auto re-compute. When used   *
# *   in this manner, this version is mostly stable.                        *
# *                                                                         *
# ***************************************************************************

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

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

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
        obj.addProperty("App::PropertyPercent", "StepOver", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Step over percentage of the drop cutter path"))
        obj.addProperty("App::PropertyDistance", "DepthOffset", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Z-axis offset from the surface of the object"))
        obj.addProperty("App::PropertyFloatConstraint", "SampleInterval", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "The Sample Interval. Small values cause long wait times"))
        obj.addProperty("App::PropertyBool", "Optimize", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable optimization which removes unnecessary points from G-Code output"))
        obj.addProperty("App::PropertyEnumeration", "CompletionMode", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "The completion mode for the operation: single or multi-pass"))
        obj.addProperty("App::PropertyEnumeration", "ScanType", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "Planear: Flat, 3D surface scan.  Rotational: 4th-axis rotational scan."))
        obj.BoundBox = ['Stock', 'BaseBoundBox']
        obj.DropCutterDir = ['X', 'Y']
        obj.Algorithm = ['OCL Dropcutter', 'OCL Waterline']
        obj.SampleInterval = (0.04, 0.01, 1.0, 0.01)
        obj.CompletionMode = ['Single-pass', 'Multi-pass']
        obj.ScanType = ['Planear', 'Rotational']

        if not hasattr(obj, 'DoNotSetDefaultValues'):
            self.setEditorProperties(obj)
        # Check if obj has attributes
        if hasattr(obj, 'CompletionMode'):
            print("--CompletionMode: ", obj.CompletionMode)
        else:
            print("**CompletionMode non-existant; single-pass based if Optimize box true")

        # Set scan mode
        if hasattr(obj, 'ScanType'):
            print("--ScanType: ", obj.ScanType)
        else:
            print("**ScanType non-existant; planear mode is default.")

    def setEditorProperties(self, obj):
        if obj.Algorithm == 'OCL Dropcutter':
            obj.setEditorMode('DropCutterDir', 0)
            obj.setEditorMode('DropCutterExtraOffset', 0)
        else:
            obj.setEditorMode('DropCutterDir', 2)
            obj.setEditorMode('DropCutterExtraOffset', 2)

    def onChanged(self, obj, prop):
        if prop == "Algorithm":
            self.setEditorProperties(obj)

    def opOnDocumentRestored(self, obj):
        self.setEditorProperties(obj)

    def opExecute(self, obj):
        '''opExecute(obj) ... process surface operation'''
        PathLog.track()
        print("\n-----\n-----\nBegin 3D surface operation")

        # reset operation variables
        N = self.resetOpVariables()

        # mark beginning of operation
        self.startTime = time.time()

        if obj.StepOver > 100:
            obj.StepOver = 100
        if obj.StepOver < 1:
            obj.StepOver = 1
        
        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment) + ')\n'

        output += "(" + obj.Label + ")"
        output += "(Compensated Tool Path. Diameter: " + str(obj.ToolController.Tool.Diameter) + ")"
        
        parentJob = PathUtils.findParentJob(obj)
        if parentJob is None:
            print("No parentJob")
            return
        
        # Set cutter details
        if obj.ToolController.Tool.ToolType == 'BallEndMill':
            self.cutter = ocl.BallCutter(obj.ToolController.Tool.Diameter, 5)  # TODO: 5 represents cutting edge height. Should be replaced with the data from toolcontroller?
        else:
            self.cutter = ocl.CylCutter(obj.ToolController.Tool.Diameter, 5)

        # Check if obj has attributes
        singlePass = True
        if hasattr(obj, 'CompletionMode'):
            if obj.CompletionMode == 'Multi-pass':
                singlePass = False
            print("--CompletionMode: ", obj.CompletionMode)
        else:
            print("**CompletionMode non-existant; single-pass based if Optimize box true")

        # Set scan mode
        rotationalMode = False
        if hasattr(obj, 'ScanType'):
            if obj.ScanType == 'Rotational':
                rotationalMode = True
            print("--ScanType: ", obj.ScanType)
        else:
            print("**ScanType non-existant; planear mode is default.")

        # Cycle through parts of model
        for base in self.model:
            print("BASE object: " + base.Name)

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
            if obj.BoundBox == "BaseBoundBox":
                bb = mesh.BoundBox
            else:
                bb = parentJob.Stock.Shape.BoundBox
            
            final = []
            # Objective is to remove material from surface in StepDown layers rather than one pass to FinalDepth
            if obj.Algorithm == 'OCL Dropcutter':
                if rotationalMode == True:
                    # print("--Rotational scan mode")
                    rtn = self.performRotDropCutScans(obj, parentJob, mesh, bb)
                    final = self.processRotationalScans(obj, singlePass)
                else:
                    # print("--Planear scan mode")
                    N = self.planearDropCut(obj, mesh, bb, singlePass)
                    final = self.processPlanearHolds(obj)
            elif obj.Algorithm == 'OCL Waterline':
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
                    stl.addTriangle(t)
                final = self._waterline(obj, stl, bb)
            # End IF
            # Send final list of commands to operation object
            self.commandlist.extend(final)

        self.endTime = time.time()
        print("OPERATION time: ", self.endTime - self.startTime, " sec.")
        # reset operation variables
        # self.resetOpVariables()



    def planearDropCut(self, obj, mesh, bb, singlePass):
        self.keepTime = time.time()
        # Determine bounding box length
        bbLength = bb.YLength
        if obj.DropCutterDir == 'Y':
            bbLength = bb.XLength

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
            stl.addTriangle(t)

        # the max and min XY area of the operation
        xmin = bb.XMin - obj.DropCutterExtraOffset.x
        xmax = bb.XMax + obj.DropCutterExtraOffset.x
        ymin = bb.YMin - obj.DropCutterExtraOffset.y
        ymax = bb.YMax + obj.DropCutterExtraOffset.y

        # Compute number and size of stepdowns, and final depth
        if singlePass:
            # depthparams =  [obj.FinalDepth.Value]  # + obj.DepthOffset.Value
            depthparams =  [obj.FinalDepth.Value]
        else:
            dep_par = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown, 0.0, obj.FinalDepth.Value)
            depthparams = [i for i in dep_par]
        # print("--depthparams:", str(depthparams))
        lenDP = len(depthparams)
        self.targetDepth = depthparams[lenDP-1]
        prevDepth = depthparams[0]

        # Scan the piece to depth
        self.CLP = self._dropCutterScan(obj, stl, bbLength, xmin, ymin, xmax, ymax, self.targetDepth)
        # print("--Received: ", str(len(self.CLP)), " CLP points at target depth of ", str(self.targetDepth))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Extract layers per depthparams
        for lyr in  range(0, lenDP):
            # print("--LAYER ", lyr)
            # print("----Depth: " + str(depthparams[lyr]))
            # Determine next depth
            if lyr < lenDP - 1:
                nextDepth = depthparams[lyr + 1]                    
            # Convert current layer data to gcode
            self.planearScanToGcode(obj, lyr, prevDepth, depthparams[lyr], self.CLP, singlePass)
            # Save previous depth
            prevDepth = depthparams[lyr]
        #print("--END LAYER SCANS")
        print("--All layer scans combined took ", time.time() - self.keepTime, " s")
        return True

    def _dropCutterScan(self, obj, stl, bbLength, xmin, ymin, xmax, ymax, fd):
        t_before = time.time()
        
        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(stl)
        pdc.setCutter(self.cutter)
        pdc.setZ(fd)  # set minimumZ (final / target depth value)
        pdc.setSampling(obj.SampleInterval)

        path = ocl.Path()                   # create an empty path object

        cutOut = (self.cutter.getDiameter() * (obj.StepOver / 100.0))
        notCutting = (self.cutter.getDiameter() * ((100 - obj.StepOver) / 100.0))
        if obj.DropCutterDir == 'X':
            Ny = int(math.ceil((bbLength + (2 * obj.DropCutterExtraOffset.y)) / cutOut)) # Number of lines
            # add Line objects to the path in this loop
            for n in range(0, Ny):
                if n == Ny - 1:
                    if obj.StepOver > 50:
                        cutOut = (self.cutter.getDiameter() / 2)
                    y = ymax - cutOut
                else:
                    y = ymin - (self.cutter.getDiameter() / 2) + ((n+1) * cutOut) # all lines are offest by 1/2 cutter diameter
                p1 = ocl.Point(xmin, y, 0)   # start-point of line
                p2 = ocl.Point(xmax, y, 0)   # end-point of line
                if (n % 2 == 0):  # even
                    l = ocl.Line(p1, p2)     # line-object
                else:  # odd
                    l = ocl.Line(p2, p1)     # line-object

                path.append(l)        # add the line to the path
        else:
            Nx = int(math.ceil((bbLength + (2 * obj.DropCutterExtraOffset.x)) / cutOut)) # Number of lines
            # add Line objects to the path in this loop
            for n in range(0, Nx):
                if n == Nx - 1:
                    if obj.StepOver > 50:
                        cutOut = (self.cutter.getDiameter() / 2)
                    x = xmax - cutOut
                else:
                    x = xmin - (self.cutter.getDiameter() / 2) + ((n+1) * cutOut) # all lines are offest by 1/2 cutter diameter
                p1 = ocl.Point(x, ymin, 0)   # start-point of line
                p2 = ocl.Point(x, ymax, 0)   # end-point of line
                if (n % 2 == 0):  # even
                    l = ocl.Line(p1, p2)     # line-object
                else:  # odd
                    l = ocl.Line(p2, p1)     # line-object

                path.append(l)        # add the line to the path

        pdc.setPath(path)

        # run drop-cutter on the path
        pdc.run()
        t_after = time.time()
        self.keepTime += t_after - t_before

        # return the list the points
        return pdc.getCLPoints()

    def planearScanToGcode(self, obj, lc, prvDep, layDep, CLP, spm):
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
            begcmd.append(Path.Command('G0', {'X': pnt.x, "Y": pnt.y, 'F': self.horizRapid}))
            if firstDrop:
                begcmd.append(Path.Command('G1', {'Z': pnt.z, 'F': self.vertFeed}))
            begcmd.reverse()
            return begcmd
        
        # Create containers for x,y,z points
        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))
        if self.holdPoint == None:
            self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))
        travVect = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Set values for first gcode point in layer
        pnt.x = CLP[0].x
        pnt.y = CLP[0].y
        pnt.z = CLP[0].z
        if CLP[0].z < layDep:
            pnt.z = layDep
        elif CLP[0].z > prvDep:
            firstDrop = False
            if spm:
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
            
            if not spm:
                # if z travels above previous layer, start/continue hold high cycle
                if pnt.z > prvDep:  
                    if self.onHold == False:
                        holdStart = True
                    self.onHold = True
                
            if self.onHold == True:
                if holdStart == True:
                    # go to current coordinate
                    output.append(Path.Command('G1', {'X': pnt.x, "Y": pnt.y, "Z": pnt.z, 'F': self.horizFeed}))
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
                    output.append(Path.Command('G1', {'X': pnt.x, "Y": pnt.y, "Z": pnt.z, 'F': self.horizFeed}))
                elif i == lastCLP:
                    output.append(Path.Command('G1', {'X': pnt.x, "Y": pnt.y, "Z": pnt.z, 'F': self.horizFeed}))
            
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
        # print("----Points after linear optimization: " + str(len(output)))
        # append layer commands to operation command list
        for o in output:
            self.gcodeCmds.append(o)
        self.gcodeCmds.append(Path.Command('N (End of layer ' + str(lc) + ')', {}))
        #return output

    def processPlanearHolds(self, obj):    
        # Process all HOLDs in gcode command list
        self.keepTime = time.time()
        hldcnt = 0
        hpCmds = []
        lenHP = len(self.holdStartPnts)
        commands = [Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid})]
        print("--Processing ", str(lenHP), " HOLD optimizations---")
        # cutter = self.cutter.getDiameter()
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



    def performRotDropCutScans(self, obj, parentJob, mesh, bb):
        cutOut = (self.cutter.getDiameter() * (obj.StepOver / 100.0))

        # Objective is to remove material from surface in StepDown layers rather than one pass to Final Depth
        xmin = bb.XMin
        xmax = bb.XMax
        ymin = bb.YMin
        ymax = bb.YMax
        zmin = bb.ZMin
        zmax = bb.ZMax

        # Determine boundbox radius for xzy limit data
        if math.fabs(zmin) > math.fabs(zmax):
            vlim = bb.ZMin
        else:
            vlim = bb.ZMax
        if obj.DropCutterDir == 'X':
            rotVect = FreeCAD.Vector(1,0,0)
            # Rotation is around X-axis, cutter moves along same axis
            if math.fabs(ymin) > math.fabs(ymax):
                hlim = bb.YMin
            else:
                hlim = bb.YMax
        else:
            rotVect = FreeCAD.Vector(0,1,0)
            # Rotation is around Y-axis, cutter moves along same axis
            if math.fabs(xmin) > math.fabs(xmax):
                hlim = bb.XMin
            else:
                hlim = bb.XMax
        
        # Compute max radius of stock, as it rotates
        self.bbRadius = math.sqrt(hlim**2 + vlim**2)
        bbRad = self.bbRadius
        bbCircum = 2 * math.pi * bbRad

        # Set updated bound box values
        # Redefine the new max and min XY area of the operation based on greatest point radius of model
        bb.ZMin = -1*bbRad
        bb.ZMax = bbRad
        if obj.DropCutterDir == 'X':
            bb.YMin = -bbRad
            bb.YMax = bbRad
            #ymin = -1*bbRad
            #ymax = bbRad
            ymin = 0.0
            ymax = 0.0
            xmin = bb.XMin - obj.DropCutterExtraOffset.x
            xmax = bb.XMax + obj.DropCutterExtraOffset.x
        else:
            bb.XMin = -bbRad
            bb.XMax = bbRad
            ymin = bb.YMin - obj.DropCutterExtraOffset.y
            ymax = bb.YMax + obj.DropCutterExtraOffset.y
            #xmin = -1*bbRad
            #xmax = bbRad
            xmin = 0.0
            xmax = 0.0

        # Compute angular index angles for rotation
        #stepDeg = round((cutOut / bbCircum) * 360, 5)
        stepDeg = (cutOut / bbCircum) * 360
        stepRads = (cutOut / bbCircum) * 2 * math.pi  #stepDeg * math.pi / 180

        # Determine angule indexes
        angIdx = 0
        self.scanAngleIndexes = []
        while angIdx < 360:
            self.scanAngleIndexes.append(angIdx)
            angIdx += stepDeg

        # Create stl object for ocl
        stl = ocl.STLSurf()
        for f in mesh.Facets:
            p = f.Points[0]
            q = f.Points[1]
            r = f.Points[2]
            # offset the triangle in Z with DepthOffset
            ###t = ocl.Triangle(ocl.Point(p[0], p[1], p[2] + obj.DepthOffset.Value),
            ###                ocl.Point(q[0], q[1], q[2] + obj.DepthOffset.Value),
            ###                ocl.Point(r[0], r[1], r[2] + obj.DepthOffset.Value))
            # Do not offset Z here becauso of polar rotation
            t = ocl.Triangle(ocl.Point(p[0], p[1], p[2]),
                            ocl.Point(q[0], q[1], q[2]),
                            ocl.Point(r[0], r[1], r[2]))
            stl.addTriangle(t)

        indexScan = []
        self.rotationalScanLines = []
        lenSAI = len(self.scanAngleIndexes)
        prevIndex = 0
        mathpi = math.pi
        if obj.DropCutterDir == 'Y':
            mathpi = -math.pi
        for sa in range(0, lenSAI):
            t_before = time.time()

            radsRot = (self.scanAngleIndexes[sa] - prevIndex) * mathpi / 180 - 0.0000001
            # Rotate STL object in ocl
            if obj.DropCutterDir == 'X':
                #rStl = stl.rotate(radsRot, 0.0, 0.0)
                stl.rotate(radsRot, 0.0, 0.0)
            else:
                #rStl = stl.rotate(0.0, radsRot, 0.0)
                stl.rotate(0.0, radsRot, 0.0)
            # get scan at index
            indexScan = self.dropCutterAxialScan(obj, stl, xmin, ymin, xmax, ymax, obj.FinalDepth.Value, sa)
            self.rotationalScanLines.append(indexScan)
            prevIndex = self.scanAngleIndexes[sa]

            # print("--OCL scan time: " + str(time.time() - t_before) + " s")
            self.keepTime += time.time() - t_before
        # End loop
        print("--Total time for ", lenSAI, " OCL scans: ", self.keepTime, " s")
        return True

    def dropCutterAxialScan(self, obj, stl, xmin, ymin, xmax, ymax, fd, idxnum):
        # t_before = time.time()

        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(stl)
        pdc.setCutter(self.cutter)
        pdc.setZ(fd)  # set minimumZ (final / target depth value)
        pdc.setSampling(obj.SampleInterval)

        path = ocl.Path()                   # create an empty path object

        if obj.DropCutterDir == 'X':
            # add Line objects to the path in this loop
            p1 = ocl.Point(xmin, 0.0, 0.0)   # start-point of line
            p2 = ocl.Point(xmax, 0.0, 0.0)   # end-point of line
        else:
            # add Line objects to the path in this loop
            p1 = ocl.Point(0.0, ymin, 0.0)   # start-point of line
            p2 = ocl.Point(0.0, ymax, 0.0)   # end-point of line
        
        if (idxnum % 2 == 0.0):  # even
            l = ocl.Line(p1, p2)     # line-object
        else:  # odd
            l = ocl.Line(p2, p1)     # line-object

        path.append(l)        # add the line to the path

        pdc.setPath(path)

        # run drop-cutter on the path
        pdc.run()
        # self.lineScanTime = time.time() - t_before

        # return the list the points
        return pdc.getCLPoints()

    def processRotationalScans(self, obj, singlePass):
        commands = []
        obj.OpStartDepth.Value = self.bbRadius
        clearHeight = self.bbRadius + 2.0
        safeHeight = clearHeight + 4.35
        # Begin gcode operation with raising cutter to safe height
        commands.append(Path.Command('G0', {'Z': safeHeight, 'F': self.vertRapid}))

        # Compute number and size of stepdowns, and final depth
        # This area needs to be updated to account for self.bbRadius values larger than obj.ClearanceHeight/SafeHeight values
        if singlePass:
            depthparams =  [obj.FinalDepth.Value]
        else:
            # dep_par = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown, 0.0, obj.FinalDepth.Value)
            dep_par = PathUtils.depth_params(clearHeight, safeHeight, self.bbRadius, obj.StepDown, 0.0, obj.FinalDepth.Value)
            depthparams = [i for i in dep_par]
        # print("--depthparams:", str(depthparams))
        lenDP = len(depthparams)
        targetDepth = depthparams[lenDP-1]
        prevDepth = depthparams[0]

        # Convert rotational scans into gcode
        rslLen = len(self.rotationalScanLines)
        for layDep in depthparams:
            # print("--Layer depth:", layDep)
            for li in range(0, rslLen):
                rSL = self.rotationalScanLines[li]
                sAI = self.scanAngleIndexes[li]
                rLTG = self.rotationalLineToGcode(obj, rSL, clearHeight, sAI, prevDepth, layDep, singlePass)
                # print("----line num:", li)
            prevDepth = layDep

        for c in self.gcodeCmds:
            commands.append(c)
        return commands

    def rotationalLineToGcode(self, obj, CLP, clearHeight, idxAng, prvDep, layerDepth, spm):
        # generate the path commands
        output = []
        lastPntFlag = False
        optimize = obj.Optimize
        onHold = False
        holdCount = 0
        holdLine = 0
        holdStart = False
        holdStop = False
        zMin = prvDep
        zMax = prvDep
        lenCLP = len(CLP)
        lastCLP = lenCLP - 1
        self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))
        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))
        
        # Create frist point
        pnt.x = CLP[0].x
        pnt.y = CLP[0].y
        if CLP[0].z < layerDepth:
            pnt.z = layerDepth
        else:
            pnt.z = CLP[0].z

        # Rotate to correct index location
        if obj.DropCutterDir == 'X':
            output.append(Path.Command('G0', {'A': idxAng, 'F': self.vertFeed}))
        else:
            output.append(Path.Command('G0', {'B': idxAng, 'F': self.vertFeed}))
        ###output.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        output.append(Path.Command('G0', {'Z': clearHeight, 'F': self.vertRapid}))

        output.append(Path.Command('G0', {'X': pnt.x, "Y": pnt.y, 'F': self.horizRapid}))
        output.append(Path.Command('G1', {'Z': pnt.z, 'F': self.vertFeed}))

        for i in range(0, lenCLP):
            if i < lastCLP:
                nxt.x = CLP[i + 1].x
                nxt.y = CLP[i + 1].y
                nxt.z = CLP[i + 1].z
                if CLP[i + 1].z < layerDepth:
                    pnt.z = layerDepth
                else:
                    pnt.z = CLP[i + 1].z
            else:
                optimize = False

            # Update zMin and zMax values
            if pnt.z < zMin:
                zMin = pnt.z
            if pnt.z > zMax:
                zMax = pnt.z

            if not spm:
                # if z travels above previous layer, start/continue hold high cycle
                if pnt.z > prvDep:  
                    if self.onHold == False:
                        holdStart = True
                    self.onHold = True
                
                if self.onHold == True:
                    if holdStart == True:
                        # go to current coordinate
                        output.append(Path.Command('G1', {'X': pnt.x, "Y": pnt.y, "Z": pnt.z, 'F': self.horizFeed}))
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
                    output.append(Path.Command('G1', {'X': pnt.x, "Y": pnt.y, "Z": pnt.z, 'F': self.horizFeed}))
                elif i == lastCLP:
                    lastPntFlag = True
 
            # Rotate point data
            prev.x = pnt.x
            prev.y = pnt.y
            prev.z = pnt.z
            pnt.x = nxt.x
            pnt.y = nxt.y
            pnt.z = nxt.z
        # print("points after optimization: " + str(len(output)))
        if lastPntFlag:
            output.append(Path.Command('G1', {'X': pnt.x, "Y": pnt.y, "Z": pnt.z, 'F': self.horizFeed}))
        for o in output:
            self.gcodeCmds.append(o)
        self.gcodeCmds.append(Path.Command('N (End index angle ' + str(round(idxAng, 4)) + ')', {}))
        return True



    def _waterline(self, obj, stl, bb):
        def drawLoops(loops):
            nloop = 0
            pp = []
            pp.append(Path.Command("(waterline begin)"))

            for loop in loops:
                p = loop[0]
                pp.append(Path.Command("(loop begin)"))
                pp.append(Path.Command('G0', {"Z": obj.SafeHeight.Value, 'F': self.vertRapid}))
                pp.append(Path.Command('G0', {'X': p.x, "Y": p.y, 'F': self.horizRapid}))
                pp.append(Path.Command('G1', {"Z": p.z, 'F': self.vertFeed}))
                prev = ocl.Point(float("inf"), float("inf"), float("inf"))
                nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
                optimize = obj.Optimize
                for i in range(1, len(loop)):
                    p = loop[i]
                    if i < len(loop) - 1:
                        nxt.x = loop[i + 1].x
                        nxt.y = loop[i + 1].y
                        nxt.z = loop[i + 1].z
                    else:
                        optimize = False
                    if not optimize or not self.isPointOnLine(FreeCAD.Vector(prev.x, prev.y, prev.z), FreeCAD.Vector(nxt.x, nxt.y, nxt.z), FreeCAD.Vector(p.x, p.y, p.z)):
                        pp.append(Path.Command('G1', {'X': p.x, "Y": p.y, "Z": p.z, 'F': self.horizFeed}))
                    prev.x = p.x
                    prev.y = p.y
                    prev.z = p.z
                    # zheight = p.z
                p = loop[0]
                pp.append(Path.Command('G1', {'X': p.x, "Y": p.y, "Z": p.z, 'F': self.horizFeed}))
                pp.append(Path.Command("(loop end)"))

                print("    loop ", nloop, " with ", len(loop), " points")
                nloop = nloop + 1
            pp.append(Path.Command("(waterline end)"))

            return pp

        depthparams = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value,
                                             obj.StartDepth.Value, obj.StepDown, 0.0, obj.FinalDepth.Value)

        t_before = time.time()
        zheights = [i for i in depthparams]

        wl = ocl.Waterline()
        wl.setSTL(stl)

        if obj.ToolController.Tool.ToolType == 'BallEndMill':
            self.cutter = ocl.BallCutter(obj.ToolController.Tool.Diameter, 5)  # TODO: 5 represents cutting edge height. Should be replaced with the data from toolcontroller?
        else:
            self.cutter = ocl.CylCutter(obj.ToolController.Tool.Diameter, 5)

        wl.setCutter(self.cutter)
        # this should be smaller than the smallest details in the STL file
        wl.setSampling(obj.SampleInterval)
        # AdaptiveWaterline() also has settings for minimum sampling interval
        # (see c++ code)
        all_loops = []
        print ("zheights: {}".format(zheights))
        for zh in zheights:
            print("calculating Waterline at z= ", zh)
            wl.reset()
            wl.setZ(zh)  # height for this waterline
            wl.run()
            all_loops.append(wl.getLoops())
        t_after = time.time()
        calctime = t_after - t_before
        n = 0
        output = []
        for loops in all_loops:  # at each z-height, we may get many loops
            print("  %d/%d:" % (n, len(all_loops)))
            output.extend(drawLoops(loops))
            n = n + 1
        print("(" + str(calctime) + ")")
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
        cmds.append(Path.Command('G0', {'X': p2.x, "Y": p2.y, 'F': self.horizRapid}))  # horizontal rapid to current XY coordinate
        if zMax != pd:
            cmds.append(Path.Command('G0', {'Z': pd, 'F': self.vertRapid}))  # drop cutter down rapidly to prevDepth depth
            cmds.append(Path.Command('G0', {'Z': p2.z, 'F': self.vertFeed}))  # drop cutter down to current Z depth, returning to normal cut path and speed
        return cmds

    def holdStopEndCmds(self, obj, p2, txt):
        cmds = []
        msg = 'N (' + txt + ')'
        cmds.append(Path.Command(msg, {}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))  # Raise cutter rapid to zMax in line of travel
        # cmds.append(Path.Command('G0', {'X': p2.x, "Y": p2.y, 'F': self.horizRapid}))  # horizontal rapid to current XY coordinate
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

    def pocketInvertExtraOffset(self):
        return True

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ... initialize defaults'''

        # obj.ZigZagAngle = 45.0
        obj.StepOver = 50
        obj.Optimize = True
        # need to overwrite the default depth calculations for facing
        job = PathUtils.findParentJob(obj)
        if job and job.Stock:
            d = PathUtils.guessDepths(job.Stock.Shape, None)
            obj.OpStartDepth.Value = d.start_depth
            obj.OpFinalDepth.Value = d.final_depth
            #obj.FinalDepth.Value = d.final_depth

    def resetOpVariables(self):
        # reset operation variables
        self.cutter = None
        self.holdPoint = None
        self.startTime = None
        self.endTime = None
        self.onHold = False
        self.lineCNT = 0
        self.layerEndPnt = None
        self.holdStartPnts = []
        self.holdStopPnts = []
        self.holdStopTypes = []
        self.holdPntCnt = 0
        self.holdZMaxVals = []
        self.holdPrevLayerVals = []
        self.gcodeCmds = []
        self.keepTime = 0
        self.lineScanTime = 0
        self.holdPntCnt = 0
        self.scanAngleIndexes = None
        self.rotationalScanLines = None
        self.stl = None
        self.bbRadius = None
        self.CLP = None
        self.targetDepth = None
        return True



def SetupProperties():
    setup = []
    setup.append("Algorithm")
    setup.append("DropCutterDir")
    setup.append("BoundBox")
    setup.append("StepOver")
    setup.append("DepthOffset")
    setup.append("CompletionMode")
    setup.append("ScanType")
    return setup

def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Surface operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectSurface(obj, name)
    return obj
