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
import math

__title__ = "Base class for PathArea based operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class and properties for Path.Area based operations."
__contributors__ = "mlampert [FreeCAD], russ4262 (Russell Johnson)"
__scriptVersion__ = "1d testing"
__createdDate__ = "2017"
__lastModified__ = "2019-04-23 23:03 CST"

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

    # These are static while document is open, if it contains a 3D Surface Op
    initFinalDepth = None
    initOpFinalDepth = None
    initOpStartDepth = None
    docRestored = False

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
        # Import FinalDepth from existing operation for use in recompute() operations
        self.initFinalDepth = obj.FinalDepth.Value
        self.initOpFinalDepth = obj.OpFinalDepth.Value
        self.docRestored = True
        print("Imported existing OpFinalDepth of " + str(self.initOpFinalDepth) + " for recompute() purposes.")
        print("Imported existing FinalDepth of " + str(self.initFinalDepth) + " for recompute() purposes.")

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

            opHeights = self.opDetermineRotationRadius(obj)  #return is list with tuples [(xRotRad, yRotRad, zRotRad), (clrOfst, safOfst)]
            (xRotRad, yRotRad, zRotRad) = opHeights[0]
            #(clrOfst, safOfset) = opHeights[1]

            maxDep = xRotRad
            if yRotRad > xRotRad:
                maxDep = yRotRad

            # Manage operation start and final depths
            if self.docRestored == True:  # This op is NOT the first in the Operations list
                print("doc restored")
                obj.FinalDepth.Value = obj.OpFinalDepth.Value
            else:
                print("new operation")
                obj.OpFinalDepth.Value = -1 * maxDep
                obj.OpStartDepth.Value = maxDep
                if self.initOpFinalDepth == None and self.initFinalDepth == None:
                    self.initFinalDepth = -1 * maxDep
                    self.initOpFinalDepth = -1 * maxDep
                else:
                    print("-initFinalDepth" + str(self.initFinalDepth))
                    print("-initOpFinalDepth" + str(self.initOpFinalDepth))

            '''
            if shape:
                bb = shape.BoundBox
                obj.OpStartDepth      = bb.ZMax
                obj.OpFinalDepth      = bb.ZMin
            else:
                obj.OpStartDepth      =  1.0
                obj.OpFinalDepth      =  0.0
            '''

        self.areaOpSetDefaultValues(obj, job)

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... overwrite to set initial values of operation specific properties.
        Can safely be overwritten by subclasses.'''
        pass

    def _buildPathArea(self, obj, baseobject, isHole, start, getsim):
        '''_buildPathArea(obj, baseobject, isHole, start, getsim) ... internal function.'''
        PathLog.track()
        area = Path.Area()
        area.setPlane(PathUtils.makeWorkplane(baseobject))
        area.add(baseobject)

        areaParams = self.areaOpAreaParams(obj, isHole)

        heights = [i for i in self.depthparams]
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

        return pp, simobj

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
        print("opExecute() in PathAreaOp.py")
        print(" -Script version: " + __scriptVersion__ + "  Lm: " + __lastModified__)

        # Import OpFinalDepth from pre-existing operation for recompute() scenarios
        if obj.OpFinalDepth.Value != self.initOpFinalDepth:
            if obj.OpFinalDepth.Value == obj.FinalDepth.Value:
                obj.FinalDepth.Value = self.initOpFinalDepth
                obj.OpFinalDepth.Value = self.initOpFinalDepth
            if self.initOpFinalDepth != None:
                obj.OpFinalDepth.Value = self.initOpFinalDepth

        # Instantiate class variables for operation reference
        self.rotateFlag = False

        # Calculate operation heights based upon rotation radii
        opHeights = self.opDetermineRotationRadius(obj)  #return is [(xRotRad, yRotRad, zRotRad), (clrOfst, safOfst)]
        (xRotRad, yRotRad, zRotRad) = opHeights[0]
        (clrOfst, safOfset) = opHeights[1]
        self.leadIn = safOfset / 2.0
        
        # Set clearnance and safe heights based upon rotation radii
        obj.ClearanceHeight.Value = xRotRad + clrOfst
        obj.SafeHeight.Value = xRotRad + safOfset
        if yRotRad > xRotRad:
            obj.ClearanceHeight.Value = yRotRad + clrOfst        
            obj.SafeHeight.Value = yRotRad + safOfset
        

        # Set axial feed rates based upon horizontal feed rates
        safeCircum = 2 * math.pi * obj.SafeHeight.Value
        self.axialFeed = 360 / safeCircum * self.horizFeed
        self.axialRapid = 360 / safeCircum * self.horizRapid

        # Set start point
        if PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
            start = obj.StartPoint
        else:
            start = None

        shapes = self.areaOpShapes(obj)  # list of tuples (shape, isHole, sub, angle, axis, tag)

        jobs = [{
            'x': s[0].BoundBox.XMax,
            'y': s[0].BoundBox.YMax,
            'shape': s
        } for s in shapes]

        jobs = PathUtils.sort_jobs(jobs, ['x', 'y'])

        shapes = [j['shape'] for j in jobs]

        sims = []

        for (shape, isHole, sub, angle, axis, tag, strDep, finDep) in shapes:
            startDep = obj.StartDepth.Value + safOfset #strDep
            safeDep = obj.SafeHeight.Value         
            clearDep = obj.ClearanceHeight.Value
            finalDep = obj.FinalDepth.Value #finDep
            
            finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
            self.depthparams = PathUtils.depth_params(
                    clearance_height=clearDep,  #obj.ClearanceHeight.Value
                    safe_height=safeDep,  #obj.SafeHeight.Value
                    start_depth=startDep,  
                    step_down=obj.StepDown.Value,
                    z_finish_step=finish_step,  #obj.FinalDepth.Value
                    final_depth=finalDep,
                    user_depths=None)

            try:
                (pp, sim) = self._buildPathArea(obj, shape, isHole, start, getsim)
                ppCmds = pp.Commands
                # Rotate model to index for cut
                axisOfRot = 'A'
                if axis == 'Y':
                    axisOfRot = 'B'
                    # Reverse angle temporarily to match model. Error in FreeCAD render of B axis rotations
                    if obj.B_AxisErrorOverride == True:
                        angle = -1 * angle
                ppCmds.insert(0, Path.Command('G0', {axisOfRot: angle, 'F': self.axialFeed}))
                # Raise cutter to safe depth and return index to starting position
                ppCmds.append(Path.Command('G0', {'Z': safeDep, 'F': self.vertRapid}))
                ppCmds.append(Path.Command('G0', {axisOfRot: 0.0, 'F': self.axialFeed}))
                # Save gcode commands to object command list
                self.commandlist.extend(ppCmds)
                sims.append(sim)
            except Exception as e:
                FreeCAD.Console.PrintError(e)
                FreeCAD.Console.PrintError("Something unexpected happened. Check project and tool config.")

            if self.areaOpRetractTool(obj):
                self.endVector = None

        # Raise cutter to safe height and rotate back to original orientation
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

    def opDetermineRotationRadius(self, obj):
        '''opDetermineRotationRadius(self, obj) ... Determine rotational radii for 4th-axis rotations, for clearance/safe heights -- '''

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

    def pocketRotationAnalysis(self, obj, objRef, sub, prnt):
        '''pocketRotationAnalysis(self, obj, objRef, sub) ... Determine X and Y independent rotation necessary to make normalAt = Z=1 -- '''
        
        rtn = False
        axis = 'X'
        orientation = 'X'
        angle = 500.0
        zTol = 1.0E-9
        rndTol = 1.0 - zTol
        testId = "pocketRotationAnalysis() in PathAreaOp.py"

        def roundRoughValues(val, zTol, rndTol):
            # Convert VALxe-15 numbers to zero
            if math.fabs(val) <= zTol:
                return 0.0            
            # Convert VAL.99999999 to next integer
            elif math.fabs(val % 1) > rndTol:
                return round(val)
            else:
                return val

        face = objRef.Shape.getElement(sub)

        norm = face.normalAt(0,0)
        nX = roundRoughValues(norm.x, zTol, rndTol)
        nY = roundRoughValues(norm.y, zTol, rndTol)
        nZ = roundRoughValues(norm.z, zTol, rndTol)
        testId += "\n -normalAt(0,0): " + str(nX) + ", " + str(nY) + ", " + str(nZ)
        
        surf = face.Surface.Axis
        saX = roundRoughValues(surf.x, zTol, rndTol)
        saY = roundRoughValues(surf.y, zTol, rndTol)
        saZ = roundRoughValues(surf.z, zTol, rndTol)
        testId += "\n -Surface.Axis: " + str(saX) + ", " + str(saY) + ", " + str(saZ)

        # Determine rotation needed and current orientation
        if saX == 0.0:
            if saY == 0.0:
                orientation = "Z"
                if saZ == 1.0:
                    angle = 0.0
                elif saZ == -1.0:
                    angle = -180.0
                else:
                    testId += "_else_X" + str(saZ)
            elif saY == 1.0:
                orientation = "Y"
                angle = 90.0
            elif saY == -1.0:
                orientation = "Y"
                angle = -90.0
            else:
                if saZ != 0.0:
                    angle = math.degrees(math.atan(saY / saZ))
                    orientation = "Y"
        elif saY == 0.0:
            if saZ == 0.0:
                orientation = "X"
                if saX == 1.0:
                    angle = -90.0
                elif saX == -1.0:
                    angle = 90.0
                else:
                    testId += "_else_X" + str(saX)
            else:
                orientation = "X"
                ratio = saX / saZ
                angle = math.degrees(math.atan(ratio))
                if ratio < 0.0:
                    testId += " NEG-ratio"
                    angle -= 90
                else:
                    testId += " POS-ratio"
                    angle = -1 * angle
                    if saX < 0.0:
                        angle = angle + 180.0
        elif saZ == 0.0:
            if saY != 0.0:
                angle = math.degrees(math.atan(saX / saY))
                orientation = "Y"
        
        if saX + nX == 0.0:
            angle = -1 * angle
        if saY + nY == 0.0:
            angle = -1 * angle
        if saZ + nZ == 0.0:
            angle = -1 * angle

        if saY == -1.0 or saY == 1.0:
            if nX != 0.0:
                angle = -1 * angle

        if angle != 500.0 and angle != 0.0:
            self.rotateFlag = True
            rtn = True
            if orientation == 'Y':
                axis = 'X'
            else:
                axis = 'Y'
            if obj.ReverseDirection == True:
                if angle < 180.0:
                    angle = angle + 180.0
                else:
                    angle = angle - 180.0
                
        else:
            angle = 0.0 # No rotation needed

        testId += "\n -Suggested rotation:  angle: " + str(angle) + ",   axis: " + str(axis)
        if prnt == True:
            print("testId: " + testId)
        return (rtn, angle, axis)


