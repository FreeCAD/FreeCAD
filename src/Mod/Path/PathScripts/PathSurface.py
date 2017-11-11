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
#import Part
import Path
import PathScripts.PathLog as PathLog
#import PathScripts.PathPocketBase as PathPocketBase
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
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureFinishDepth;

    def initOperation(self, obj):
        '''initPocketOp(obj) ... create facing specific properties'''
        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "The library to use to generate the path"))
        obj.Algorithm = ['OCL Dropcutter', 'OCL Waterline']
        obj.addProperty("App::PropertyFloatConstraint", "SampleInterval", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "The Sample Interval.  Small values cause long wait"))
        obj.SampleInterval = (0.04, 0.01, 1.0, 0.01)

    def opExecute(self, obj):
        '''opExecute(obj) ... process engraving operation'''
        PathLog.track()

        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment)+')\n'


        output += "(" + obj.Label + ")"
        output += "(Compensated Tool Path. Diameter: " + str(obj.ToolController.Tool.Diameter) + ")"


        parentJob = PathUtils.findParentJob(obj)
        if parentJob is None:
            return

        print("base object: " + self.baseobject.Name)

        if obj.Algorithm in ['OCL Dropcutter', 'OCL Waterline']:
            try:
                import ocl
            except:
                FreeCAD.Console.PrintError(
                        translate("Path_Surface", "This operation requires OpenCamLib to be installed.\n"))
                return

        if self.baseobject.TypeId.startswith('Mesh'):
            mesh = self.baseobject.Mesh
        else:
            # try/except is for Path Jobs created before GeometryTolerance
            try:
                deflection = parentJob.GeometryTolerance
            except AttributeError:
                from PathScripts.PathPreferences import PathPreferences
                deflection = PathPreferences.defaultGeometryTolerance()
            self.baseobject.Shape.tessellate(0.5)
            mesh = MeshPart.meshFromShape(self.baseobject.Shape, Deflection=deflection)

        bb = mesh.BoundBox

        s = ocl.STLSurf()
        for f in mesh.Facets:
            p = f.Points[0]
            q = f.Points[1]
            r = f.Points[2]
            t = ocl.Triangle(ocl.Point(p[0], p[1], p[2]), ocl.Point(
                q[0], q[1], q[2]), ocl.Point(r[0], r[1], r[2]))
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
            pp.append(Path.Command("(waterline begin)" ))

            for loop in loops:
                p = loop[0]
                pp.append(Path.Command("(loop begin)" ))
                pp.append(Path.Command('G0', {"Z": obj.SafeHeight.Value, 'F': self.vertRapid}))
                pp.append(Path.Command('G0', {'X': p.x, "Y": p.y, 'F': self.horizRapid}))
                pp.append(Path.Command('G1', {"Z": p.z, 'F': self.vertFeed}))

                for p in loop[1:]:
                    pp.append(Path.Command('G1', {'X': p.x, "Y": p.y, "Z": p.z, 'F': self.horizFeed}))
                   # zheight = p.z
                p = loop[0]
                pp.append(Path.Command('G1', {'X': p.x, "Y": p.y, "Z": p.z, 'F': self.horizFeed}))
                pp.append(Path.Command("(loop end)" ))

                print("    loop ", nloop, " with ", len(loop), " points")
                nloop = nloop + 1
            pp.append(Path.Command("(waterline end)" ))

            return pp

        depthparams = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value,
                                   obj.StartDepth.Value, obj.StepDown, obj.FinishDepth.Value, obj.FinalDepth.Value)

        t_before = time.time()
        zheights = [i for i in depthparams]

        wl = ocl.Waterline()
        wl.setSTL(s)
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

    def _dropcutter(self, obj, s, bb):
        import ocl
        import time

        cutter = ocl.CylCutter(obj.ToolController.Tool.Diameter, 5)
        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(s)
        pdc.setCutter(cutter)
        pdc.minimumZ = 0.25
        pdc.setSampling(obj.SampleInterval)

        # some parameters for this "zigzig" pattern
        xmin = bb.XMin - cutter.getDiameter()
        xmax = bb.XMax + cutter.getDiameter()
        ymin = bb.YMin - cutter.getDiameter()
        ymax = bb.YMax + cutter.getDiameter()

        # number of lines in the y-direction
        Ny = int(bb.YLength / cutter.getDiameter())
        dy = float(ymax - ymin) / Ny  # the y step-over

        path = ocl.Path()                   # create an empty path object

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

        for c in clp:
            output.append(Path.Command('G1', {'X': c.x, "Y": c.y, "Z": c.z, 'F': self.horizFeed}))

        return output

    def pocketInvertExtraOffset(self):
        return True

    def areaOpSetDefaultValues(self, obj):
        '''areaOpSetDefaultValues(obj) ... initialize mill facing properties'''
       # obj.StepOver = 50
       # obj.ZigZagAngle = 45.0

        # need to overwrite the default depth calculations for facing
        job = PathUtils.findParentJob(obj)
        if job and job.Base:
            d = PathUtils.guessDepths(job.Base.Shape, None)
            obj.OpStartDepth = d.safe_height
            obj.OpFinalDepth = d.start_depth

def Create(name):
    '''Create(name) ... Creates and returns a Surface operation.'''
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectSurface(obj)
    return obj
