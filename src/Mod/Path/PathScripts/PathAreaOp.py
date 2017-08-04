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
import PathScripts.PathUtils as PathUtils
import Path

from PathScripts.PathUtils import depth_params
from PathScripts.PathUtils import makeWorkplane
from PathScripts.PathUtils import waiting_effects
from PySide import QtCore

__title__ = "Base class for PathArea based operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule()

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

FeatureTool         = 0x01
FeatureDepths       = 0x02
FeatureHeights      = 0x04
FeatureStartPoint   = 0x08
FeatureBaseFaces    = 0x10
FeatureBaseEdges    = 0x20
FeatureFinishDepth  = 0x40

FeatureBaseGeometry = FeatureBaseFaces | FeatureBaseEdges

class ObjectOp(object):

    def __init__(self, obj):
        PathLog.track()

        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "An optional comment for this Contour"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"))

        if FeatureBaseGeometry & self.opFeatures(obj):
            obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base geometry for this operation"))

        if FeatureTool & self.opFeatures(obj):
            obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))

        if FeatureDepths & self.opFeatures(obj):
            obj.addProperty("App::PropertyDistance", "StepDown", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Incremental Step Down of Tool"))
            obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Starting Depth of Tool- first cut depth in Z"))
            obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Final Depth of Tool- lowest value in Z"))

        if FeatureFinishDepth & self.opFeatures(obj):
            obj.addProperty("App::PropertyDistance", "FinishDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Maximum material removed on final pass."))

        if FeatureHeights & self.opFeatures(obj):
            obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height needed to clear clamps and obstructions"))
            obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Rapid Safety Height between locations."))

        if FeatureStartPoint & self.opFeatures(obj):
            obj.addProperty("App::PropertyVector", "StartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "The start point of this path"))
            obj.addProperty("App::PropertyBool", "UseStartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if specifying a Start Point"))

        # Debugging
        obj.addProperty("App::PropertyString", "AreaParams", "Path")
        obj.setEditorMode('AreaParams', 2)  # hide
        obj.addProperty("App::PropertyString", "PathParams", "Path")
        obj.setEditorMode('PathParams', 2)  # hide
        obj.addProperty("Part::PropertyPartShape", "removalshape", "Path")
        obj.setEditorMode('removalshape', 2)  # hide

        self.initOperation(obj)
        obj.Proxy = self
        self.setDefaultValues(obj)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def opFeatures(self, obj):
        return FeatureTool | FeatureDepths | FeatureHeights | FeatureStartPoint | FeatureBaseGeometry | FeatureFinishDepth
    def opOnChanged(self, obj, prop):
        pass
    def opSetDefaultValues(self, obj):
        pass
     
    def onChanged(self, obj, prop):
        if prop in ['AreaParams', 'PathParams', 'removalshape']:
            obj.setEditorMode(prop, 2)
        self.opOnChanged(obj, prop)

    def setDefaultValues(self, obj):
        PathUtils.addToJob(obj)

        obj.Active = True

        if FeatureTool & self.opFeatures(obj):
            obj.ToolController = PathUtils.findToolController(obj)

        if FeatureDepths & self.opFeatures(obj):
            try:
                shape = self.opShapeForDepths(obj)
            except:
                shape = None

            if shape:
                bb = shape.BoundBox
                obj.StartDepth      = bb.ZMax
                obj.FinalDepth      = bb.ZMin
                obj.StepDown        = 1.0
            else:
                obj.StartDepth      =  1.0
                obj.FinalDepth      =  0.0
                obj.StepDown        =  1.0

        if FeatureHeights & self.opFeatures(obj):
            try:
                shape = self.opShapeForDepths(obj)
            except:
                shape = None

            if shape:
                bb = shape.BoundBox
                obj.ClearanceHeight = bb.ZMax + 5.0
                obj.SafeHeight      = bb.ZMax + 3.0
            else:
                obj.ClearanceHeight = 10.0
                obj.SafeHeight      =  8.0

        if FeatureStartPoint & self.opFeatures(obj):
            obj.UseStartPoint = False

        self.opSetDefaultValues(obj)

    @waiting_effects
    def _buildPathArea(self, obj, baseobject, start=None, getsim=False):
        PathLog.track()
        area = Path.Area()
        area.setPlane(makeWorkplane(baseobject))
        area.add(baseobject)

        areaParams = self.opAreaParams(obj)

        heights = [i for i in self.depthparams]
        PathLog.debug('depths: {}'.format(heights))
        area.setParams(**areaParams)
        obj.AreaParams = str(area.getParams())

        PathLog.debug("Area with params: {}".format(area.getParams()))

        sections = area.makeSections(mode=0, project=True, heights=heights)
        shapelist = [sec.getShape() for sec in sections]

        pathParams = self.opPathParams(obj)
        pathParams['shapes'] = shapelist
        pathParams['feedrate'] = self.horizFeed
        pathParams['feedrate_v'] = self.vertFeed
        pathParams['verbose'] = True
        pathParams['resume_height'] = obj.StepDown.Value
        pathParams['retraction'] = obj.ClearanceHeight.Value
        pathParams['return_end'] = True

        if self.endVector is not None:
            pathParams['start'] = self.endVector
        elif obj.UseStartPoint:
            pathParams['start'] = obj.StartPoint

        obj.PathParams = str({key: value for key, value in pathParams.items() if key != 'shapes'})

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

    def execute(self, obj, getsim=False):
        PathLog.track()
        self.endVector = None

        if not obj.Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            if obj.ViewObject:
                obj.ViewObject.Visibility = False
            return

        self.depthparams = depth_params(
                clearance_height=obj.ClearanceHeight.Value,
                safe_height=obj.SafeHeight.Value,
                start_depth=obj.StartDepth.Value,
                step_down=obj.StepDown.Value,
                z_finish_step=0.0,
                final_depth=obj.FinalDepth.Value,
                user_depths=None)

        toolLoad = obj.ToolController
        if toolLoad is None or toolLoad.ToolNumber == 0:

            FreeCAD.Console.PrintError("No Tool Controller is selected. We need a tool to build a Path.")
            return
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            self.vertRapid = toolLoad.VertRapid.Value
            self.horizRapid = toolLoad.HorizRapid.Value
            tool = toolLoad.Proxy.getTool(toolLoad)
            if not tool or tool.Diameter == 0:
                FreeCAD.Console.PrintError("No Tool found or diameter is zero. We need a tool to build a Path.")
                return
            else:
                self.radius = tool.Diameter/2

        commandlist = []
        commandlist.append(Path.Command("(" + obj.Label + ")"))

        shape = self.opShape(obj, commandlist)

        if FeatureStartPoint and obj.UseStartPoint:
            start = obj.StartPoint
        else:
            start = FreeCAD.Vector()

        try:
            (pp, sim) = self._buildPathArea(obj, shape, start, getsim)
            commandlist.extend(pp.Commands)
        except Exception as e:
            FreeCAD.Console.PrintError(e)
            FreeCAD.Console.PrintError("Something unexpected happened. Check project and tool config.")
            sim = None


        # Let's finish by rapid to clearance...just for safety
        commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        PathLog.track()
        path = Path.Path(commandlist)
        obj.Path = path
        return sim

    def addBase(self, obj, base, sub=""):
        PathLog.track()
        baselist = obj.Base
        if baselist is None:
            baselist = []
        item = (base, sub)
        if item in baselist:
            PathLog.warning(translate("Path", "this object already in the list" + "\n"))
        else:
            baselist.append(item)
            obj.Base = baselist
