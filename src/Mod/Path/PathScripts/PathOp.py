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

from PathScripts.PathUtils import waiting_effects
from PySide import QtCore

__title__ = "Base class for all operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule()

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

FeatureTool         = 0x0001
FeatureDepths       = 0x0002
FeatureHeights      = 0x0004
FeatureStartPoint   = 0x0008
FeatureFinishDepth  = 0x0010
FeatureStepDown     = 0x0020
FeatureBaseVertexes = 0x1000
FeatureBaseEdges    = 0x2000
FeatureBaseFaces    = 0x4000
FeatureBasePanels   = 0x8000

FeatureBaseGeometry = FeatureBaseVertexes | FeatureBaseFaces | FeatureBaseEdges | FeatureBasePanels

class ObjectOp(object):

    def __init__(self, obj):
        PathLog.track()

        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "An optional comment for this Operation"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"))

        if FeatureBaseGeometry & self.opFeatures(obj):
            obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base geometry for this operation"))

        if FeatureTool & self.opFeatures(obj):
            obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))

        if FeatureDepths & self.opFeatures(obj):
            obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Starting Depth of Tool- first cut depth in Z"))
            obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Final Depth of Tool- lowest value in Z"))

        if FeatureStepDown & self.opFeatures(obj):
            obj.addProperty("App::PropertyDistance", "StepDown", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Incremental Step Down of Tool"))

        if FeatureFinishDepth & self.opFeatures(obj):
            obj.addProperty("App::PropertyDistance", "FinishDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Maximum material removed on final pass."))

        if FeatureHeights & self.opFeatures(obj):
            obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height needed to clear clamps and obstructions"))
            obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Rapid Safety Height between locations."))

        if FeatureStartPoint & self.opFeatures(obj):
            obj.addProperty("App::PropertyVector", "StartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "The start point of this path"))
            obj.addProperty("App::PropertyBool", "UseStartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if specifying a Start Point"))

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
        self.opOnChanged(obj, prop)

    def setDefaultValues(self, obj, callOp = True):
        PathUtils.addToJob(obj)

        obj.Active = True

        if FeatureTool & self.opFeatures(obj):
            obj.ToolController = PathUtils.findToolController(obj)

        if FeatureDepths & self.opFeatures(obj):
            obj.StartDepth      =  1.0
            obj.FinalDepth      =  0.0

        if FeatureStepDown & self.opFeatures(obj):
            obj.StepDown        =  1.0

        if FeatureHeights & self.opFeatures(obj):
            obj.ClearanceHeight = 10.0
            obj.SafeHeight      =  8.0

        if FeatureStartPoint & self.opFeatures(obj):
            obj.UseStartPoint = False

        self.opSetDefaultValues(obj)

    @waiting_effects
    def execute(self, obj):
        PathLog.track()

        if not obj.Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            if obj.ViewObject:
                obj.ViewObject.Visibility = False
            return

        if FeatureTool & self.opFeatures(obj):
            tc = obj.ToolController
            if tc is None or tc.ToolNumber == 0:
                FreeCAD.Console.PrintError("No Tool Controller is selected. We need a tool to build a Path.")
                return
            else:
                self.vertFeed = tc.VertFeed.Value
                self.horizFeed = tc.HorizFeed.Value
                self.vertRapid = tc.VertRapid.Value
                self.horizRapid = tc.HorizRapid.Value
                tool = tc.Proxy.getTool(tc)
                if not tool or tool.Diameter == 0:
                    FreeCAD.Console.PrintError("No Tool found or diameter is zero. We need a tool to build a Path.")
                    return
                else:
                    self.radius = tool.Diameter/2

        self.commandlist = []
        self.commandlist.append(Path.Command("(%s)" % obj.Label))
        if obj.Comment:
            self.commandlist.append(Path.Command("(%s)" % obj.Comment))

        result = self.opExecute(obj)

        # Let's finish by rapid to clearance...just for safety
        self.commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        path = Path.Path(self.commandlist)
        obj.Path = path
        return result

    def addBase(self, obj, base, sub):
        PathLog.track()
        baselist = obj.Base
        if baselist is None:
            baselist = []
        item = (base, sub)
        if item in baselist:
            PathLog.notice(translate("Path", "this object already in the list" + "\n"))
        else:
            baselist.append(item)
            obj.Base = baselist

