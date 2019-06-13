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

# SCRIPT NOTES:
# - FUTURE: Relocate rotational calculations to Job setup tool, creating a Machine section
#          with axis & rotation toggles and associated min/max values

import FreeCAD
import FreeCADGui
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
import Draft

# from PathScripts.PathUtils import waiting_effects
from PySide import QtCore
import math

__title__ = "Base class for PathArea based operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class and properties for Path.Area based operations."
__contributors__ = "mlampert [FreeCAD], russ4262 (Russell Johnson)"
__createdDate__ = "2017"
__scriptVersion__ = "2e testing"
__lastModified__ = "2019-06-11 14:30 CST"

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule()
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt translation handling
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
    initOpFinalDepth = None
    initOpStartDepth = None
    initWithRotation = False
    defValsSet = False
    docRestored = False

    def opFeatures(self, obj):
        '''opFeatures(obj) ... returns the base features supported by all Path.Area based operations.
        The standard feature list is OR'ed with the return value of areaOpFeatures().
        Do not overwrite, implement areaOpFeatures(obj) instead.'''
        # return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureStepDown | PathOp.FeatureHeights | PathOp.FeatureStartPoint | self.areaOpFeatures(obj) | PathOp.FeatureRotation
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

        self.setupAdditionalProperties(obj)

        self.initAreaOp(obj)

    def setupAdditionalProperties(self, obj):
        if not hasattr(obj, 'EnableRotation'):
            obj.addProperty("App::PropertyEnumeration", "EnableRotation", "Rotation", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable rotation to gain access to pockets/areas not normal to Z axis."))
            obj.EnableRotation = ['Off', 'A(x)', 'B(y)', 'A & B']

    def initAreaOp(self, obj):
        '''initAreaOp(obj) ... overwrite if the receiver class needs initialisation.
        Can safely be overwritten by subclasses.'''
        pass

    def areaOpShapeForDepths(self, obj, job):
        '''areaOpShapeForDepths(obj) ... returns the shape used to make an initial calculation for the depths being used.
        The default implementation returns the job's Base.Shape'''
        if job:
            if job.Stock:
                PathLog.debug("job=%s base=%s shape=%s" % (job, job.Stock, job.Stock.Shape))
                return job.Stock.Shape
            else:
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
        # PathLog.track(obj.Label, prop)
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
        
        self.initOpFinalDepth = obj.OpFinalDepth.Value
        self.initOpStartDepth = obj.OpStartDepth.Value
        self.docRestored = True
        #PathLog.debug("Imported existing OpFinalDepth of " + str(self.initOpFinalDepth) + " for recompute() purposes.")
        #PathLog.debug("Imported existing StartDepth of " + str(self.initOpStartDepth) + " for recompute() purposes.")

        self.setupAdditionalProperties(obj)
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
        
        # Initial setting for EnableRotation is taken from Job settings/SetupSheet
        # User may override on per-operation basis as needed.
        if hasattr(job.SetupSheet, 'SetupEnableRotation'):
            obj.EnableRotation = job.SetupSheet.SetupEnableRotation
        else:
            obj.EnableRotation = 'Off'
        PathLog.debug("opSetDefaultValues(): Enable Rotation: {}".format(obj.EnableRotation))

        if PathOp.FeatureDepths & self.opFeatures(obj):
            try:
                shape = self.areaOpShapeForDepths(obj, job)
            except:
                shape = None

            # Set initial start and final depths
            if shape is None:
                PathLog.debug("shape is None")
                startDepth = 1.0
                finalDepth = 0.0
            else:
                bb = job.Stock.Shape.BoundBox
                startDepth = bb.ZMax
                finalDepth = bb.ZMin

            # Adjust start and final depths if rotation is enabled
            if obj.EnableRotation != 'Off':
                self.initWithRotation = True
                # Calculate rotational distances/radii
                opHeights = self.opDetermineRotationRadii(obj)  # return is list with tuples [(xRotRad, yRotRad, zRotRad), (clrOfst, safOfset)]
                (xRotRad, yRotRad, zRotRad) = opHeights[0]
                # (self.safOfset, self.safOfst) = opHeights[1]
                PathLog.debug("opHeights[0]: " + str(opHeights[0]))
                PathLog.debug("opHeights[1]: " + str(opHeights[1]))

                if obj.EnableRotation == 'A(x)':
                    startDepth = xRotRad
                if obj.EnableRotation == 'B(y)':
                    startDepth = yRotRad
                else:
                    startDepth = max(xRotRad, yRotRad)
                finalDepth = -1 * startDepth

            # Manage operation start and final depths
            if self.docRestored is True:  # This op is NOT the first in the Operations list
                PathLog.debug("Doc restored")
                obj.FinalDepth.Value = obj.OpFinalDepth.Value
                obj.StartDepth.Value = obj.OpStartDepth.Value
            else:
                PathLog.debug("New operation")
                obj.StartDepth.Value = startDepth
                obj.FinalDepth.Value = finalDepth
                obj.OpStartDepth.Value = startDepth
                obj.OpFinalDepth.Value = finalDepth
                # obj.OpStartDepth.UserString = str(startDepth) + ' mm'  # Read-only
                # obj.OpFinalDepth.UserString = str(finalDepth) + ' mm'  # Read-only

                if obj.EnableRotation != 'Off':
                    if self.initOpFinalDepth is None:
                        self.initOpFinalDepth = finalDepth
                        PathLog.debug("Saved self.initOpFinalDepth")
                    if self.initOpStartDepth is None:
                        self.initOpStartDepth = startDepth
                        PathLog.debug("Saved self.initOpStartDepth")
                    self.defValsSet = True
            PathLog.debug("Default OpDepths are Start: {}, and Final: {}".format(obj.OpStartDepth.Value, obj.OpFinalDepth.Value))
            PathLog.debug("Default Depths are Start: {}, and Final: {}".format(startDepth, finalDepth))

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
        PathLog.debug("\n\n----- opExecute() in PathAreaOp.py")
        PathLog.track()
        self.endVector = None
        PathLog.debug("\n\n----- opExecute() in PathAreaOp.py")
        # PathLog.debug("OpDepths are Start: {}, and Final: {}".format(obj.OpStartDepth.Value, obj.OpFinalDepth.Value))
        # PathLog.debug("Depths are Start: {}, and Final: {}".format(obj.StartDepth.Value, obj.FinalDepth.Value))
        # PathLog.debug("initOpDepths are Start: {}, and Final: {}".format(self.initOpStartDepth, self.initOpFinalDepth))

        # Instantiate class variables for operation reference
        self.endVector = None
        self.rotateFlag = False
        self.leadIn = 2.0  # self.safOfst / 2.0
        self.cloneNames = []
        self.guiMsgs = []  # list of message tuples (title, msg) to be displayed in GUI 
        self.stockBB = PathUtils.findParentJob(obj).Stock.Shape.BoundBox

        # Import OpFinalDepth from pre-existing operation for recompute() scenarios
        if self.defValsSet is True:
            PathLog.debug("self.defValsSet is True.")
            if self.initOpStartDepth is not None:
                if self.initOpStartDepth != obj.OpStartDepth.Value:
                    obj.OpStartDepth.Value = self.initOpStartDepth
                    obj.StartDepth.Value = self.initOpStartDepth

            if self.initOpFinalDepth is not None:
                if self.initOpFinalDepth != obj.OpFinalDepth.Value:
                    obj.OpFinalDepth.Value = self.initOpFinalDepth
                    obj.FinalDepth.Value = self.initOpFinalDepth
            self.defValsSet = False

        if obj.EnableRotation != 'Off':
            self.useRotJobClones('Start')
            # Calculate operation heights based upon rotation radii
            opHeights = self.opDetermineRotationRadii(obj)
            (self.xRotRad, self.yRotRad, self.zRotRad) = opHeights[0]
            (self.safOfset, self.safOfst) = opHeights[1]

            # Set clearnance and safe heights based upon rotation radii
            if obj.EnableRotation == 'A(x)':
                self.strDep = self.xRotRad
            elif obj.EnableRotation == 'B(y)':
                self.strDep = self.yRotRad
            else:
                self.strDep = max(self.xRotRad, self.yRotRad)
                self.finDep = -1 * self.strDep

            obj.ClearanceHeight.Value = self.strDep + self.safOfset
            obj.SafeHeight.Value = self.strDep + self.safOfst

            # Set axial feed rates based upon horizontal feed rates
            safeCircum = 2 * math.pi * obj.SafeHeight.Value
            self.axialFeed = 360 / safeCircum * self.horizFeed
            self.axialRapid = 360 / safeCircum * self.horizRapid

            if self.initWithRotation == False:
                if obj.FinalDepth.Value == obj.OpFinalDepth.Value:
                    obj.FinalDepth.Value = self.finDep
                if obj.StartDepth.Value == obj.OpStartDepth.Value:
                    obj.StartDepth.Value = self.strDep

            # Create visual axises for debugging purposes.
            if PathLog.getLevel() == 2:
                self.visualAxis()
        else:
            self.strDep = obj.StartDepth.Value
            self.finDep = obj.FinalDepth.Value

        # Initiate depthparams and calculate operation heights for rotational operation
        finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
        self.depthparams = PathUtils.depth_params(
            clearance_height=obj.ClearanceHeight.Value,
            safe_height=obj.SafeHeight.Value,
            start_depth=obj.StartDepth.Value,
            step_down=obj.StepDown.Value,
            z_finish_step=finish_step,
            final_depth=obj.FinalDepth.Value,
            user_depths=None)

        # Set start point
        if PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
            start = obj.StartPoint
        else:
            start = None

        aOS = self.areaOpShapes(obj)  # list of tuples (shape, isHole, sub, angle, axis)

        # Adjust tuples length received from other PathWB tools/operations beside PathPocketShape
        shapes = []
        for shp in aOS:
            if len(shp) == 2:
                (fc, iH) = shp
                #    fc, iH,   sub,     angle, axis
                tup = fc, iH, 'otherOp', 0.0, 'X', obj.StartDepth.Value, obj.FinalDepth.Value
                shapes.append(tup)
            else:
                shapes.append(shp)

        if len(shapes) > 1:
            jobs = [{
                'x': s[0].BoundBox.XMax,
                'y': s[0].BoundBox.YMax,
                'shape': s
            } for s in shapes]

            jobs = PathUtils.sort_jobs(jobs, ['x', 'y'])

            shapes = [j['shape'] for j in jobs]

        # PathLog.debug("Pre_path depths are Start: {}, and Final: {}".format(obj.StartDepth.Value, obj.FinalDepth.Value))
        sims = []
        numShapes = len(shapes)
        if numShapes == 1:
            nextAxis = shapes[0][4]
        elif numShapes > 1:
            nextAxis = shapes[1][4]
        else:
            nextAxis = 'X'

        for ns in range(0, numShapes):
            (shape, isHole, sub, angle, axis, strDep, finDep) = shapes[ns]
            if ns < numShapes - 1:
                nextAxis = shapes[ns + 1][4]
            else:
                nextAxis = 'L'

            finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
            self.depthparams = PathUtils.depth_params(
                clearance_height=obj.ClearanceHeight.Value,
                safe_height=obj.SafeHeight.Value,
                start_depth=strDep,  # obj.StartDepth.Value,
                step_down=obj.StepDown.Value,
                z_finish_step=finish_step,
                final_depth=finDep,  # obj.FinalDepth.Value,
                user_depths=None)

            try:
                (pp, sim) = self._buildPathArea(obj, shape, isHole, start, getsim)
            except Exception as e:
                FreeCAD.Console.PrintError(e)
                FreeCAD.Console.PrintError("Something unexpected happened. Check project and tool config.")
            else:
                ppCmds = pp.Commands
                if obj.EnableRotation != 'Off' and self.rotateFlag is True:
                    # Rotate model to index for cut
                    axisOfRot = 'A'
                    if axis == 'Y':
                        axisOfRot = 'B'
                        # Reverse angle temporarily to match model. Error in FreeCAD render of B axis rotations
                        if obj.B_AxisErrorOverride is True:
                            angle = -1 * angle
                    # Rotate Model to correct angle
                    ppCmds.insert(0, Path.Command('G1', {axisOfRot: angle, 'F': self.axialFeed}))
                    ppCmds.insert(0, Path.Command('N100', {}))

                    # Raise cutter to safe depth and return index to starting position
                    ppCmds.insert(0, Path.Command('N200', {}))
                    ppCmds.append(Path.Command('G1', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
                    # ppCmds.append(Path.Command('G0', {axisOfRot: 0.0, 'F': self.axialRapid}))
                    if axis != nextAxis:
                        ppCmds.append(Path.Command('G0', {axisOfRot: 0.0, 'F': self.axialRapid}))
                # Save gcode commands to object command list
                self.commandlist.extend(ppCmds)
                sims.append(sim)

            if self.areaOpRetractTool(obj):
                self.endVector = None

        # Raise cutter to safe height and rotate back to original orientation
        if self.rotateFlag is True:
            self.commandlist.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
            self.commandlist.append(Path.Command('G0', {'A': 0.0, 'F': self.axialRapid}))
            self.commandlist.append(Path.Command('G0', {'B': 0.0, 'F': self.axialRapid}))

        PathLog.debug("obj.Name: " + str(obj.Name))
        if obj.EnableRotation != 'Off':
            self.useRotJobClones('Delete')
        self.guiMessage('title', None, show=True)
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

    def opDetermineRotationRadii(self, obj):
        '''opDetermineRotationRadii(self, obj)
            Determine rotational radii for 4th-axis rotations, for clearance/safe heights '''

        parentJob = PathUtils.findParentJob(obj)
        # bb = parentJob.Stock.Shape.BoundBox
        xlim = 0.0
        ylim = 0.0
        zlim = 0.0

        # Determine boundbox radius based upon xzy limits data
        if math.fabs(self.stockBB.ZMin) > math.fabs(self.stockBB.ZMax):
            zlim = self.stockBB.ZMin
        else:
            zlim = self.stockBB.ZMax

        if obj.EnableRotation != 'B(y)':
            # Rotation is around X-axis, cutter moves along same axis
            if math.fabs(self.stockBB.YMin) > math.fabs(self.stockBB.YMax):
                ylim = self.stockBB.YMin
            else:
                ylim = self.stockBB.YMax

        if obj.EnableRotation != 'A(x)':
            # Rotation is around Y-axis, cutter moves along same axis
            if math.fabs(self.stockBB.XMin) > math.fabs(self.stockBB.XMax):
                xlim = self.stockBB.XMin
            else:
                xlim = self.stockBB.XMax

        xRotRad = math.sqrt(ylim**2 + zlim**2)
        yRotRad = math.sqrt(xlim**2 + zlim**2)
        zRotRad = math.sqrt(xlim**2 + ylim**2)

        clrOfst = parentJob.SetupSheet.ClearanceHeightOffset.Value
        safOfst = parentJob.SetupSheet.ClearanceHeightOffset.Value

        return [(xRotRad, yRotRad, zRotRad), (clrOfst, safOfst)]

    def pocketRotationAnalysis(self, obj, norm, surf, prnt):
        '''pocketRotationAnalysis(self, obj, norm, surf, prnt)
            Determine X and Y independent rotation necessary to make normalAt = Z=1 (0,0,1) '''
        PathLog.track()

        praInfo = "pocketRotationAnalysis() in PathAreaOp.py"
        rtn = False
        axis = 'X'
        orientation = 'X'
        angle = 500.0

        def roundRoughValues(val):
            zTol = 1.0E-9
            rndTol = 1.0 - zTol
            # Convert VALxe-15 numbers to zero
            if math.fabs(val) <= zTol:
                return 0.0
            # Convert VAL.99999999 to next integer
            elif math.fabs(val % 1) > rndTol:
                return round(val)
            else:
                return val

        nX = roundRoughValues(norm.x)
        nY = roundRoughValues(norm.y)
        nZ = roundRoughValues(norm.z)
        praInfo += "\n -normalAt(0,0): " + str(nX) + ", " + str(nY) + ", " + str(nZ)

        saX = roundRoughValues(surf.x)
        saY = roundRoughValues(surf.y)
        saZ = roundRoughValues(surf.z)
        praInfo += "\n -Surface.Axis: " + str(saX) + ", " + str(saY) + ", " + str(saZ)

        # Determine rotation needed and current orientation
        if saX == 0.0:
            if saY == 0.0:
                orientation = "Z"
                if saZ == 1.0:
                    angle = 0.0
                elif saZ == -1.0:
                    angle = -180.0
                else:
                    praInfo += "_else_X" + str(saZ)
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
                    praInfo += "_else_X" + str(saX)
            else:
                orientation = "X"
                ratio = saX / saZ
                angle = math.degrees(math.atan(ratio))
                if ratio < 0.0:
                    praInfo += " NEG-ratio"
                    # angle -= 90
                else:
                    praInfo += " POS-ratio"
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

        # Enforce enabled rotation in settings
        if orientation == 'Y':
            axis = 'X'
            if obj.EnableRotation == 'B(y)':  # Axis disabled
                angle = 500.0
        else:
            axis = 'Y'
            if obj.EnableRotation == 'A(x)':  # Axis disabled
                angle = 500.0

        if angle != 500.0 and angle != 0.0:
            praInfo += "\n - ... rotation triggered"
            self.rotateFlag = True
            rtn = True
            if obj.ReverseDirection is True:
                if angle < 180.0:
                    angle = angle + 180.0
                else:
                    angle = angle - 180.0
            praInfo += "\n -Suggested rotation:  angle: " + str(angle) + ",   axis: " + str(axis)
        else:
            praInfo += "\n - ... NO rotation triggered"

        if prnt is True:
            PathLog.info("praInfo: " + str(praInfo))
        return (rtn, angle, axis, praInfo)

    def guiMessage(self, title, msg, show=False):
        if msg is not None:
            self.guiMsgs.append((title, msg))
        if show is True:
            if FreeCAD.GuiUp and len(self.guiMsgs) > 0:
                # self.guiMsgs.pop(0)  # remove formatted place holder.
                from PySide.QtGui import QMessageBox
                # from PySide import QtGui
                for entry in self.guiMsgs:
                    (title, msg) = entry
                    QMessageBox.warning(None, title, msg)
                    # QtGui.QMessageBox.warning(None, title, msg)
                self.guiMsgs = []  # Reset messages
                return True

        # Types: information, warning, critical, question
        if False:
            reply = QtGui.QMessageBox.question(None,"",message,
                QtGui.QMessageBox.Yes | QtGui.QMessageBox.No, QtGui.QMessageBox.No)
            if reply == QtGui.QMessageBox.Yes:
                yes = 'yes'
            if reply == QtGui.QMessageBox.No:
                no = 'no'
        if False:
            msgBox = QtGui.QMessageBox()
            msgBox.setText(translate("Arch","This mesh has more than 1000 facets."))
            msgBox.setInformativeText(translate("Arch","This operation can take a long time. Proceed?"))
            msgBox.setStandardButtons(QtGui.QMessageBox.Ok | QtGui.QMessageBox.Cancel)
            msgBox.setDefaultButton(QtGui.QMessageBox.Cancel)
            ret = msgBox.exec_()
            if ret == QtGui.QMessageBox.Cancel:
                return
        return False

    def visualAxis(self):
        if not FreeCAD.ActiveDocument.getObject('xAxCyl'):
            xAx = 'xAxCyl'
            yAx = 'yAxCyl'
            zAx = 'zAxCyl'
            FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup","visualAxis")
            FreeCADGui.ActiveDocument.getObject('visualAxis').Visibility = False

            FreeCAD.ActiveDocument.addObject("Part::Cylinder", xAx)
            cyl = FreeCAD.ActiveDocument.getObject(xAx)
            cyl.Label = xAx
            cyl.Radius = self.xRotRad
            cyl.Height = 0.01
            cyl.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(0,1,0),90))
            cyl.purgeTouched()
            FreeCAD.ActiveDocument.getObject("visualAxis").addObject(cyl)
            cylGui = FreeCADGui.ActiveDocument.getObject(xAx)
            cylGui.ShapeColor = (0.667,0.000,0.000)
            cylGui.Transparency = 80
            cylGui.Visibility = False

            FreeCAD.ActiveDocument.addObject("Part::Cylinder", yAx)
            cyl = FreeCAD.ActiveDocument.getObject(yAx)
            cyl.Label = yAx
            cyl.Radius = self.yRotRad
            cyl.Height = 0.01
            cyl.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(1,0,0),90))
            FreeCAD.ActiveDocument.getObject("visualAxis").addObject(cyl)
            cyl.purgeTouched()
            cylGui = FreeCADGui.ActiveDocument.getObject(yAx)
            cylGui.ShapeColor = (0.000,0.667,0.000)
            cylGui.Transparency = 80
            cylGui.Visibility = False
            
            if False:
                FreeCAD.ActiveDocument.addObject("Part::Cylinder", zAx)
                cyl = FreeCAD.ActiveDocument.getObject(zAx)
                cyl.Label = zAx
                cyl.Radius = self.yRotRad
                cyl.Height = 0.01
                cyl.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(1,0,0),90))
                FreeCAD.ActiveDocument.getObject("visualAxis").addObject(cyl)
                cyl.purgeTouched()
                cylGui = FreeCADGui.ActiveDocument.getObject(zAx)
                cylGui.ShapeColor = (0.000,0.000,0.498)
                cylGui.Transparency = 80
                cylGui.Visibility = False

    def useRotJobClones(self, cloneName):
        if FreeCAD.ActiveDocument.getObject('rotJobClones'):
            if cloneName == 'Delete':
                if PathLog.getLevel() != 2:
                    for cln in FreeCAD.ActiveDocument.getObject('rotJobClones').Group:
                        FreeCAD.ActiveDocument.removeObject(cln.Name)
                    FreeCAD.ActiveDocument.removeObject('rotJobClones')
                return
            if cloneName == 'Start':
                for cln in FreeCAD.ActiveDocument.getObject('rotJobClones').Group:
                    FreeCAD.ActiveDocument.removeObject(cln.Name)
                FreeCAD.ActiveDocument.removeObject('rotJobClones')
                return
        else:
            FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup","rotJobClones")
            FreeCADGui.ActiveDocument.getObject('rotJobClones').Visibility = False

        if cloneName != 'Start' and cloneName != 'Delete':
            FreeCAD.ActiveDocument.getObject('rotJobClones').addObject(FreeCAD.ActiveDocument.getObject(cloneName))
            FreeCADGui.ActiveDocument.getObject(cloneName).Visibility = False

    def cloneBaseAndStock(self, obj, base, angle, axis, subCount):
        # Create a temporary clone and stock of model for rotational use.
        rndAng = round(angle, 8)
        if rndAng < 0.0:  # neg sign is converted to underscore in clone name creation.
            tag = axis + '_' + axis + '_' + str(math.fabs(rndAng)).replace('.', '_')
        else:
            tag = axis + str(rndAng).replace('.', '_')
        clnNm = obj.Name + '_base_' + '_' + str(subCount) + '_'  + tag
        stckClnNm = obj.Name + '_stock_' + '_' + str(subCount) + '_'  + tag
        if clnNm not in self.cloneNames:
            self.cloneNames.append(clnNm)
            self.cloneNames.append(stckClnNm)
            FreeCAD.ActiveDocument.addObject('Part::Feature', clnNm).Shape = base.Shape
            FreeCAD.ActiveDocument.addObject('Part::Feature', stckClnNm).Shape = PathUtils.findParentJob(obj).Stock.Shape
            FreeCADGui.ActiveDocument.getObject(stckClnNm).Transparency=90
            FreeCADGui.ActiveDocument.getObject(clnNm).ShapeColor = (1.000,0.667,0.000)
            self.useRotJobClones(clnNm)
            self.useRotJobClones(stckClnNm)
        clnBase = FreeCAD.ActiveDocument.getObject(clnNm)                                
        clnStock = FreeCAD.ActiveDocument.getObject(stckClnNm)
        tag = base.Name + '_' + tag
        return (clnBase, clnStock, tag)

    def getFaceNormAndSurf(self, face):
        '''getFaceNormAndSurf(self, face)
            Return face.normalAt(0,0) or face.normal(0,0) and face.Surface.Axis vectors
        '''
        norm = FreeCAD.Vector(0.0, 0.0, 0.0)
        surf = FreeCAD.Vector(0.0, 0.0, 0.0)

        if hasattr(face, 'normalAt'):
            n = face.normalAt(0, 0)
        elif hasattr(face, 'normal'):
            n = face.normal(0, 0)
        if hasattr(face.Surface, 'Axis'):
            s = face.Surface.Axis
        else:
            s = n
        norm.x = n.x
        norm.y = n.y
        norm.z = n.z
        surf.x = s.x
        surf.y = s.y
        surf.z = s.z
        return (norm, surf)

    def applyRotationalAnalysis(self, obj, base, angle, axis, subCount):
        '''applyRotationalAnalysis(self, obj, base, angle, axis, subCount)
            Create temp clone and stock and apply rotation to both.
            Return new rotated clones
        '''
        if axis == 'X':
            vect = FreeCAD.Vector(1, 0, 0)
        elif axis == 'Y':
            vect = FreeCAD.Vector(0, 1, 0)

        if obj.InverseAngle is True:
            angle = -1 * angle

        # Create a temporary clone of model for rotational use.
        (clnBase, clnStock, tag) = self.cloneBaseAndStock(obj, base, angle, axis, subCount)

        # Rotate base to such that Surface.Axis of pocket bottom is Z=1
        clnBase = Draft.rotate(clnBase, angle, center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect, copy=False)
        clnStock = Draft.rotate(clnStock, angle, center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect, copy=False)

        clnBase.purgeTouched()
        clnStock.purgeTouched()
        return (clnBase, angle, clnStock, tag)

    def applyInverseAngle(self, obj, clnBase, clnStock, axis, angle):
        if axis == 'X':
            vect = FreeCAD.Vector(1, 0, 0)
        elif axis == 'Y':
            vect = FreeCAD.Vector(0, 1, 0)
        # Rotate base to inverse of original angle
        clnBase = Draft.rotate(clnBase, (-2 * angle), center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect, copy=False)
        clnStock = Draft.rotate(clnStock, (-2 * angle), center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect, copy=False)
        clnBase.purgeTouched()
        clnStock.purgeTouched()
        if obj.InverseAngle is False:
            obj.InverseAngle = True
        else:
            obj.InverseAngle = False
        angle = -1 * angle
        PathLog.debug("  --Rotated to InverseAngle.")
        return (clnBase, clnStock, angle)

    def calculateStartFinalDepths(self, obj, shape, stock):
        '''calculateStartFinalDepths(self, obj, shape, stock)
            Calculate correct start and final depths for the face
        '''
        finDep = max(obj.FinalDepth.Value, shape.BoundBox.ZMin)
        stockTop = stock.Shape.BoundBox.ZMax
        if obj.EnableRotation == 'Off':
            strDep = obj.StartDepth.Value
            if strDep <= finDep:
                strDep = stockTop
        else:
            strDep = min(obj.StartDepth.Value, stockTop)
            if strDep <= finDep:
                strDep = stockTop  # self.strDep
                msg = "Start depth <= face depth.\nIncreased to stock top."
                # msg = translate('Path', msg + "\nFace depth is {} mm.".format(face.BoundBox.ZMax)
                msg = translate('Path', msg)
                PathLog.error(msg)
        return (strDep, finDep)

    def sortTuplesByIndex(self, TupleList, tagIdx):
        '''sortTuplesByIndex(self, TupleList, tagIdx)
            sort list of tuples based on tag index provided
            return (TagList, GroupList)
        '''
        # Separate elements, regroup by orientation (axis_angle combination)
        TagList = ['X34.2']
        GroupList = [[(2.3, 3.4, 'X')]]
        for tup in TupleList:
            if tup[tagIdx] in TagList:
                # Determine index of found string
                i = 0
                for orn in TagList:
                    if orn == tup[4]:
                        break
                    i += 1
                GroupList[i].append(tup)
            else:
                TagList.append(tup[4])  # add orientation entry
                GroupList.append([tup])  # add orientation entry
        # Remove temp elements
        TagList.pop(0)
        GroupList.pop(0)
        return (TagList, GroupList)



