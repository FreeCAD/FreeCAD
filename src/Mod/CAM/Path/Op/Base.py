# -*- coding: utf-8 -*-
# ***************************************************************************
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
from PathScripts.PathUtils import waiting_effects
from PySide.QtCore import QT_TRANSLATE_NOOP
import Path
import Path.Base.Util as PathUtil
import PathScripts.PathUtils as PathUtils
import math
import time


# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

__title__ = "Base class for all operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Base class and properties implementation for all CAM operations."

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


FeatureTool = 0x0001  # ToolController
FeatureDepths = 0x0002  # FinalDepth, StartDepth
FeatureHeights = 0x0004  # ClearanceHeight, SafeHeight
FeatureStartPoint = 0x0008  # StartPoint
FeatureFinishDepth = 0x0010  # FinishDepth
FeatureStepDown = 0x0020  # StepDown
FeatureNoFinalDepth = 0x0040  # edit or not edit FinalDepth
FeatureBaseVertexes = 0x0100  # Base
FeatureBaseEdges = 0x0200  # Base
FeatureBaseFaces = 0x0400  # Base
FeatureBasePanels = 0x0800  # Base
FeatureLocations = 0x1000  # Locations
FeatureCoolant = 0x2000  # Coolant
FeatureDiameters = 0x4000  # Turning Diameters

FeatureBaseGeometry = FeatureBaseVertexes | FeatureBaseFaces | FeatureBaseEdges


class PathNoTCException(Exception):
    """PathNoTCException is raised when no TC was selected or matches the input
    criteria. This can happen intentionally by the user when they cancel the TC
    selection dialog."""

    def __init__(self):
        super().__init__("No Tool Controller found")


class ObjectOp(object):
    """
    Base class for proxy objects of all Path operations.

    Use this class as a base class for new operations. It provides properties
    and some functionality for the standard properties each operation supports.
    By OR'ing features from the feature list an operation can select which ones
    of the standard features it requires and/or supports.

    The currently supported features are:
        FeatureTool          ... Use of a ToolController
        FeatureDepths        ... Depths, for start, final
        FeatureHeights       ... Heights, safe and clearance
        FeatureStartPoint    ... Supports setting a start point
        FeatureFinishDepth   ... Operation supports a finish depth
        FeatureStepDown      ... Support for step down
        FeatureNoFinalDepth  ... Disable support for final depth modifications
        FeatureBaseVertexes  ... Base geometry support for vertexes
        FeatureBaseEdges     ... Base geometry support for edges
        FeatureBaseFaces     ... Base geometry support for faces
        FeatureLocations     ... Base location support
        FeatureCoolant       ... Support for operation coolant
        FeatureDiameters     ... Support for turning operation diameters

    The base class handles all base API and forwards calls to subclasses with
    an op prefix. For instance, an op is not expected to overwrite onChanged(),
    but implement the function opOnChanged().
    If a base class overwrites a base API function it should call the super's
    implementation - otherwise the base functionality might be broken.
    """

    def addBaseProperty(self, obj):
        obj.addProperty(
            "App::PropertyLinkSubListGlobal",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base geometry for this operation"),
        )

    def addOpValues(self, obj, values):
        if "start" in values:
            obj.addProperty(
                "App::PropertyDistance",
                "OpStartDepth",
                "Op Values",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Holds the calculated value for the StartDepth"
                ),
            )
            obj.setEditorMode("OpStartDepth", 1)  # read-only
        if "final" in values:
            obj.addProperty(
                "App::PropertyDistance",
                "OpFinalDepth",
                "Op Values",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Holds the calculated value for the FinalDepth"
                ),
            )
            obj.setEditorMode("OpFinalDepth", 1)  # read-only
        if "tooldia" in values:
            obj.addProperty(
                "App::PropertyDistance",
                "OpToolDiameter",
                "Op Values",
                QT_TRANSLATE_NOOP("App::Property", "Holds the diameter of the tool"),
            )
            obj.setEditorMode("OpToolDiameter", 1)  # read-only
        if "stockz" in values:
            obj.addProperty(
                "App::PropertyDistance",
                "OpStockZMax",
                "Op Values",
                QT_TRANSLATE_NOOP("App::Property", "Holds the max Z value of Stock"),
            )
            obj.setEditorMode("OpStockZMax", 1)  # read-only
            obj.addProperty(
                "App::PropertyDistance",
                "OpStockZMin",
                "Op Values",
                QT_TRANSLATE_NOOP("App::Property", "Holds the min Z value of Stock"),
            )
            obj.setEditorMode("OpStockZMin", 1)  # read-only

    def __init__(self, obj, name, parentJob=None):
        Path.Log.track()

        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Make False, to prevent operation from generating code"
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "Comment",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "An optional comment for this Operation"
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "UserLabel",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"),
        )
        obj.addProperty(
            "App::PropertyString",
            "CycleTime",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Operations Cycle Time Estimation"),
        )
        obj.setEditorMode("CycleTime", 1)  # read-only

        features = self.opFeatures(obj)

        if FeatureBaseGeometry & features:
            self.addBaseProperty(obj)

        if FeatureLocations & features:
            obj.addProperty(
                "App::PropertyVectorList",
                "Locations",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Base locations for this operation"),
            )

        if FeatureTool & features:
            obj.addProperty(
                "App::PropertyLink",
                "ToolController",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The tool controller that will be used to calculate the path",
                ),
            )
            self.addOpValues(obj, ["tooldia"])

        if FeatureCoolant & features:
            obj.addProperty(
                "App::PropertyEnumeration",
                "CoolantMode",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Coolant mode for this operation"),
            )

        if FeatureDepths & features:
            obj.addProperty(
                "App::PropertyDistance",
                "StartDepth",
                "Depth",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Starting Depth of Tool- first cut depth in Z"
                ),
            )
            obj.addProperty(
                "App::PropertyDistance",
                "FinalDepth",
                "Depth",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Final Depth of Tool- lowest value in Z"
                ),
            )
            if FeatureNoFinalDepth & features:
                obj.setEditorMode("FinalDepth", 2)  # hide
            self.addOpValues(obj, ["start", "final"])
        else:
            # StartDepth has become necessary for expressions on other properties
            obj.addProperty(
                "App::PropertyDistance",
                "StartDepth",
                "Depth",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Starting Depth internal use only for derived values",
                ),
            )
            obj.setEditorMode("StartDepth", 1)  # read-only

        self.addOpValues(obj, ["stockz"])

        if FeatureStepDown & features:
            obj.addProperty(
                "App::PropertyDistance",
                "StepDown",
                "Depth",
                QT_TRANSLATE_NOOP("App::Property", "Incremental Step Down of Tool"),
            )

        if FeatureFinishDepth & features:
            obj.addProperty(
                "App::PropertyDistance",
                "FinishDepth",
                "Depth",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Maximum material removed on final pass."
                ),
            )

        if FeatureHeights & features:
            obj.addProperty(
                "App::PropertyDistance",
                "ClearanceHeight",
                "Depth",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The height needed to clear clamps and obstructions",
                ),
            )
            obj.addProperty(
                "App::PropertyDistance",
                "SafeHeight",
                "Depth",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Rapid Safety Height between locations."
                ),
            )

        if FeatureStartPoint & features:
            obj.addProperty(
                "App::PropertyVectorDistance",
                "StartPoint",
                "Start Point",
                QT_TRANSLATE_NOOP("App::Property", "The start point of this path"),
            )
            obj.addProperty(
                "App::PropertyBool",
                "UseStartPoint",
                "Start Point",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Make True, if specifying a Start Point"
                ),
            )

        if FeatureDiameters & features:
            obj.addProperty(
                "App::PropertyDistance",
                "MinDiameter",
                "Diameter",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Lower limit of the turning diameter"
                ),
            )
            obj.addProperty(
                "App::PropertyDistance",
                "MaxDiameter",
                "Diameter",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Upper limit of the turning diameter."
                ),
            )

        # members being set later
        self.commandlist = None
        self.horizFeed = None
        self.horizRapid = None
        self.job = None
        self.model = None
        self.radius = None
        self.stock = None
        self.tool = None
        self.vertFeed = None
        self.vertRapid = None
        self.addNewProps = None

        self.initOperation(obj)

        for n in self.opPropertyEnumerations():
            Path.Log.debug("n: {}".format(n))
            Path.Log.debug("n[0]: {}  n[1]: {}".format(n[0], n[1]))
            if hasattr(obj, n[0]):
                setattr(obj, n[0], n[1])

        if not hasattr(obj, "DoNotSetDefaultValues") or not obj.DoNotSetDefaultValues:
            if parentJob:
                self.job = PathUtils.addToJob(obj, jobname=parentJob.Name)
            job = self.setDefaultValues(obj)
            if job:
                job.SetupSheet.Proxy.setOperationProperties(obj, name)
                obj.recompute()
                obj.Proxy = self

    @classmethod
    def opPropertyEnumerations(self, dataType="data"):
        """opPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        enums = {
            "CoolantMode": [
                (translate("CAM_Operation", "None"), "None"),
                (translate("CAM_Operation", "Flood"), "Flood"),
                (translate("CAM_Operation", "Mist"), "Mist"),
            ],
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def setEditorModes(self, obj, features):
        """Editor modes are not preserved during document store/restore, set editor modes for all properties"""

        for op in ["OpStartDepth", "OpFinalDepth", "OpToolDiameter", "CycleTime"]:
            if hasattr(obj, op):
                obj.setEditorMode(op, 1)  # read-only

        if FeatureDepths & features:
            if FeatureNoFinalDepth & features:
                obj.setEditorMode("OpFinalDepth", 2)

    def onDocumentRestored(self, obj):
        Path.Log.track()
        features = self.opFeatures(obj)
        if (
            FeatureBaseGeometry & features
            and "App::PropertyLinkSubList" == obj.getTypeIdOfProperty("Base")
        ):
            Path.Log.info("Replacing link property with global link (%s)." % obj.State)
            base = obj.Base
            obj.removeProperty("Base")
            self.addBaseProperty(obj)
            obj.Base = base
            obj.touch()
            obj.Document.recompute()

        if FeatureTool & features and not hasattr(obj, "OpToolDiameter"):
            self.addOpValues(obj, ["tooldia"])

        if FeatureCoolant & features:
            oldvalue = str(obj.CoolantMode) if hasattr(obj, "CoolantMode") else "None"
            if (
                hasattr(obj, "CoolantMode")
                and not obj.getTypeIdOfProperty("CoolantMode")
                == "App::PropertyEnumeration"
            ):
                obj.removeProperty("CoolantMode")

            if not hasattr(obj, "CoolantMode"):
                obj.addProperty(
                    "App::PropertyEnumeration",
                    "CoolantMode",
                    "Path",
                    QT_TRANSLATE_NOOP(
                        "App::Property", "Coolant option for this operation"
                    ),
                )
                for n in self.opPropertyEnumerations():
                    if n[0] == "CoolantMode":
                        setattr(obj, n[0], n[1])
                obj.CoolantMode = oldvalue

        if FeatureDepths & features and not hasattr(obj, "OpStartDepth"):
            self.addOpValues(obj, ["start", "final"])
            if FeatureNoFinalDepth & features:
                obj.setEditorMode("OpFinalDepth", 2)

        if not hasattr(obj, "OpStockZMax"):
            self.addOpValues(obj, ["stockz"])

        if not hasattr(obj, "CycleTime"):
            obj.addProperty(
                "App::PropertyString",
                "CycleTime",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Operations Cycle Time Estimation"),
            )

        self.setEditorModes(obj, features)
        self.opOnDocumentRestored(obj)

    def dumps(self):
        """__getstat__(self) ... called when receiver is saved.
        Can safely be overwritten by subclasses."""
        return None

    def loads(self, state):
        """__getstat__(self) ... called when receiver is restored.
        Can safely be overwritten by subclasses."""
        return None

    def opFeatures(self, obj):
        """opFeatures(obj) ... returns the OR'ed list of features used and supported by the operation.
        The default implementation returns "FeatureTool | FeatureDepths | FeatureHeights | FeatureStartPoint"
        Should be overwritten by subclasses."""
        return (
            FeatureTool
            | FeatureDepths
            | FeatureHeights
            | FeatureStartPoint
            | FeatureBaseGeometry
            | FeatureFinishDepth
            | FeatureCoolant
        )

    def initOperation(self, obj):
        """initOperation(obj) ... implement to create additional properties.
        Should be overwritten by subclasses."""
        pass

    def opOnDocumentRestored(self, obj):
        """opOnDocumentRestored(obj) ... implement if an op needs special handling like migrating the data model.
        Should be overwritten by subclasses."""
        pass

    def opOnChanged(self, obj, prop):
        """opOnChanged(obj, prop) ... overwrite to process property changes.
        This is a callback function that is invoked each time a property of the
        receiver is assigned a value. Note that the FC framework does not
        distinguish between assigning a different value and assigning the same
        value again.
        Can safely be overwritten by subclasses."""
        pass

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj, job) ... overwrite to set initial default values.
        Called after the receiver has been fully created with all properties.
        Can safely be overwritten by subclasses."""
        pass

    def opUpdateDepths(self, obj):
        """opUpdateDepths(obj) ... overwrite to implement special depths calculation.
        Can safely be overwritten by subclass."""
        pass

    def opExecute(self, obj):
        """opExecute(obj) ... called whenever the receiver needs to be recalculated.
        See documentation of execute() for a list of base functionality provided.
        Should be overwritten by subclasses."""
        pass

    def opRejectAddBase(self, obj, base, sub):
        """opRejectAddBase(base, sub) ... if op returns True the addition of the feature is prevented.
        Should be overwritten by subclasses."""
        return False

    def onChanged(self, obj, prop):
        """onChanged(obj, prop) ... base implementation of the FC notification framework.
        Do not overwrite, overwrite opOnChanged() instead."""

        # there's a bit of cycle going on here, if sanitizeBase causes the transaction to
        # be cancelled we end right here again with the unsainitized Base - if that is the
        # case, stop the cycle and return immediately
        if prop == "Base" and self.sanitizeBase(obj):
            return

        if "Restore" not in obj.State and prop in ["Base", "StartDepth", "FinalDepth"]:
            self.updateDepths(obj, True)

        self.opOnChanged(obj, prop)

    def applyExpression(self, obj, prop, expr):
        """applyExpression(obj, prop, expr) ... set expression expr on obj.prop if expr is set"""
        if expr:
            obj.setExpression(prop, expr)
            return True
        return False

    def setDefaultValues(self, obj):
        """setDefaultValues(obj) ... base implementation.
        Do not overwrite, overwrite opSetDefaultValues() instead."""
        if self.job:
            job = self.job
        else:
            job = PathUtils.addToJob(obj)

        obj.Active = True

        features = self.opFeatures(obj)

        if FeatureTool & features:
            if 1 < len(job.Operations.Group):
                obj.ToolController = PathUtil.toolControllerForOp(
                    job.Operations.Group[-2]
                )
            else:
                obj.ToolController = PathUtils.findToolController(obj, self)
            if not obj.ToolController:
                raise PathNoTCException()
            obj.OpToolDiameter = obj.ToolController.Tool.Diameter

        if FeatureCoolant & features:
            Path.Log.track()
            Path.Log.debug(obj.getEnumerationsOfProperty("CoolantMode"))
            obj.CoolantMode = job.SetupSheet.CoolantMode

        if FeatureDepths & features:
            if self.applyExpression(
                obj, "StartDepth", job.SetupSheet.StartDepthExpression
            ):
                obj.OpStartDepth = 1.0
            else:
                obj.StartDepth = 1.0
            if self.applyExpression(
                obj, "FinalDepth", job.SetupSheet.FinalDepthExpression
            ):
                obj.OpFinalDepth = 0.0
            else:
                obj.FinalDepth = 0.0
        else:
            obj.StartDepth = 1.0

        if FeatureStepDown & features:
            if not self.applyExpression(
                obj, "StepDown", job.SetupSheet.StepDownExpression
            ):
                obj.StepDown = "1 mm"

        if FeatureHeights & features:
            if job.SetupSheet.SafeHeightExpression:
                if not self.applyExpression(
                    obj, "SafeHeight", job.SetupSheet.SafeHeightExpression
                ):
                    obj.SafeHeight = "3 mm"
            if job.SetupSheet.ClearanceHeightExpression:
                if not self.applyExpression(
                    obj, "ClearanceHeight", job.SetupSheet.ClearanceHeightExpression
                ):
                    obj.ClearanceHeight = "5 mm"

        if FeatureDiameters & features:
            obj.MinDiameter = "0 mm"
            obj.MaxDiameter = "0 mm"
            if job.Stock:
                obj.MaxDiameter = job.Stock.Shape.BoundBox.XLength

        if FeatureStartPoint & features:
            obj.UseStartPoint = False

        self.opSetDefaultValues(obj, job)
        return job

    def _setBaseAndStock(self, obj, ignoreErrors=False):
        job = PathUtils.findParentJob(obj)

        if not job:
            if not ignoreErrors:
                Path.Log.error(translate("CAM", "No parent job found for operation."))
            return False
        if not job.Model.Group:
            if not ignoreErrors:
                Path.Log.error(
                    translate("CAM", "Parent job %s doesn't have a base object")
                    % job.Label
                )
            return False
        self.job = job
        self.model = job.Model.Group
        self.stock = job.Stock
        return True

    def getJob(self, obj):
        """getJob(obj) ... return the job this operation is part of."""
        if not hasattr(self, "job") or self.job is None:
            if not self._setBaseAndStock(obj):
                return None
        return self.job

    def updateDepths(self, obj, ignoreErrors=False):
        """updateDepths(obj) ... base implementation calculating depths depending on base geometry.
        Should not be overwritten."""

        def faceZmin(bb, fbb):
            if fbb.ZMax == fbb.ZMin and fbb.ZMax == bb.ZMax:  # top face
                return fbb.ZMin
            elif fbb.ZMax > fbb.ZMin and fbb.ZMax == bb.ZMax:  # vertical face, full cut
                return fbb.ZMin
            elif fbb.ZMax > fbb.ZMin and fbb.ZMin > bb.ZMin:  # internal vertical wall
                return fbb.ZMin
            elif fbb.ZMax == fbb.ZMin and fbb.ZMax > bb.ZMin:  # face/shelf
                return fbb.ZMin
            return bb.ZMin

        if not self._setBaseAndStock(obj, ignoreErrors):
            return False

        stockBB = self.stock.Shape.BoundBox
        zmin = stockBB.ZMin
        zmax = stockBB.ZMax

        obj.OpStockZMin = zmin
        obj.OpStockZMax = zmax

        if hasattr(obj, "Base") and obj.Base:
            for base, sublist in obj.Base:
                bb = base.Shape.BoundBox
                zmax = max(zmax, bb.ZMax)
                for sub in sublist:
                    try:
                        if sub:
                            fbb = base.Shape.getElement(sub).BoundBox
                        else:
                            fbb = base.Shape.BoundBox
                        zmin = max(zmin, faceZmin(bb, fbb))
                        zmax = max(zmax, fbb.ZMax)
                    except Part.OCCError as e:
                        Path.Log.error(e)

        else:
            # clearing with stock boundaries
            job = PathUtils.findParentJob(obj)
            zmax = stockBB.ZMax
            zmin = job.Proxy.modelBoundBox(job).ZMax

        if FeatureDepths & self.opFeatures(obj):
            # first set update final depth, it's value is not negotiable
            if not Path.Geom.isRoughly(obj.OpFinalDepth.Value, zmin):
                obj.OpFinalDepth = zmin
            zmin = obj.OpFinalDepth.Value

            def minZmax(z):
                if hasattr(obj, "StepDown") and not Path.Geom.isRoughly(
                    obj.StepDown.Value, 0
                ):
                    return z + obj.StepDown.Value
                else:
                    return z + 1

            # ensure zmax is higher than zmin
            if (zmax - 0.0001) <= zmin:
                zmax = minZmax(zmin)

            # update start depth if requested and required
            if not Path.Geom.isRoughly(obj.OpStartDepth.Value, zmax):
                obj.OpStartDepth = zmax
        else:
            # every obj has a StartDepth
            if obj.StartDepth.Value != zmax:
                obj.StartDepth = zmax

        self.opUpdateDepths(obj)

    def sanitizeBase(self, obj):
        """sanitizeBase(obj) ... check if Base is valid and clear on errors."""
        if hasattr(obj, "Base"):
            try:
                for (o, sublist) in obj.Base:
                    for sub in sublist:
                        o.Shape.getElement(sub)
            except Part.OCCError:
                Path.Log.error(
                    "{} - stale base geometry detected - clearing.".format(obj.Label)
                )
                obj.Base = []
                return True
        return False

    @waiting_effects
    def execute(self, obj):
        """execute(obj) ... base implementation - do not overwrite!
        Verifies that the operation is assigned to a job and that the job also has a valid Base.
        It also sets the following instance variables that can and should be safely be used by
        implementation of opExecute():
            self.model        ... List of base objects of the Job itself
            self.stock        ... Stock object for the Job itself
            self.vertFeed     ... vertical feed rate of assigned tool
            self.vertRapid    ... vertical rapid rate of assigned tool
            self.horizFeed    ... horizontal feed rate of assigned tool
            self.horizRapid   ... norizontal rapid rate of assigned tool
            self.tool         ... the actual tool being used
            self.radius       ... the main radius of the tool being used
            self.commandlist  ... a list for collecting all commands produced by the operation

        Once everything is validated and above variables are set the implementation calls
        opExecute(obj) - which is expected to add the generated commands to self.commandlist
        Finally the base implementation adds a rapid move to clearance height and assigns
        the receiver's Path property from the command list.
        """
        Path.Log.track()

        if not obj.Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            return

        if not self._setBaseAndStock(obj):
            return

        # make sure Base is still valid or clear it
        self.sanitizeBase(obj)

        if FeatureTool & self.opFeatures(obj):
            tc = obj.ToolController
            if tc is None or tc.ToolNumber == 0:
                Path.Log.error(
                    translate(
                        "CAM",
                        "No Tool Controller is selected. We need a tool to build a Path.",
                    )
                )
                return
            else:
                self.vertFeed = tc.VertFeed.Value
                self.horizFeed = tc.HorizFeed.Value
                self.vertRapid = tc.VertRapid.Value
                self.horizRapid = tc.HorizRapid.Value
                tool = tc.Proxy.getTool(tc)
                if not tool or float(tool.Diameter) == 0:
                    Path.Log.error(
                        translate(
                            "CAM",
                            "No Tool found or diameter is zero. We need a tool to build a Path.",
                        )
                    )
                    return
                self.radius = float(tool.Diameter) / 2.0
                self.tool = tool
                obj.OpToolDiameter = tool.Diameter

        self.updateDepths(obj)
        # now that all op values are set make sure the user properties get updated accordingly,
        # in case they still have an expression referencing any op values
        obj.recompute()

        self.commandlist = []
        self.commandlist.append(Path.Command("(%s)" % obj.Label))
        if obj.Comment:
            self.commandlist.append(Path.Command("(%s)" % obj.Comment))

        result = self.opExecute(obj)

        if self.commandlist and (FeatureHeights & self.opFeatures(obj)):
            # Let's finish by rapid to clearance...just for safety
            self.commandlist.append(
                Path.Command("G0", {"Z": obj.ClearanceHeight.Value})
            )

        path = Path.Path(self.commandlist)
        obj.Path = path
        obj.CycleTime = self.getCycleTimeEstimate(obj)
        self.job.Proxy.getCycleTime()
        return result

    def getCycleTimeEstimate(self, obj):

        tc = obj.ToolController

        if tc is None or tc.ToolNumber == 0:
            Path.Log.error(translate("CAM", "No Tool Controller selected."))
            return translate("CAM", "Tool Error")

        hFeedrate = tc.HorizFeed.Value
        vFeedrate = tc.VertFeed.Value
        hRapidrate = tc.HorizRapid.Value
        vRapidrate = tc.VertRapid.Value

        if (
            hFeedrate == 0 or vFeedrate == 0
        ) and not Path.Preferences.suppressAllSpeedsWarning():
            Path.Log.warning(
                translate(
                    "CAM",
                    "Tool Controller feedrates required to calculate the cycle time.",
                )
            )
            return translate("CAM", "Feedrate Error")

        if (
            hRapidrate == 0 or vRapidrate == 0
        ) and not Path.Preferences.suppressRapidSpeedsWarning():
            Path.Log.warning(
                translate(
                    "CAM",
                    "Add Tool Controller Rapid Speeds on the SetupSheet for more accurate cycle times.",
                )
            )

        # Get the cycle time in seconds
        seconds = obj.Path.getCycleTime(hFeedrate, vFeedrate, hRapidrate, vRapidrate)

        if not seconds or math.isnan(seconds):
            return translate("CAM", "Cycletime Error")

        # Convert the cycle time to a HH:MM:SS format
        cycleTime = time.strftime("%H:%M:%S", time.gmtime(seconds))

        return cycleTime

    def addBase(self, obj, base, sub):
        Path.Log.track(obj, base, sub)
        base = PathUtil.getPublicObject(base)

        if self._setBaseAndStock(obj):
            for model in self.job.Model.Group:
                if base == self.job.Proxy.baseObject(self.job, model):
                    base = model
                    break

            baselist = obj.Base
            if baselist is None:
                baselist = []

            for p, el in baselist:
                if p == base and sub in el:
                    Path.Log.notice(
                        (
                            translate("CAM", "Base object %s.%s already in the list")
                            + "\n"
                        )
                        % (base.Label, sub)
                    )
                    return

            if not self.opRejectAddBase(obj, base, sub):
                baselist.append((base, sub))
                obj.Base = baselist
            else:
                Path.Log.notice(
                    (
                        translate("CAM", "Base object %s.%s rejected by operation")
                        + "\n"
                    )
                    % (base.Label, sub)
                )

    def isToolSupported(self, obj, tool):
        """toolSupported(obj, tool) ... Returns true if the op supports the given tool.
        This function can safely be overwritten by subclasses."""

        return True
