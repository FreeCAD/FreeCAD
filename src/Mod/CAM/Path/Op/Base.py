# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Path.Geom
import PathScripts.PathUtils as PathUtils
from Path.Op.Util import getCycleTimeEstimate

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
FeatureLinking = 0x0080  # Linking
FeatureBaseVertexes = 0x0100  # Base
FeatureBaseEdges = 0x0200  # Base
FeatureBaseFaces = 0x0400  # Base
FeatureBaseModels = 0x0800  # Base
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


class _TransformedShapeProxy:
    """Lightweight proxy that wraps a FreeCAD document object and intercepts
    ``.Shape`` access to return a pre-transformed copy.

    Every other attribute (``Name``, ``Label``, ``isDerivedFrom``, ``Placement``,
    etc.) is delegated transparently to the wrapped object.

    This is used by the 3+2 positioning logic so that child operations see
    Z-up geometry without the base class ever writing to a FreeCAD property
    (which would trigger cascading recomputes).
    """

    __slots__ = ("_real_obj", "_transformed_shape")

    def __init__(self, real_obj, transformed_shape):
        object.__setattr__(self, "_real_obj", real_obj)
        object.__setattr__(self, "_transformed_shape", transformed_shape)

    @property
    def Shape(self):
        return object.__getattribute__(self, "_transformed_shape")

    def __getattr__(self, name):
        return getattr(object.__getattribute__(self, "_real_obj"), name)

    def __eq__(self, other):
        real = object.__getattribute__(self, "_real_obj")
        if isinstance(other, _TransformedShapeProxy):
            return real == object.__getattribute__(other, "_real_obj")
        return real == other

    def __hash__(self):
        return hash(object.__getattribute__(self, "_real_obj"))


def _transform_shape_with_arc_fix(shape, matrix):
    """Transform *shape* by *matrix* and recover arcs degraded to BSplines.

    ``transformShape()`` can turn circles/arcs into BSplineCurves.
    This attempts to convert them back via ``toBiArcs()`` so that
    downstream operations (e.g. Deburr) still see native arc geometry.

    Returns the (possibly fixed) transformed ``Part.Shape``.
    """
    transformed = shape.copy().transformShape(matrix, False, False)
    fixed_edges = []
    any_converted = False
    for edge in transformed.Edges:
        try:
            curve = edge.Curve
        except TypeError:
            fixed_edges.append(edge)
            continue
        if type(curve).__name__ == "BSplineCurve":
            try:
                arcs = curve.toBiArcs(0.001)
                if arcs and len(arcs) == 1:
                    fixed_edges.append(Part.Edge(arcs[0]))
                    any_converted = True
                    continue
            except Exception:
                # Biarc conversion can fail for degenerate or unsupported
                # B-spline geometry; fall back to keeping the original edge.
                pass
        fixed_edges.append(edge)

    if any_converted:
        return Part.makeCompound(fixed_edges)
    return transformed


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
        FeatureBaseModels    ... Base geometry support for whole shape
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
                QT_TRANSLATE_NOOP("App::Property", "Holds the calculated value for the StartDepth"),
            )
            obj.setEditorMode("OpStartDepth", 1)  # read-only
        if "final" in values:
            obj.addProperty(
                "App::PropertyDistance",
                "OpFinalDepth",
                "Op Values",
                QT_TRANSLATE_NOOP("App::Property", "Holds the calculated value for the FinalDepth"),
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

    def addLinking(self, obj):
        obj.addProperty(
            "App::PropertyEnumeration",
            "CollisionAvoidanceStrategy",
            "Linking",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Method collision detection to create optimal path between areas"
                "\n\nClearance Height: no collision detection, uses clearance height for rapid moves between areas"
                "\nRetract Height: no collision detection, uses safe height for rapid moves between areas"
                "\nLine of Sight: fastest - checks the path centerline"
                "\nTool Diameter: balanced - checks clearance using the tool diameter"
                "\nTool Shape: safest - checks clearance using the cross section of the tool shape",
            ),
        )
        obj.CollisionAvoidanceStrategy = [
            "Clearance Height",
            "Retract Height",
            "Line of Sight",
            "Tool Diameter",
            "Tool Shape",
        ]
        obj.addProperty(
            "App::PropertyLength",
            "CollisionClearance",
            "Linking",
            QT_TRANSLATE_NOOP("App::Property", "Distance for collision detection"),
        )

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
            "App::PropertyBool",
            "BlockDelete",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Enable post processor to add block delete commands"
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "Comment",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "An optional comment for this Operation"),
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

        obj.addProperty(
            "App::PropertyVector",
            "Workplane",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The orientation of the tool for this operation. Default is (0, 0, 1) for standard Z-up milling.",
            ),
        )
        obj.Workplane = FreeCAD.Vector(0, 0, 1)

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
                QT_TRANSLATE_NOOP("App::Property", "Starting Depth of Tool- first cut depth in Z"),
            )
            obj.addProperty(
                "App::PropertyDistance",
                "FinalDepth",
                "Depth",
                QT_TRANSLATE_NOOP("App::Property", "Final Depth of Tool- lowest value in Z"),
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
                QT_TRANSLATE_NOOP("App::Property", "Maximum material removed on final pass."),
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
                QT_TRANSLATE_NOOP("App::Property", "Rapid Safety Height between locations."),
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
                QT_TRANSLATE_NOOP("App::Property", "Make True, if specifying a Start Point"),
            )

        if FeatureDiameters & features:
            obj.addProperty(
                "App::PropertyDistance",
                "MinDiameter",
                "Diameter",
                QT_TRANSLATE_NOOP("App::Property", "Lower limit of the turning diameter"),
            )
            obj.addProperty(
                "App::PropertyDistance",
                "MaxDiameter",
                "Diameter",
                QT_TRANSLATE_NOOP("App::Property", "Upper limit of the turning diameter."),
            )

        if FeatureLinking & features:
            self.addLinking(obj)

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
        self.isBaseValid = True

        self.initOperation(obj)

        for n in self.opPropertyEnumerations():
            Path.Log.debug("n: {}".format(n))
            Path.Log.debug("n[0]: {}  n[1]: {}".format(n[0], n[1]))
            if hasattr(obj, n[0]):
                setattr(obj, n[0], n[1])

        if not hasattr(obj, "DoNotSetDefaultValues") or not obj.DoNotSetDefaultValues:
            if parentJob:
                self.job = parentJob
                self.model = parentJob.Model.Group if parentJob.Model else []
                self.stock = parentJob.Stock if hasattr(parentJob, "Stock") else None
                PathUtils.addToJob(obj, jobname=parentJob.Name)
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
        self.checkBase(obj)
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
                and not obj.getTypeIdOfProperty("CoolantMode") == "App::PropertyEnumeration"
            ):
                obj.removeProperty("CoolantMode")

            if not hasattr(obj, "CoolantMode"):
                obj.addProperty(
                    "App::PropertyEnumeration",
                    "CoolantMode",
                    "Path",
                    QT_TRANSLATE_NOOP("App::Property", "Coolant option for this operation"),
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
        if not hasattr(obj, "BlockDelete"):
            obj.addProperty(
                "App::PropertyBool",
                "BlockDelete",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Enable post processor to add block delete commands"
                ),
            )

        if FeatureStepDown & features and not hasattr(obj, "StepDown"):
            obj.addProperty(
                "App::PropertyDistance",
                "StepDown",
                "Depth",
                QT_TRANSLATE_NOOP("App::Property", "Incremental Step Down of Tool"),
            )
            obj.StepDown = 0

        if FeatureLinking & features and not hasattr(obj, "CollisionAvoidanceStrategy"):
            self.addLinking(obj)
            for n in self.opPropertyEnumerations():
                if hasattr(obj, n[0]):
                    setattr(obj, n[0], n[1])
            obj.CollisionAvoidanceStrategy = "Clearance Height"
            self.applyExpression(obj, "CollisionClearance", "OpToolDiameter")

        if not hasattr(obj, "Workplane"):
            obj.addProperty(
                "App::PropertyVector",
                "Workplane",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The orientation of the tool for this operation. Default is (0, 0, 1) for standard Z-up milling.",
                ),
            )
            obj.Workplane = FreeCAD.Vector(0, 0, 1)

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

    def initAfterBase(self, obj):
        """initAfterBase(obj) ... implement to execute extra commands
        while create new operation after add all base geometry.
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

    def baseShapes(self, obj):
        """baseShapes(obj) ... yield (base, subs) tuples for the operation's
        base geometry.

        When a 3+2 geometry rotation is active (``self._geom_transform_matrix``
        is set), each *base* object is wrapped in a
        :class:`_TransformedShapeProxy` whose ``.Shape`` returns the
        pre-transformed (Z-up) copy.  Sub-element names are unchanged —
        ``proxy.Shape.getElement("Face3")`` returns the transformed Face3.

        When no rotation is active this is equivalent to iterating
        ``obj.Base`` directly.

        Operations that want 3+2 support should use::

            for base, subs in self.baseShapes(obj):
                shape = base.Shape.getElement(sub)
                ...

        instead of ``for base, subs in obj.Base:``.
        """
        matrix = getattr(self, "_geom_transform_matrix", None)
        Path.Log.debug(f"baseShapes called, transform active: {matrix is not None}")
        if matrix is None:
            Path.Log.debug(f"  No transform, yielding {len(obj.Base)} base items directly")
            yield from obj.Base
            return

        # Cache proxies so the same base object is wrapped only once
        proxy_cache = {}
        for base_obj, subs in obj.Base:
            key = id(base_obj)
            if key not in proxy_cache:
                if hasattr(base_obj, "Shape") and base_obj.Shape:
                    shape = _transform_shape_with_arc_fix(base_obj.Shape, matrix)

                    # Validate the shape before creating proxy
                    Path.Log.debug(f"  Final shape type: {type(shape).__name__}")
                    if hasattr(shape, "ShapeType"):
                        Path.Log.debug(f"  Shape type: {shape.ShapeType}")
                    if hasattr(shape, "isNull") and shape.isNull():
                        Path.Log.warning("  Transformed shape is null, using original")
                        shape = base_obj.Shape
                    elif hasattr(shape, "Volume") and shape.Volume < 1e-9:
                        Path.Log.debug(f"  Transformed shape has very small volume: {shape.Volume}")

                    # Check if we have faces
                    if hasattr(shape, "Faces"):
                        Path.Log.debug(f"  Shape has {len(shape.Faces)} faces")
                        if len(shape.Faces) == 0:
                            Path.Log.warning("  Transformed shape has no faces!")

                    proxy_cache[key] = _TransformedShapeProxy(base_obj, shape)
                else:
                    proxy_cache[key] = base_obj
            yield proxy_cache[key], subs

    def opRejectAddBase(self, obj, base, sub):
        """opRejectAddBase(base, sub) ... if op returns True the addition of the feature is prevented.
        Should be overwritten by subclasses."""
        return False

    def onChanged(self, obj, prop):
        """onChanged(obj, prop) ... base implementation of the FC notification framework.
        Do not overwrite, overwrite opOnChanged() instead."""
        if prop == "Base" and not self.checkBase(obj):
            return

        if not getattr(self, "isBaseValid", True):
            return

        if "Restore" not in obj.State and prop in ("Base", "StartDepth", "FinalDepth"):
            self.updateDepths(obj, True)

        self.opOnChanged(obj, prop)

        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

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
        if not job:
            raise ValueError(
                "No job associated with the operation. Please ensure the operation is part of a job."
            )
        obj.Active = True

        features = self.opFeatures(obj)

        if FeatureTool & features:
            for op in job.Operations.Group[-2::-1]:
                obj.ToolController = PathUtil.toolControllerForOp(op)
                if obj.ToolController:
                    break
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
            if self.applyExpression(obj, "StartDepth", job.SetupSheet.StartDepthExpression):
                obj.OpStartDepth = 1.0
            else:
                obj.StartDepth = 1.0
            if self.applyExpression(obj, "FinalDepth", job.SetupSheet.FinalDepthExpression):
                obj.OpFinalDepth = 0.0
            else:
                obj.FinalDepth = 0.0
        else:
            obj.StartDepth = 1.0

        if FeatureStepDown & features:
            if not self.applyExpression(obj, "StepDown", job.SetupSheet.StepDownExpression):
                obj.StepDown = "1 mm"

        if FeatureHeights & features:
            if job.SetupSheet.SafeHeightExpression:
                if not self.applyExpression(obj, "SafeHeight", job.SetupSheet.SafeHeightExpression):
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

        if FeatureLinking & features:
            obj.CollisionAvoidanceStrategy = job.SetupSheet.CollisionAvoidanceStrategy
            self.applyExpression(obj, "CollisionClearance", "OpToolDiameter")

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
                    translate("CAM", "Parent job %s doesn't have a base object") % job.Label
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

        # When 3+2 workplane is active, compute depths from transformed
        # geometry so that all Z values are in the rotated (Z-up) frame.
        matrix = getattr(self, "_geom_transform_matrix", None)

        if matrix is not None:
            stockBB = self.stock.Shape.copy().transformShape(matrix, False, False).BoundBox
        else:
            stockBB = self.stock.Shape.BoundBox
        zmin = stockBB.ZMin
        zmax = stockBB.ZMax

        obj.OpStockZMin = zmin
        obj.OpStockZMax = zmax

        if hasattr(obj, "Base") and obj.Base:
            for base, sublist in obj.Base:
                if matrix is not None:
                    transformed = base.Shape.copy().transformShape(matrix, False, False)
                    bb = transformed.BoundBox
                else:
                    transformed = None
                    bb = base.Shape.BoundBox
                zmax = max(zmax, bb.ZMax)
                for sub in sublist:
                    try:
                        if sub:
                            if transformed is not None:
                                fbb = transformed.getElement(sub).BoundBox
                            else:
                                fbb = base.Shape.getElement(sub).BoundBox
                        else:
                            fbb = bb
                        zmin = max(zmin, faceZmin(bb, fbb))
                        zmax = max(zmax, fbb.ZMax)
                    except Part.OCCError as e:
                        Path.Log.error(e)

        else:
            # clearing with stock boundaries
            job = PathUtils.findParentJob(obj)
            zmax = stockBB.ZMax
            if matrix is not None:
                # Transform model bounding box to get Z in rotated frame
                modelBB = job.Proxy.modelBoundBox(job)
                rot = getattr(self, "_geometry_rotation", None)
                if rot is not None:
                    corners = [
                        FreeCAD.Vector(modelBB.XMin, modelBB.YMin, modelBB.ZMin),
                        FreeCAD.Vector(modelBB.XMax, modelBB.YMin, modelBB.ZMin),
                        FreeCAD.Vector(modelBB.XMin, modelBB.YMax, modelBB.ZMin),
                        FreeCAD.Vector(modelBB.XMax, modelBB.YMax, modelBB.ZMin),
                        FreeCAD.Vector(modelBB.XMin, modelBB.YMin, modelBB.ZMax),
                        FreeCAD.Vector(modelBB.XMax, modelBB.YMin, modelBB.ZMax),
                        FreeCAD.Vector(modelBB.XMin, modelBB.YMax, modelBB.ZMax),
                        FreeCAD.Vector(modelBB.XMax, modelBB.YMax, modelBB.ZMax),
                    ]
                    transformed_corners = [rot.multVec(c) for c in corners]
                    zmin = max(c.z for c in transformed_corners)
                else:
                    zmin = modelBB.ZMax
            else:
                zmin = job.Proxy.modelBoundBox(job).ZMax

        if FeatureDepths & self.opFeatures(obj):
            # first set update final depth, it's value is not negotiable
            if not Path.Geom.isRoughly(obj.OpFinalDepth.Value, zmin):
                obj.OpFinalDepth = zmin
            zmin = obj.OpFinalDepth.Value

            # ensure zmax is higher than zmin
            if zmax < zmin or Path.Geom.isRoughly(zmax, zmin):
                zmax = zmin

            # update start depth if requested and required
            if not Path.Geom.isRoughly(obj.OpStartDepth.Value, zmax):
                obj.OpStartDepth = zmax
        else:
            # every obj has a StartDepth
            if obj.StartDepth.Value != zmax:
                obj.StartDepth = zmax

        self.opUpdateDepths(obj)

    def checkBase(self, obj):
        """checkBase(obj) ... check if Base is valid."""
        if hasattr(obj, "Base"):
            try:
                for o, sublist in obj.Base:
                    for sub in sublist:
                        o.Shape.getElement(sub)
            except Exception:
                Path.Log.error(
                    "%s - stale base geometry detected - %s, %s" % (obj.Label, o.Label, sub)
                )
                self.isBaseValid = False
                return False

        self.isBaseValid = True
        return True

    def _setup_workplane_transform(self, obj):
        """Set up 3+2 geometry transformation if workplane is not Z-up.

        When the workplane is rotated, this method:
        1. Solves for the rotary axis angles via the orientation solver
        2. Computes the geometry transform matrix (rotation that maps the
           workplane normal to Z-up)
        3. Stores rotation G-code commands for later emission
        4. Sets ``self._geom_transform_matrix`` so that ``updateDepths()``
           and ``baseShapes()`` see transformed geometry

        Returns:
            True if the operation may proceed (workplane is Z-up, or rotation
            was successfully set up). False if the workplane requires rotation
            but it cannot be applied — the caller must abort execution rather
            than running the op against an unrotated, non-Z-up workplane.
        """
        # Clean any stale state from a previous execute()
        for attr in ("_geom_transform_matrix", "_geometry_rotation", "_rotation_commands"):
            if hasattr(self, attr):
                delattr(self, attr)

        if not hasattr(obj, "Workplane"):
            return True

        wp = obj.Workplane
        z_up = FreeCAD.Vector(0, 0, 1)

        # Check if workplane is effectively Z-up (no rotation needed)
        if wp.isEqual(z_up, 1e-6):
            return True

        # Need rotation — get machine from job
        machine = self.job.Proxy.getMachine() if self.job else None
        if machine is None or not machine.has_rotary_axes:
            Path.Log.warning(
                f"Operation {obj.Label}: Workplane requires rotation but "
                f"no machine with rotary axes is configured"
            )
            return False

        # Solve orientation
        try:
            import Path.Base.Generator.rotation as rotation

            result = rotation.solve_orientation(machine, wp)
            Path.Log.debug(result)

            if not result.success:
                Path.Log.error(
                    f"Operation {obj.Label}: Cannot solve workplane "
                    f"orientation: {result.reason}"
                )
                return False

            # Build rotation commands (G0 moves for each rotary axis)
            cmd_params = {name: angle for name, angle in result.angles.items()}
            rotation_cmds = [Path.Command("G0", cmd_params)] if cmd_params else []

            # Compute the geometry transform matrix.
            chain = rotation.build_kinematic_chain(machine)
            Path.Log.debug(f"Chain: {chain}")
            Path.Log.debug(f"Solution Angles: {result.angles}")
            geom_rotation = rotation.compute_rotation_matrix(chain, result.angles)
            Path.Log.debug(f"Geometry rotation: {geom_rotation}")

            # Store as FreeCAD.Matrix for transformShape()
            self._geom_transform_matrix = geom_rotation.toMatrix()
            self._geometry_rotation = geom_rotation
            self._rotation_commands = rotation_cmds

            Path.Log.info(
                f"Operation {obj.Label}: 3+2 workplane active, " f"angles={result.angles}"
            )
            return True

        except Exception as e:
            Path.Log.error(f"Operation {obj.Label}: Error setting up workplane " f"transform: {e}")
            # Clean up on failure
            for attr in ("_geom_transform_matrix", "_geometry_rotation", "_rotation_commands"):
                if hasattr(self, attr):
                    delattr(self, attr)
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

        job = getattr(self, "job", None) or PathUtils.findParentJob(obj)
        if job and "freezed" in job.getStatusString().casefold():
            return

        if not obj.Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            return

        if not self._setBaseAndStock(obj):
            return

        # make sure Base is still valid
        if not self.checkBase(obj):
            obj.Path = Path.Path()
            raise Exception("Base geometry error!")

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

        # --- 3+2 Setup: compute geometry transformation before depth calculation ---
        # If the workplane is not Z-up, solve the orientation and set up the
        # transform matrix so that updateDepths() sees transformed BoundBoxes
        # and baseShapes() yields transformed geometry. Abort the op cleanly
        # if rotation is required but unavailable — otherwise downstream
        # geometry ops would fail with confusing errors against unrotated input.
        if not self._setup_workplane_transform(obj):
            obj.Path = Path.Path("(workplane rotation unavailable)")
            return

        self.updateDepths(obj)
        # now that all op values are set make sure the user properties get updated accordingly,
        # in case they still have an expression referencing any op values
        obj.recompute()

        self.commandlist = []
        self.commandlist.append(Path.Command("(%s)" % obj.Label))
        if obj.Comment:
            self.commandlist.append(Path.Command("(%s)" % obj.Comment))

        # Emit rotation commands if 3+2 is active
        if hasattr(self, "_rotation_commands"):
            self.commandlist.extend(self._rotation_commands)
            delattr(self, "_rotation_commands")

        # If a geometry transform is active, wrap self.model and self.stock
        # in proxy objects so operations that access them see Z-up geometry.
        # This only touches plain Python attributes — no FreeCAD properties
        # are written, so no recomputes are triggered.
        saved_model = self.model
        saved_stock = self.stock
        matrix = getattr(self, "_geom_transform_matrix", None)
        if matrix is not None:

            def transform_shape(obj):
                if not hasattr(obj, "Shape") or not obj.Shape:
                    return obj
                final_shape = _transform_shape_with_arc_fix(obj.Shape, matrix)
                return _TransformedShapeProxy(obj, final_shape)

            self.model = [transform_shape(m) for m in self.model]
            if self.stock and hasattr(self.stock, "Shape") and self.stock.Shape:
                self.stock = transform_shape(self.stock)

        try:
            result = self.opExecute(obj)
        finally:
            # Always restore originals, even if opExecute raises
            self.model = saved_model
            self.stock = saved_stock

        # Add block delete annotations if enabled
        if hasattr(obj, "BlockDelete") and obj.BlockDelete:
            for command in self.commandlist:
                annotations = command.Annotations
                annotations["BlockDelete"] = True
                command.Annotations = annotations

        # Add handling of coolant commands.
        # if the coolant mode is not None, add the command to turn it on right before the first non-rapid
        # move in the command list.
        # Add the command to turn it off right after the last non-rapid move in the command list.
        if hasattr(obj, "CoolantMode") and obj.CoolantMode != "None":
            # Find the first and last cutting moves (includes G1, G2, G3, and canned drill cycles)
            # Use Path.Geom.CmdMove which includes: G1, G2, G3, G73, G81, G82, G83, G85
            first_feed_index = None
            last_feed_index = None

            for i, cmd in enumerate(self.commandlist):
                if cmd.Name in Path.Geom.CmdMove:
                    if first_feed_index is None:
                        first_feed_index = i
                    last_feed_index = i

            # Insert coolant commands if we found cutting moves
            if first_feed_index is not None:
                # Insert coolant on command before first cutting move
                if obj.CoolantMode == "Flood":
                    coolant_on = Path.Command("M8", {})
                elif obj.CoolantMode == "Mist":
                    coolant_on = Path.Command("M7", {})
                else:
                    coolant_on = None

                if coolant_on:
                    self.commandlist.insert(first_feed_index, coolant_on)
                    # Adjust last_feed_index since we inserted a command
                    last_feed_index += 1

                    # Insert coolant off command after last cutting move
                    coolant_off = Path.Command("M9", {})
                    self.commandlist.insert(last_feed_index + 1, coolant_off)

        if self.commandlist and (FeatureHeights & self.opFeatures(obj)):
            # Let's finish by rapid to clearance...just for safety
            self.commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        path = Path.Path(self.commandlist)

        # Clean up temporary 3+2 attributes
        for attr in ("_geometry_rotation", "_geom_transform_matrix"):
            if hasattr(self, attr):
                delattr(self, attr)

        obj.Path = path
        obj.CycleTime = getCycleTimeEstimate(obj)
        self.job.Proxy.getCycleTime()
        return result

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
                        (translate("CAM", "Base object %s.%s already in the list") + "\n")
                        % (base.Label, sub)
                    )
                    return

            if not self.opRejectAddBase(obj, base, sub):
                baselist.append((base, sub))
                obj.Base = baselist
            else:
                Path.Log.notice(
                    (translate("CAM", "Base object %s.%s rejected by operation") + "\n")
                    % (base.Label, sub)
                )

    def isToolSupported(self, obj, tool):
        """toolSupported(obj, tool) ... Returns true if the op supports the given tool.
        This function can safely be overwritten by subclasses."""

        return True


class Compass:
    """
    A compass is a tool to help with direction so the Compass is a helper
    class to manage settings that affect tool and spindle direction.

    Settings managed:
        - Spindle Direction: Forward / Reverse / None
        - Cut Side: Inside / Outside (for perimeter operations)
        - Cut Mode: Climb / Conventional
        - Path Direction: CW / CCW (derived for perimeter operations)
        - Operation Type: Perimeter / Area (for facing/pocketing operations)

    This class allows the user to set and get any of these properties and the rest will update accordingly.
    Supports both perimeter operations (profiling) and area operations (facing, pocketing).

    Args:
        spindle_direction: "Forward", "Reverse", or "None"
        operation_type: "Perimeter" or "Area" (defaults to "Perimeter")
    """

    FORWARD = "Forward"
    REVERSE = "Reverse"
    NONE = "None"
    CW = "CW"
    CCW = "CCW"
    CLIMB = "Climb"
    CONVENTIONAL = "Conventional"
    INSIDE = "Inside"
    OUTSIDE = "Outside"
    PERIMETER = "Perimeter"
    AREA = "Area"

    def __init__(self, spindle_direction, operation_type=None):
        self._spindle_dir = (
            spindle_direction
            if spindle_direction in (self.FORWARD, self.REVERSE, self.NONE)
            else self.NONE
        )
        self._cut_side = self.OUTSIDE
        self._cut_mode = self.CLIMB
        self._operation_type = (
            operation_type or self.PERIMETER
        )  # Default to perimeter for backward compatibility
        self._path_dir = self._calculate_path_dir()

    @property
    def spindle_dir(self):
        return self._spindle_dir

    @spindle_dir.setter
    def spindle_dir(self, value):
        if value in (self.FORWARD, self.REVERSE, self.NONE):
            self._spindle_dir = value
            self._path_dir = self._calculate_path_dir()
        else:
            self._spindle_dir = self.NONE
            self._path_dir = self._calculate_path_dir()

    @property
    def cut_side(self):
        return self._cut_side

    @cut_side.setter
    def cut_side(self, value):
        self._cut_side = value.capitalize()
        self._path_dir = self._calculate_path_dir()

    @property
    def cut_mode(self):
        return self._cut_mode

    @cut_mode.setter
    def cut_mode(self, value):
        self._cut_mode = value.capitalize()
        self._path_dir = self._calculate_path_dir()

    @property
    def operation_type(self):
        return self._operation_type

    @operation_type.setter
    def operation_type(self, value):
        self._operation_type = value.capitalize()
        self._path_dir = self._calculate_path_dir()

    @property
    def path_dir(self):
        return self._path_dir

    def _calculate_path_dir(self):
        if self.spindle_dir == self.NONE:
            return "UNKNOWN"

        # For area operations (facing, pocketing), path direction is not applicable
        if self._operation_type == self.AREA:
            return "N/A"

        spindle_rotation = self._rotation_from_spindle(self.spindle_dir)

        for candidate in (self.CW, self.CCW):
            mode = self._expected_cut_mode(self._cut_side, spindle_rotation, candidate)
            if mode == self._cut_mode:
                return candidate

        return "UNKNOWN"

    def _rotation_from_spindle(self, direction):
        return self.CW if direction == self.FORWARD else self.CCW

    def _expected_cut_mode(self, cut_side, spindle_rotation, path_dir):
        lookup = {
            (self.INSIDE, self.CW, self.CCW): self.CLIMB,
            (self.INSIDE, self.CCW, self.CW): self.CLIMB,
            (self.OUTSIDE, self.CW, self.CW): self.CLIMB,
            (self.OUTSIDE, self.CCW, self.CCW): self.CLIMB,
        }
        return lookup.get((cut_side, spindle_rotation, path_dir), self.CONVENTIONAL)

    def get_step_direction(self, approach_direction):
        """
        For area operations, determine the step direction for climb/conventional milling.

        Args:
            approach_direction: "X+", "X-", "Y+", "Y-" - the primary cutting direction

        Returns:
            True if steps should be in positive direction, False for negative direction
        """
        if self._operation_type != self.AREA:
            raise ValueError("Step direction is only applicable for area operations")

        if self.spindle_dir == self.NONE:
            return True  # Default to positive direction

        spindle_rotation = self._rotation_from_spindle(self.spindle_dir)

        # For area operations, climb/conventional depends on relationship between
        # spindle rotation, approach direction, and step direction
        if approach_direction in ["X-", "X+"]:
            # Stepping in Y direction
            if self._cut_mode == self.CLIMB:
                # Climb: step direction matches spindle for X- approach
                return (approach_direction == "X-") == (spindle_rotation == self.CW)
            else:  # Conventional
                # Conventional: step direction opposite to spindle for X- approach
                return (approach_direction == "X-") != (spindle_rotation == self.CW)
        else:  # Y approach
            # Stepping in X direction
            if self._cut_mode == self.CLIMB:
                # Climb: step direction matches spindle for Y- approach
                return (approach_direction == "Y-") == (spindle_rotation == self.CW)
            else:  # Conventional
                # Conventional: step direction opposite to spindle for Y- approach
                return (approach_direction == "Y-") != (spindle_rotation == self.CW)

    def get_cutting_direction(self, approach_direction, pass_index=0, pattern="zigzag"):
        """
        For area operations, determine the cutting direction for each pass.

        Args:
            approach_direction: "X+", "X-", "Y+", "Y-" - the primary cutting direction
            pass_index: Index of the current pass (0-based)
            pattern: "zigzag", "unidirectional", "spiral"

        Returns:
            True if cutting should be in forward direction, False for reverse
        """
        if self._operation_type != self.AREA:
            raise ValueError("Cutting direction is only applicable for area operations")

        if self.spindle_dir == self.NONE:
            return True  # Default to forward direction

        spindle_rotation = self._rotation_from_spindle(self.spindle_dir)

        # Determine base cutting direction for climb/conventional
        if approach_direction in ["X-", "X+"]:
            # Cutting along Y axis
            if self._cut_mode == self.CLIMB:
                base_forward = (approach_direction == "X-") == (spindle_rotation == self.CW)
            else:  # Conventional
                base_forward = (approach_direction == "X-") != (spindle_rotation == self.CW)
        else:  # Y approach
            # Cutting along X axis
            if self._cut_mode == self.CLIMB:
                base_forward = (approach_direction == "Y-") == (spindle_rotation == self.CW)
            else:  # Conventional
                base_forward = (approach_direction == "Y-") != (spindle_rotation == self.CW)

        # Apply pattern modifications
        if pattern == "zigzag" and pass_index % 2 == 1:
            base_forward = not base_forward
        elif pattern == "unidirectional":
            # Always same direction
            pass

        return base_forward

    def report(self):
        report_data = {
            "spindle_dir": self.spindle_dir,
            "cut_side": self.cut_side,
            "cut_mode": self.cut_mode,
            "operation_type": self.operation_type,
            "path_dir": self.path_dir,
        }

        Path.Log.debug("Machining Compass config:")
        for k, v in report_data.items():
            Path.Log.debug(f"  {k:15s}: {v}")
        return report_data


def SetupPropertiesLinking():
    setup = []
    setup.append("CollisionAvoidanceStrategy")
    setup.append("CollisionClearance")
    return setup
