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
    sys.exit(translate("Path_Surface", "This operation requires OpenCamLib to be installed."))


class ObjectSurface(PathOp.ObjectOp):
    '''Proxy object for Surfacing operation.'''
    cutter = None
    holdPoint = None
    startTime = None
    endTime = None
    onHold = None
    lineCNT = 0
    layerEndPnt = None
    holdStartPnts = []
    holdStopPnts = []
    holdPntCnt = 0
    holdZMaxVals = []
    holdPrevLayerVals = []
    gcodeCmds = []
    scanTime = 0

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
        obj.BoundBox = ['Stock', 'BaseBoundBox']
        obj.DropCutterDir = ['X', 'Y']
        obj.Algorithm = ['OCL Dropcutter', 'OCL Waterline']
        obj.SampleInterval = (0.05, 0.01, 1.0, 0.01)
        #obj.SampleInterval = (1.27, 0.01, 1.0, 0.01)
        #obj.SampleInterval = (6.35, 0.01, 1.0, 0.01)  # multiply original by 127 to yield 1/4"(6.35mm) for default sample interval in surface op window

        if not hasattr(obj, 'DoNotSetDefaultValues'):
            self.setEditorProperties(obj)

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

        # reset operation variables
        self.cutter = None
        self.holdPoint = None
        self.startTime = None
        self.endTime = None
        self.onHold = None
        self.lineCNT = 0
        self.layerEndPnt = None
        self.holdStartPnts = []
        self.holdStopPnts = []
        self.holdPntCnt = 0
        self.holdZMaxVals = []
        self.holdPrevLayerVals = []
        self.gcodeCmds = []
        self.scanTime = 0

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
            return
        
        # Set cutter details
        if obj.ToolController.Tool.ToolType == 'BallEndMill':
            self.cutter = ocl.BallCutter(obj.ToolController.Tool.Diameter, 5)  # TODO: 5 represents cutting edge height. Should be replaced with the data from toolcontroller?
        else:
            self.cutter = ocl.CylCutter(obj.ToolController.Tool.Diameter, 5)

        self.holdPntCnt = 0

        # Cycle through parts of model
        for base in self.model:
            print("\n-----\n-----\nbase object: " + base.Name)
            print("StepOver is  " + str(obj.StepOver))

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
            # Determine bounding box length
            bbLength = bb.YLength
            if obj.DropCutterDir == 'Y':
                bbLength = bb.XLength

            s = ocl.STLSurf()
            for f in mesh.Facets:
                p = f.Points[0]
                q = f.Points[1]
                r = f.Points[2]
                # offset the triangle in Z with DepthOffset
                t = ocl.Triangle(ocl.Point(p[0], p[1], p[2] + obj.DepthOffset.Value),
                                 ocl.Point(q[0], q[1], q[2] + obj.DepthOffset.Value),
                                 ocl.Point(r[0], r[1], r[2] + obj.DepthOffset.Value))
                s.addTriangle(t)

            final = []
            if obj.Algorithm == 'OCL Dropcutter':
                # Objective is to remove material from surface in StepDown layers rather than one pass to FinalDepth
                finalDepth = obj.FinalDepth.Value
                targetDepth = finalDepth + obj.DepthOffset.Value
                toBeRemoved = obj.OpStockZMax.Value - targetDepth
                if toBeRemoved < 0:
                    toBeRemoved = 0
                totalLayers = int(math.ceil(toBeRemoved / obj.StepDown.Value)) + 1

                # the max and min XY area of the operation
                xmin = bb.XMin - obj.DropCutterExtraOffset.x
                xmax = bb.XMax + obj.DropCutterExtraOffset.x
                ymin = bb.YMin - obj.DropCutterExtraOffset.y
                ymax = bb.YMax + obj.DropCutterExtraOffset.y

                # Scan the piece to depth
                CLP = self._dropCutterScan(obj, s, bbLength, xmin, ymin, xmax, ymax, targetDepth)
                print("--Received: ", str(len(CLP)), " CLP points at target depth of ", str(targetDepth))

                layerDepth = obj.OpStockZMax.Value + obj.DepthOffset.Value  # obj.OpStartDepth.Value
                nextDepth = layerDepth - obj.StepDown.Value
                prevDepth = obj.OpStockZMax.Value # obj.OpStartDepth.Value
                self.onHold = False
                gcode = []
                lc = 0
                self.scanTime = 0

                pnt = ocl.Point(float("inf"), float("inf"), float("inf"))

                while lc < totalLayers:
                    print("LAYER: " + str(lc))
                    print("--Layer Depth: " + str(layerDepth))
                    
                    self.dcScanToGcode_2(obj, lc, prevDepth, layerDepth, CLP)

                    # add commands to operation list
                    self.commandlist.extend(gcode)

                    nextDepth = layerDepth - obj.StepDown.Value
                    if nextDepth < targetDepth:  # Do not allow cutter below targetDepth
                        nextDepth = targetDepth                    
                    prevDepth = layerDepth
                    layerDepth = nextDepth
                    lc += 1
                    # print("--Next Depth: " + str(nextDepth))

                print("--All layer extractions combined took ", self.scanTime, " s")
                # Process all HOLDs in gcode command list
                self.scanTime = time.time()
                hldcnt = 0
                hpCmds = []
                lenHP = len(self.holdStartPnts)
                lenHP2 = len(self.holdStopPnts)
                if lenHP != lenHP2:
                    print("ERROR: HoldPoint lists different lengths.")
                    exit()
                if lenHP != self.holdPntCnt:
                    print("ERROR: lenHP != holdPntCnt.")
                    exit()
                # cycle throug hold points
                print("--Processing ", str(lenHP), " HOLD optimizations---")
                cutter = self.cutter.getDiameter()
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
                            # Set the max and min XY boundaries of the HOLD connection operation
                            if p1.x < p2.x:
                                xmin = p1.x - cutter/1.5
                                xmax = p2.x + cutter/1.5
                            else: 
                                xmin = p2.x - cutter/1.5
                                xmax = p1.x + cutter/1.5

                            if p1.y < p2.y:
                                ymin = p1.y - cutter/1.5
                                ymax = p2.y + cutter/1.5
                            else: 
                                ymin = p2.y - cutter/1.5
                                ymax = p1.y + cutter/1.5
                            # get focused list of points based on bound box with p1 and p2 as corners, with cutter diam. as additional buffer
                            subCLP = self.subsectionCLP(CLP, xmin, ymin, xmax, ymax)
                            # Determine max z height for clearance between p1 and p2
                            zMax = self.getMaxHeight(layerDepth, p1, p2, cutter, subCLP)
                            # Create gcode commands to connect p1 and p2
                            hpCmds = self.holdStopCmds(obj, zMax, pd, p2)
                            for cmd in hpCmds:
                                final.append(cmd)
                            hldcnt += 1
                        else:
                            final.append(self.gcodeCmds[idx])
                else:
                    final = self.gcodeCmds
            elif obj.Algorithm == 'OCL Waterline':
                final = self._waterline(obj, s, bb)
            # Send final list of commands to operation object
            self.commandlist.extend(final)

        self.endTime = time.time()
        print("OPERATION time: ", self.endTime - self.startTime, " sec.")

    def _waterline(self, obj, s, bb):
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
        wl.setSTL(s)

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

    def _dropCutterScan(self, obj, s, bbLength, xmin, ymin, xmax, ymax, fd):
        t_before = time.time()
        
        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(s)
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
        self.scanTime += t_after - t_before

        # return the list the points
        return pdc.getCLPoints()

    def dcScanToGcode_2(self, obj, lc, prvDep, layDep, CLP):
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

        def makePnt(pnt):
            p = ocl.Point(float("inf"), float("inf"), float("inf"))
            p.x = pnt.x
            p.y = pnt.y
            p.z = pnt.z
            return p
        
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
        if CLP[0].z < layDep:
            pnt.z = layDep
        elif CLP[0].z > prvDep:
            firstDrop = False
        else:
            pnt.z = CLP[0].z

        # generate the path commands
        # Send cutter to starting position(first point)
        output.append(Path.Command('N (Beginning of layer ' + str(lc) + ')', {}))
        output.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))  # Z was set to obj.ClearanceHeight.Value
        output.append(Path.Command('G0', {'X': pnt.x, "Y": pnt.y, 'F': self.horizRapid}))
        if firstDrop:
            output.append(Path.Command('G1', {'Z': pnt.z, 'F': self.vertFeed}))
        
        # Begin processing ocl points list into gcode
        for i in range(0, lenCLP):
            pnt.x = CLP[i].x
            pnt.y = CLP[i].y
            if CLP[i].z < layDep:
                pnt.z = layDep
            else:
                pnt.z = CLP[i].z

            # Calculate next point for consideration of next point
            if i < lastCLP:
                nxt.x = CLP[i + 1].x
                nxt.y = CLP[i + 1].y
                if CLP[i + 1].z < layDep:
                    nz = layDep
                else:
                    nz = CLP[i + 1].z
                nxt.z = nz
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
            
            # if z travels above previous layer, start/continue hold high cycle
            if pnt.z > prvDep:  
                if self.onHold == False:
                    holdStart = True
                self.onHold = True
            
            if self.onHold == True:
                if holdStart == True:
                    holdCount += 1  # track number of holds
                    holdLine = self.lineCNT
                    # Save holdStart coordinate and prvDep values
                    self.holdPoint.x = pnt.x
                    self.holdPoint.y = pnt.y
                    self.holdPoint.z = pnt.z
                    self.holdStartPnts.append(makePnt(pnt))
                    self.holdPrevLayerVals.append(prvDep)
                    # go to current coordinate
                    output.append(Path.Command('G1', {'X': pnt.x, "Y": pnt.y, "Z": pnt.z, 'F': self.horizFeed}))
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
                    for cmd in self.holdStopCmds(obj, zMax, prvDep, pnt):
                        output.append(cmd)
                else:
                    # if  start and stop points on different lines, process has complex hold
                    self.holdStopPnts.append(makePnt(pnt))
                    self.holdZMaxVals.append(zMax)
                    output.append("HD")  # add placeholder for processing of hold
                    self.holdPntCnt += 1
                
                # reset necessary hold related settings
                zMax = prvDep
                holdStop = False
                self.onHold = False
                self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))

            if self.onHold == False:
                #if not optimize or not self.isPointOnLine(FreeCAD.Vector(prev.x, prev.y, prev.z), FreeCAD.Vector(nxt.x, nxt.y, nxt.z), FreeCAD.Vector(pnt.x, pnt.y, pnt.z)):
                if not self.isPointOnLine(FreeCAD.Vector(prev.x, prev.y, prev.z), FreeCAD.Vector(nxt.x, nxt.y, nxt.z), FreeCAD.Vector(pnt.x, pnt.y, pnt.z)):
                    output.append(Path.Command('G1', {'X': pnt.x, "Y": pnt.y, "Z": pnt.z, 'F': self.horizFeed}))
                elif i == lastCLP:
                    output.append(Path.Command('G1', {'X': pnt.x, "Y": pnt.y, "Z": pnt.z, 'F': self.horizFeed}))
            
            prev.x = pnt.x
            prev.y = pnt.y
            prev.z = pnt.z

        #save current layer end point
        if self.onHold == True:
            if holdLine == self.lineCNT:
                self.holdStartPnts.pop()
                self.holdPrevLayerVals.pop()
                for cmd in self.holdStopCmds(obj, zMax, prvDep, pnt):
                    output.append(cmd)
            else:
                self.holdStopPnts.append(makePnt(pnt))
                self.holdZMaxVals.append(zMax)
                output.append("HD")  # add placeholder for processing of hold
                self.holdPntCnt += 1
            self.onHold = False
        
        # save last point for insertion into next layer CLP as start point
        endPnt = pnt
        endPnt.z = obj.OpStockZMax.Value + obj.DepthOffset.Value
        self.layerEndPnt = endPnt
        print("--Points after linear optimization: " + str(len(output)))
        # append layer commands to operation command list
        for o in output:
            self.gcodeCmds.append(o)
        self.gcodeCmds.append(Path.Command('N (End of layer ' + str(lc) + ')', {}))
        #return output

    def holdStopCmds(self, obj, zMax, pd, p2):
        cmds = []
        cmds.append(Path.Command('N (Begin: Hold Stop)', {}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'Z': zMax, 'F': self.vertRapid}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'X': p2.x, "Y": p2.y, 'F': self.horizRapid}))  # horizontal rapid to current XY coordinate
        cmds.append(Path.Command('G0', {'Z': pd, 'F': self.vertRapid}))  # drop cutter down rapidly to prevDepth depth
        cmds.append(Path.Command('G0', {'Z': p2.z, 'F': self.vertFeed}))  # drop cutter down to current Z depth, returning to normal cut path and speed
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
            obj.OpStartDepth = d.start_depth
            obj.OpFinalDepth = d.final_depth
            #obj.FinalDepth.Value = d.final_depth
    

def SetupProperties():
    setup = []
    setup.append("Algorithm")
    setup.append("DropCutterDir")
    setup.append("BoundBox")
    setup.append("StepOver")
    setup.append("DepthOffset")
    return setup

def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Surface operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectSurface(obj, name)
    return obj
