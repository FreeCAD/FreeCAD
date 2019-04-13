# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils

from PathScripts.PathUtils import waiting_effects
from PySide import QtCore

__title__ = "Base class for PathArea based operations."
__author__ = "sliptonic (Brad Collette)"
__contributors__ = "mlampert [FreeCAD], russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class and properties for Path.Area based operations."
__scriptVersion__ = "1a Stable"
__created__ = "2017"
__lastModified__ = "2019-04-13 13:17 CST"

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule()
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectOp(PathOp.ObjectOp):
    '''Base class for all Path.Area based operations.
    Provides standard features including debugging properties AreaParams,
    PathParams and removalshape, all hidden.
    The main reason for existence is to implement the standard interface
    to Path.Area so subclasses only have to provide the shapes for the
    operations.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... returns the base features supported by all Path.Area based operations.
        The standard feature list is OR'ed with the return value of areaOpFeatures().
        Do not overwrite, implement areaOpFeatures(obj) instead.'''
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureStepDown | PathOp.FeatureHeights | PathOp.FeatureStartPoint | self.areaOpFeatures(obj)

    def areaOpFeatures(self, obj):
        '''areaOpFeatures(obj) ... overwrite to add operation specific features.
        Can safely be overwritten by subclasses.'''
        return 0

    def initOperation(self, obj):
        '''initOperation(obj) ... sets up standard Path.Area properties and calls initAreaOp().
        Do not overwrite, overwrite initAreaOp(obj) instead.'''
        PathLog.track()

        # Debugging
        obj.addProperty("App::PropertyString", "AreaParams", "Path")
        obj.setEditorMode('AreaParams', 2)  # hide
        obj.addProperty("App::PropertyString", "PathParams", "Path")
        obj.setEditorMode('PathParams', 2)  # hide
        obj.addProperty("Part::PropertyPartShape", "removalshape", "Path")
        obj.setEditorMode('removalshape', 2)  # hide

        self.initAreaOp(obj)

    def initAreaOp(self, obj):
        '''initAreaOp(obj) ... overwrite if the receiver class needs initialisation.
        Can safely be overwritten by subclasses.'''
        pass

    def areaOpShapeForDepths(self, obj):
        '''areaOpShapeForDepths(obj) ... returns the shape used to make an initial calculation for the depths being used.
        The default implementation returns the job's Base.Shape'''
        job = PathUtils.findParentJob(obj)
        if job and job.Base:
            PathLog.debug("job=%s base=%s shape=%s" % (job, job.Base, job.Base.Shape))
            return job.Base.Shape
        if job:
            PathLog.warning(translate("PathAreaOp", "job %s has no Base.") % job.Label)
        else:
            PathLog.warning(translate("PathAreaOp", "no job for op %s found.") % obj.Label)
        return None

    def areaOpOnChanged(self, obj, prop):
        '''areaOpOnChanged(obj, porp) ... overwrite to process operation specific changes to properties.
        Can safely be overwritten by subclasses.'''
        pass

    def opOnChanged(self, obj, prop):
        '''opOnChanged(obj, prop) ... base implementation of the notification framework - do not overwrite.
        The base implementation takes a stab at determining Heights and Depths if the operations's Base
        changes.
        Do not overwrite, overwrite areaOpOnChanged(obj, prop) instead.'''
        #PathLog.track(obj.Label, prop)
        if prop in ['AreaParams', 'PathParams', 'removalshape']:
            obj.setEditorMode(prop, 2)

        if prop == 'Base' and len(obj.Base) == 1:
            (base, sub) = obj.Base[0]
            bb = base.Shape.BoundBox  # parent boundbox
            subobj = base.Shape.getElement(sub[0])
            fbb = subobj.BoundBox  # feature boundbox

            if hasattr(obj, 'Side'):
                if bb.XLength == fbb.XLength and bb.YLength == fbb.YLength:
                    obj.Side = "Outside"
                else:
                    obj.Side = "Inside"

        self.areaOpOnChanged(obj, prop)

    def opOnDocumentRestored(self, obj):
        for prop in ['AreaParams', 'PathParams', 'removalshape']:
            if hasattr(obj, prop):
                obj.setEditorMode(prop, 2)
        self.areaOpOnDocumentRestored(obj)

    def areaOpOnDocumentRestored(self, obj):
        '''areaOpOnDocumentRestored(obj) ... overwrite to fully restore receiver'''
        pass

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj) ... base implementation, do not overwrite.
        The base implementation sets the depths and heights based on the
        areaOpShapeForDepths() return value.
        Do not overwrite, overwrite areaOpSetDefaultValues(obj, job) instead.'''
        PathLog.debug("opSetDefaultValues(%s, %s)" % (obj.Label, job.Label))
        if PathOp.FeatureDepths & self.opFeatures(obj):
            try:
                shape = self.areaOpShapeForDepths(obj)
            except:
                shape = None

            if shape:
                bb = shape.BoundBox
                obj.OpStartDepth      = bb.ZMax
                obj.OpFinalDepth      = bb.ZMin
            else:
                obj.OpStartDepth      =  1.0
                obj.OpFinalDepth      =  0.0

        self.areaOpSetDefaultValues(obj, job)

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... overwrite to set initial values of operation specific properties.
        Can safely be overwritten by subclasses.'''
        pass

    def _buildPathArea(self, obj, baseobject, isHole, angle, axis, start, getsim, depthparams, safeDep):
        '''_buildPathArea(obj, baseobject, isHole, start, getsim) ... internal function.'''
        PathLog.track()
        area = Path.Area()
        area.setPlane(PathUtils.makeWorkplane(baseobject))
        area.add(baseobject)

        areaParams = self.areaOpAreaParams(obj, isHole)

        #heights = [i for i in self.depthparams]
        heights = [i for i in depthparams]
        PathLog.debug('depths: {}'.format(heights))
        area.setParams(**areaParams)
        obj.AreaParams = str(area.getParams())

        PathLog.debug("Area with params: {}".format(area.getParams()))

        sections = area.makeSections(mode=0, project=self.areaOpUseProjection(obj), heights=heights)
        PathLog.debug("sections = %s" % sections)
        shapelist = [sec.getShape() for sec in sections]
        PathLog.debug("shapelist = %s" % shapelist)

        pathParams = self.areaOpPathParams(obj, isHole)
        pathParams['shapes'] = shapelist
        pathParams['feedrate'] = self.horizFeed
        pathParams['feedrate_v'] = self.vertFeed
        pathParams['verbose'] = True
        pathParams['resume_height'] = obj.SafeHeight.Value
        pathParams['retraction'] = obj.ClearanceHeight.Value
        pathParams['return_end'] = True
        # Note that emitting preambles between moves breaks some dressups and prevents path optimization on some controllers
        pathParams['preamble'] = False

        if not self.areaOpRetractTool(obj):
            pathParams['threshold'] = 2.001 * self.radius

        if self.endVector is not None:
            pathParams['start'] = self.endVector
        elif PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
            pathParams['start'] = obj.StartPoint

        obj.PathParams = str({key: value for key, value in pathParams.items() if key != 'shapes'})
        PathLog.debug("Path with params: {}".format(obj.PathParams))

        (pp, end_vector) = Path.fromShapes(**pathParams)

        PathLog.debug('pp: {}, end vector: {}'.format(pp, end_vector))
        self.endVector = end_vector

        simobj = None
        if getsim:
            areaParams['Thicken'] = True
            areaParams['ToolRadius'] = self.radius - self.radius * .005
            area.setParams(**areaParams)
            sec = area.makeSections(mode=0, project=False, heights=heights)[-1].getShape()
            simobj = sec.extrude(FreeCAD.Vector(0, 0, baseobject.BoundBox.ZMax))
        
        # Russ4262
        print("_buildPathArea() in PathAreaOp.py: angle: " + str(angle) + ", axis: " + str(axis))
        cmds = []
        cmds = pp.Commands
        axisOfRot = 'A'
        if axis == 'Y':
            axisOfRot = 'B'
        cmds.insert(0, Path.Command('G0', {axisOfRot: angle, 'F': self.axialFeed}))
        cmds.append(Path.Command('G0', {'Z': safeDep, 'F': self.vertRapid}))
        cmds.append(Path.Command('G0', {axisOfRot: 0.0, 'F': self.axialFeed}))

        return cmds, simobj

    def opExecute(self, obj, getsim=False):
        '''opExecute(obj, getsim=False) ... implementation of Path.Area ops.
        determines the parameters for _buildPathArea().
        Do not overwrite, implement
            areaOpAreaParams(obj, isHole) ... op specific area param dictionary
            areaOpPathParams(obj, isHole) ... op specific path param dictionary
            areaOpShapes(obj)             ... the shape for path area to process
            areaOpUseProjection(obj)      ... return true if operation can use projection
        instead.'''
        PathLog.track()
        self.endVector = None

        finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
        self.depthparams = PathUtils.depth_params(
                clearance_height=obj.ClearanceHeight.Value,
                safe_height=obj.SafeHeight.Value,
                start_depth=obj.StartDepth.Value,
                step_down=obj.StepDown.Value,
                z_finish_step=finish_step,
                final_depth=obj.FinalDepth.Value,
                user_depths=None)

        if PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
            start = obj.StartPoint
        else:
            start = None

        # Calculate operation heights based upon rotation radii
        opHeights = self.opDetermineRotationRadius(obj)  #return is [(xRotRad, yRotRad, zRotRad), (clrOfst, safOfst)]
        (xRotRad, yRotRad, zRotRad) = opHeights[0]
        (clrOfst, safOfset) = opHeights[1]

        obj.ClearanceHeight.Value = xRotRad + clrOfst
        if yRotRad > xRotRad:
            obj.ClearanceHeight.Value = yRotRad + clrOfst
        
        obj.SafeHeight.Value = xRotRad + safOfset
        if yRotRad > xRotRad:
            obj.SafeHeight.Value = yRotRad + safOfset

        shapes = self.areaOpShapes(obj)

        jobs = [{
            'x': s[0].BoundBox.XMax,
            'y': s[0].BoundBox.YMax,
            'shape': s
        } for s in shapes]

        jobs = PathUtils.sort_jobs(jobs, ['x', 'y'])

        shapes = [j['shape'] for j in jobs]

        sims = []
        for (shape, isHole, depths, (angle, axis), rotang) in shapes:
            (strDep, finDep) = depths  # unpack tuple
            clearDep = obj.ClearanceHeight.Value
            safeDep = obj.SafeHeight.Value
            
            startDep = strDep + safOfset
            #if angle != rotang:
            #    print("angle != rotang")
            #    # startDep = strDep + 10

            finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
            #self.depthparams = PathUtils.depth_params(
            depthparams = PathUtils.depth_params(
                    clearance_height=clearDep,  #obj.ClearanceHeight.Value
                    safe_height=safeDep,  #obj.SafeHeight.Value
                    start_depth=startDep,  #obj.StartDepth.Value
                    step_down=obj.StepDown.Value,
                    z_finish_step=finish_step,  #obj.FinalDepth.Value
                    final_depth=finDep,
                    user_depths=None)

            try:
                (cmds, sim) = self._buildPathArea(obj, shape, isHole, rotang, axis, start, getsim, depthparams, safeDep)
                self.commandlist.extend(cmds)
                sims.append(sim)
            except Exception as e:
                FreeCAD.Console.PrintError(e)
                FreeCAD.Console.PrintError("Something unexpected happened. Check project and tool config.")

            if self.areaOpRetractTool(obj):
                self.endVector = None
        
        # Russ4262
        if self.rotateFlag == True:
            self.commandlist.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
            self.commandlist.append(Path.Command('G0', {'A': 0.0, 'F': self.axialFeed}))
            self.commandlist.append(Path.Command('G0', {'B': 0.0, 'F': self.axialFeed}))

        return sims

    def areaOpRetractTool(self, obj):
        '''areaOpRetractTool(obj) ... return False to keep the tool at current level between shapes. Default is True.'''
        return True

    def areaOpAreaParams(self, obj, isHole):
        '''areaOpAreaParams(obj, isHole) ... return operation specific area parameters in a dictionary.
        Note that the resulting parameters are stored in the property AreaParams.
        Must be overwritten by subclasses.'''
        pass
    def areaOpPathParams(self, obj, isHole):
        '''areaOpPathParams(obj, isHole) ... return operation specific path parameters in a dictionary.
        Note that the resulting parameters are stored in the property PathParams.
        Must be overwritten by subclasses.'''
        pass
    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return all shapes to be processed by Path.Area for this op.
        Must be overwritten by subclasses.'''
        pass
    def areaOpUseProjection(self, obj):
        '''areaOpUseProcjection(obj) ... return True if the operation can use procjection, defaults to False.
        Can safely be overwritten by subclasses.'''
        return False
    
    def areaOpDetermineFaceRotationNeeded(self, obj, objRef, sub):
        '''areaOpDetermineFaceRotationNeeded(self, obj, objRef, sub) ... Determine X and Y independent rotation necessary to make normalAt = Z=1 -- '''
        
        import math
        rtn = False
        axis = 'X'
        orientation = 'X'
        angle = 500.0
        axisVect = FreeCAD.Vector(1,0,0)
        zeroTolerance = 0.00000001
        nineNineTolerance = 1.0 - zeroTolerance
        testId = "areaOpDetermineFaceRotationNeeded() in PathOp.py::  "
        reverse = 1

        face = objRef.Shape.getElement(sub)
        #norm = face.normalAt(0,0)
        #print("normalAt: " + str(norm))
        norm = face.Surface.Axis
        #print("PRE::: sub: " + str(sub) + " - Surface.Axis: " + str(face.Surface.Axis))
        nX = norm.x
        nY = norm.y
        nZ = norm.z

        # Convert VALxe-15 numbers to zero
        if math.fabs(nX) <= zeroTolerance:
            nX = 0.0 
        if math.fabs(nY) <= zeroTolerance:
            nY = 0.0 
        if math.fabs(nZ) <= zeroTolerance:
            nZ = 0.0 
        
        if nX % 1 > nineNineTolerance:
            nX = round(nX)

        if nX == 0.0:
            testId += 'X0'
            if nY == 0.0:
                testId += 'Y0'
                orientation = 'Z'
                if nZ == 1.0:
                    testId += 'Z1'
                    angle = 0.0
                elif nZ == -1.0:
                    testId += 'Z-1'
                    angle = 180.0
            elif nY == 1.0:
                testId += 'Y1'
                orientation = 'Y'
                angle = 90.0
            elif nY == -1.0:
                testId += 'Y-1'
                orientation = 'Y'
                angle = -90.0
            else:
                testId += 'Y!0'
                if nZ != 0.0:
                    testId += 'Z!0'
                    angle = math.degrees(math.atan(nY / nZ))
                    orientation = 'Y'
        elif nY == 0.0:
            testId += 'Y0'
            if nZ == 0.0:
                testId += 'Z0'
                orientation = 'X'
                if nX == 1.0:
                    testId += 'X1'
                    angle = -90.0
                    reverse = -1
                elif nX == -1.0:
                    testId += 'X-1'
                    angle = 90.0
            else:
                testId += 'Z!0'
                if nX < 0.0:
                    testId += ' -X'
                else:
                    testId += ' +X'
                if nZ < 0.0:
                    testId += ' -Z'
                else:
                    testId += ' +Z'
                
                ratio = nX / nZ
                angle = math.degrees(math.atan(ratio))
                if ratio < 0.0:
                    testId += ' NEG-ratio'
                    reverse = -1
                    angle -= 90
                else:
                    angle = -1 * angle
                orientation = 'X'
        elif nZ == 0.0:
            testId += 'Z0'
            if nY != 0.0:
                testId += 'Y!0'
                angle = math.degrees(math.atan(nX / nY))
                orientation = 'Y'
        #print("testId: " + testId)

        if angle != 500.0 and angle != 0.0:
            self.rotateFlag = True
            obj.IndexAngle = angle
            rtn = True
            if orientation == 'Y':
                axis = 'X'
                axisVect = FreeCAD.Vector(1,0,0)
                obj.RotationAxis = axis
            else:
                axis = 'Y'
                axisVect = FreeCAD.Vector(0,1,0)
                obj.RotationAxis = axis
        else:
            angle = 0.0 # No rotation needed

        rotate = angle * reverse

        print("Suggested rotation to normal::: angle: " + str(angle) + ",   axis: " + str(axis) + ",   rotate: " + str(rotate))
        return (rtn, angle, axis, rotate)
    def opDetermineRotationRadius(self, obj):
        '''opDetermineRotationRadius(self, obj) ... Determine rotational radii for 4th-axis rotations, for clearance/safe heights -- '''
        import math
        parentJob = PathUtils.findParentJob(obj)
        bb = parentJob.Stock.Shape.BoundBox
        # Determine boundbox radius based upon xzy limits data
        if math.fabs(bb.ZMin) > math.fabs(bb.ZMax):
            zlim = bb.ZMin
        else:
            zlim = bb.ZMax                    

        # Rotation is around X-axis, cutter moves along same axis
        if math.fabs(bb.YMin) > math.fabs(bb.YMax):
            ylim = bb.YMin
        else:
            ylim = bb.YMax

        # Rotation is around Y-axis, cutter moves along same axis
        if math.fabs(bb.XMin) > math.fabs(bb.XMax):
            xlim = bb.XMin
        else:
            xlim = bb.XMax
        
        xRotRad = math.sqrt(ylim**2 + zlim**2)
        yRotRad = math.sqrt(xlim**2 + zlim**2)
        zRotRad = math.sqrt(xlim**2 + ylim**2)

        clrOfst = parentJob.SetupSheet.ClearanceHeightOffset.Value
        safOfst = parentJob.SetupSheet.ClearanceHeightOffset.Value
    
        return [(xRotRad, yRotRad, zRotRad), (clrOfst, safOfst)]

