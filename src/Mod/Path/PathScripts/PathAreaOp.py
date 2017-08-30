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
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class and properties for Path.Area based operations."

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
    PathParams and removalshape, all hidding.
    The main reason for existance is to implement the standard interface
    to Path.Area so subclasses only have to provide the shapes for the
    operations.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... returns the base features supported by all Path.Area based operations.
        The standard feature list is OR'ed with the return value of areaOpFeatures().
        Do not overwrite, implement areaOpFeatures(obj) instead.'''
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureStepDown | PathOp.FeatureHeights | PathOp.FeatureStartPoint | self.areaOpFeatures(obj)

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

    def areaOpShapeForDepths(self, obj):
        '''areaOpShapeForDepths(obj) ... returns the shape used to make an initial calculation for the depths being used.
        The default implementation retuns the job's Base.Shape'''
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
        '''opOnChanged(obj, prop) ... base implemenation of the notification framework - do not overwrite.
        The base implementation takes a stab at determining Heights and Depths if the operations's Base
        changes.
        Do not overwrite, overwrite areaOpOnChanged(obj, prop) instead.'''
        #PathLog.track(obj.Label, prop)
        if prop in ['AreaParams', 'PathParams', 'removalshape']:
            obj.setEditorMode(prop, 2)

        if PathOp.FeatureBaseGeometry & self.opFeatures(obj):
            if prop == 'Base' and len(obj.Base) == 1:
                PathLog.info("opOnChanged(%s, %s)" % (obj.Label, prop))
                try:
                    (base, sub) = obj.Base[0]
                    bb = base.Shape.BoundBox  # parent boundbox
                    subobj = base.Shape.getElement(sub[0])
                    fbb = subobj.BoundBox  # feature boundbox
                    obj.StartDepth = bb.ZMax
                    obj.ClearanceHeight = bb.ZMax + 5.0
                    obj.SafeHeight = bb.ZMax + 3.0

                    if fbb.ZMax == fbb.ZMin and fbb.ZMax == bb.ZMax:  # top face
                        obj.FinalDepth = bb.ZMin
                    elif fbb.ZMax > fbb.ZMin and fbb.ZMax == bb.ZMax:  # vertical face, full cut
                        obj.FinalDepth = fbb.ZMin
                    elif fbb.ZMax > fbb.ZMin and fbb.ZMin > bb.ZMin:  # internal vertical wall
                        obj.FinalDepth = fbb.ZMin
                    elif fbb.ZMax == fbb.ZMin and fbb.ZMax > bb.ZMin:  # face/shelf
                        obj.FinalDepth = fbb.ZMin
                    else:  # catch all
                        obj.FinalDepth = bb.ZMin

                    if hasattr(obj, 'Side'):
                        if bb.XLength == fbb.XLength and bb.YLength == fbb.YLength:
                            obj.Side = "Outside"
                        else:
                            obj.Side = "Inside"

                except Exception as e:
                    PathLog.error(translate("PatArea", "Error in calculating depths: %s" % e))
                    obj.StartDepth = 5.0
                    obj.ClearanceHeight = 10.0
                    obj.SafeHeight = 8.0
                    if hasattr(obj, 'Side'):
                        obj.Side = "Outside"

        self.areaOpOnChanged(obj, prop)

    def opSetDefaultValues(self, obj):
        '''opSetDefaultValues(obj) ... base implementation, do not overwrite.
        The base implementation sets the depths and heights based on the
        areaOpShapeForDepths() return value.
        Do not overwrite, overwrite areaOpSetDefaultValues(obj) instead.'''
        PathLog.info("opSetDefaultValues(%s)" % (obj.Label))
        if PathOp.FeatureDepths & self.opFeatures(obj):
            try:
                shape = self.areaOpShapeForDepths(obj)
            except:
                shape = None

            if shape:
                bb = shape.BoundBox
                obj.StartDepth      = bb.ZMax
                obj.FinalDepth      = bb.ZMin
                if PathOp.FeatureStepDown & self.opFeatures(obj):
                    obj.StepDown        = 1.0
            else:
                obj.StartDepth      =  1.0
                obj.FinalDepth      =  0.0
                if PathOp.FeatureStepDown & self.opFeatures(obj):
                    obj.StepDown        =  1.0

        if PathOp.FeatureHeights & self.opFeatures(obj):
            try:
                shape = self.areaOpShapeForDepths(obj)
            except:
                shape = None

            if shape:
                bb = shape.BoundBox
                obj.ClearanceHeight = bb.ZMax + 5.0
                obj.SafeHeight      = bb.ZMax + 3.0
            else:
                obj.ClearanceHeight = 10.0
                obj.SafeHeight      =  8.0

        self.areaOpSetDefaultValues(obj)

    def areaOpSetDefaultValues(self, obj):
        '''areaOpSetDefaultValues(obj) ... overwrite to set initial values of operation specific properties.
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
        pathParams['resume_height'] = obj.StepDown.Value
        pathParams['retraction'] = obj.ClearanceHeight.Value
        pathParams['return_end'] = True
        # Note that emmitting preambles between moves breaks some dressups and prevents path optimization on some controllers
        pathParams['preamble'] = False

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

        self.depthparams = PathUtils.depth_params(
                clearance_height=obj.ClearanceHeight.Value,
                safe_height=obj.SafeHeight.Value,
                start_depth=obj.StartDepth.Value,
                step_down=obj.StepDown.Value,
                z_finish_step=0.0,
                final_depth=obj.FinalDepth.Value,
                user_depths=None)

        if PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
            start = obj.StartPoint
        else:
            start = None

        shapes = self.areaOpShapes(obj)

        sims = []
        for (shape, isHole) in shapes:
            try:
                (pp, sim) = self._buildPathArea(obj, shape, isHole, start, getsim)
                self.commandlist.extend(pp.Commands)
                sims.append(sim)
            except Exception as e:
                FreeCAD.Console.PrintError(e)
                FreeCAD.Console.PrintError("Something unexpected happened. Check project and tool config.")

            if self.areaOpRetractTool(obj):
                self.endVector = None

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
