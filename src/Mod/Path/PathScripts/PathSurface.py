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

__title__ = "Path Surface Operation"
__author__ = "sliptonic (Brad Collette)"
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
        obj.BoundBox = ['Stock', 'BaseBoundBox']
        obj.DropCutterDir = ['X', 'Y']
        obj.Algorithm = ['OCL Dropcutter', 'OCL Waterline']
        obj.SampleInterval = (0.04, 0.01, 1.0, 0.01)

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

        # OCL must be installed
        try:
            import ocl
        except:
            FreeCAD.Console.PrintError(
                translate("Path_Surface", "This operation requires OpenCamLib to be installed.") + "\n")
            return

        print("StepOver is  " + str(obj.StepOver))
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

        for base in self.model:
            print("base object: " + base.Name)

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

            if obj.Algorithm == 'OCL Dropcutter':
                output = self._dropcutter(obj, s, bb)
            elif obj.Algorithm == 'OCL Waterline':
                output = self._waterline(obj, s, bb)

            self.commandlist.extend(output)

    def _waterline(self, obj, s, bb):
        import time
        import ocl

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
                next = ocl.Point(float("inf"), float("inf"), float("inf"))
                optimize = obj.Optimize
                for i in range(1, len(loop)):
                    p = loop[i]
                    if i < len(loop) - 1:
                        next.x = loop[i + 1].x
                        next.y = loop[i + 1].y
                        next.z = loop[i + 1].z
                    else:
                        optimize = False
                    if not optimize or not self.isPointOnLine(FreeCAD.Vector(prev.x, prev.y, prev.z), FreeCAD.Vector(next.x, next.y, next.z), FreeCAD.Vector(p.x, p.y, p.z)):
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
            cutter = ocl.BallCutter(obj.ToolController.Tool.Diameter, 5)  # TODO: 5 represents cutting edge height. Should be replaced with the data from toolcontroller?
        else:
            cutter = ocl.CylCutter(obj.ToolController.Tool.Diameter, 5)

        wl.setCutter(cutter)
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

    def _dropcutter(self, obj, s, bb):
        import ocl
        import time
        if obj.ToolController.Tool.ToolType == 'BallEndMill':
            cutter = ocl.BallCutter(obj.ToolController.Tool.Diameter, 5)  # TODO: 5 represents cutting edge height. Should be replaced with the data from toolcontroller?
        else:
            cutter = ocl.CylCutter(obj.ToolController.Tool.Diameter, 5)

        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(s)
        pdc.setCutter(cutter)
        pdc.setZ(obj.FinalDepth.Value + obj.DepthOffset.Value)  # set minimumZ
        pdc.setSampling(obj.SampleInterval)

        # the max and min XY area of the operation
        xmin = bb.XMin - obj.DropCutterExtraOffset.x
        xmax = bb.XMax + obj.DropCutterExtraOffset.x
        ymin = bb.YMin - obj.DropCutterExtraOffset.y
        ymax = bb.YMax + obj.DropCutterExtraOffset.y

        path = ocl.Path()                   # create an empty path object

        if obj.DropCutterDir == 'Y':
            Ny = int(bb.YLength / (cutter.getDiameter() * (obj.StepOver / 100.0)))
            dy = float(ymax - ymin) / Ny  # the y step-over

            # add Line objects to the path in this loop
            for n in xrange(0, Ny):
                y = ymin + n * dy
                p1 = ocl.Point(xmin, y, 0)   # start-point of line
                p2 = ocl.Point(xmax, y, 0)   # end-point of line
                if (n % 2 == 0):  # even
                    l = ocl.Line(p1, p2)     # line-object
                else:  # odd
                    l = ocl.Line(p2, p1)     # line-object

                path.append(l)        # add the line to the path
        else:
            Nx = int(bb.XLength / (cutter.getDiameter() * (obj.StepOver / 100.0)))
            dx = float(xmax - xmin) / Nx  # the y step-over

            # add Line objects to the path in this loop
            for n in xrange(0, Nx):
                x = xmin + n * dx
                p1 = ocl.Point(x, ymin, 0)   # start-point of line
                p2 = ocl.Point(x, ymax, 0)   # end-point of line
                if (n % 2 == 0):  # even
                    l = ocl.Line(p1, p2)     # line-object
                else:  # odd
                    l = ocl.Line(p2, p1)     # line-object

                path.append(l)        # add the line to the path

        pdc.setPath(path)

        # run drop-cutter on the path
        t_before = time.time()
        pdc.run()
        t_after = time.time()
        print("calculation took ", t_after - t_before, " s")

        # retrieve the points
        clp = pdc.getCLPoints()
        print("points received: " + str(len(clp)))

        # generate the path commands
        output = []
        output.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        output.append(Path.Command('G0', {'X': clp[0].x, "Y": clp[0].y, 'F': self.horizRapid}))
        output.append(Path.Command('G1', {'Z': clp[0].z, 'F': self.vertFeed}))
        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        next = ocl.Point(float("inf"), float("inf"), float("inf"))
        optimize = obj.Optimize
        for i in range(0, len(clp)):
            c = clp[i]
            if i < len(clp) - 1:
                next.x = clp[i + 1].x
                next.y = clp[i + 1].y
                next.z = clp[i + 1].z
            else:
                optimize = False

            if not optimize or not self.isPointOnLine(FreeCAD.Vector(prev.x, prev.y, prev.z), FreeCAD.Vector(next.x, next.y, next.z), FreeCAD.Vector(c.x, c.y, c.z)):
                output.append(Path.Command('G1', {'X': c.x, "Y": c.y, "Z": c.z, 'F': self.horizFeed}))
            prev.x = c.x
            prev.y = c.y
            prev.z = c.z
        print("points after optimization: " + str(len(output)))
        return output

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
