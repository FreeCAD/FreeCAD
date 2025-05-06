# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2013 WandererFan <wandererfan@gmail.com>                *
# *   Copyright (c) 2019 Zheng, Lei (realthunder)<realthunder.dev@gmail.com>*
# *   Copyright (c) 2020 Carlo Pavan <carlopav@gmail.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
"""Provides the object code for the PathArray object.

The copies will be placed along a path like a polyline, spline, or bezier
curve, and along the selected subelements.

To Do
-----
The `'PathSubelements'` property must be changed in type, as it does not need
to be an `App::PropertyLinkSubList`.
A `LinkSubList` is to select multiple subelements (edges) from multiple
objects. However, since we need to select a `'Path Object'` already,
which is a single object, the subelements that we can choose must belong
to this `'Path Object'` only.

Therefore, the correct property that must be used is `App::PropertyLinkSub`.
Then in the property editor we will be unable to select more than one object
thus preventing errors of the subelements not matching the `'PathObject'`.

In fact, both `'PathObject'` and `'PathSubelements'`
could be handled with a single `App::PropertyLinkSub` property,
as this property can be used to select a single object,
or a single object with its subelements.

In the future, we could migrate the properties, or outright break
compatibility with older objects by changing both properties
`'PathObject'` and `'PathSubelements'`.

An alternative to this would be to use a single `App::PropertyLinkSubList`.
This would allow us to build PathArrays on multiple objects and multiple
subelements (edges) of those objects at the same time. However, to do this,
the logic in `execute` would have to be changed to account for multiple
objects. Therefore, the first solution is simpler, that is, using
a single property of type `App::PropertyLinkSub`.
"""
## @package patharray
# \ingroup draftobjects
# \brief Provides the object code for the PathArray object.

import FreeCAD as App
import DraftVecUtils
import lazy_loader.lazy_loader as lz

from draftutils.messages import _wrn, _err
from draftutils.translate import translate
def QT_TRANSLATE_NOOP(ctx,txt): return txt
from draftobjects.base import DraftObject
from draftobjects.draftlink import DraftLink

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")

## \addtogroup draftobjects
# @{


class PathArray(DraftLink):
    """The Draft Path Array object.

    The object distributes copies of an object along a path like a polyline,
    spline, or bezier curve.

    Attributes
    ----------
    Align: bool
        It defaults to `False`.
        It sets whether the object will be specially aligned to the path.

    AlignMode: str
        It defaults to `'Original'`.
        Indicates the type of alignment that will be calculated when
        `Align` is `True`.

        `'Original'` mode is the historic `'Align'` for old (v0.18) documents.
        It is not really the Fernat alignment. It uses the normal parameter
        from `getNormal` (or the default) as a constant, it does not calculate
        curve normal.
        `X` is curve tangent, `Y` is normal parameter, `Z` is the cross product
        `X` x `Y`.

        `'Tangent'` mode is similar to `Original`, but includes a pre-rotation
        (in execute) to align the `Base` object's `X` to `TangentVector`,
        then `X` follows curve tangent, normal input parameter
        is the Z component.

        If `ForceVertical` is `True`, the normal parameter from `getNormal`
        is ignored, and `X` is curve tangent, `Z` is `VerticalVector`,
        and `Y` is the cross product `X` x `Z`.

        `'Frenet'` mode orients the copies to a coordinate system
        along the path.
        `X` is tangent to curve, `Y` is curve normal, `Z` is curve binormal.
        If normal cannot be computed, for example, in a straight line,
        the default is used.

    ForceVertical: bool
        It defaults to `False`.
        If it is `True`, and `AlignMode` is `'Original'` or `'Tangent'`,
        it will use the vector in `VerticalVector` as the `Z` axis.

    StartOffset: float
        It defaults to 0.0.
        It is the length from the start of the path to the first copy.

    EndOffset: float
        It defaults to 0.0.
        It is the length at the end of the path that will not be available
        for object placement.

    ReversePath: bool
        It defaults to False.
        This will walk the path in reverse, also reversing object
        orientation. Start and end offsets will count from opposite ends
        of the path, etc.

    SpacingMode: string
        It defaults to `'Fixed count'`.
        Objects can be spaced to divide the available length evenly
        (`'Fixed count'`, this is the original spacing mode from FreeCAD 1.0),
        or to be placed in given distances along the path from each other:
        `'Fixed spacing'` will keep placing objects for as long as there
        is still space available, while `'Fixed count and spacing'`
        will place a given number of objects (provided they fit in available
        space).

    SpacingUnit: length
        It defaults to 20mm.
        When fixed spacing modes are used, this is the spacing distance
        used. If UseSpacingPattern is also enabled, this is the unit length
        of "1.0" in the spacing pattern (so, default pattern of [1.0, 2.0]
        with default SpacingUnit of 20mm means a spacing pattern of
        20mm, 40mm).

    UseSpacingPattern: bool
        Default is False.
        Enables the SpacingPattern for uneven distribution of objects.
        Will have slightly different effect depending on SpacingMode.

    SpacingPattern: float list
        Default is [1.0, 2.0]
        When UseSpacingPattern is True, this list contains the proportions
        of distances between consecutive object pairs. Can be used in any
        spacing mode. In "fixed spacing" modes SpacingPattern is multiplied
        by SpacingUnit. In flexible spacing modes ("fixed count"), spacing
        pattern defines the proportion of distances.
    """

    def __init__(self, obj):
        super().__init__(obj, "PathArray")

    def attach(self, obj):
        """Set up the properties when the object is attached.

        Note: we don't exactly know why the properties are added
        in the `attach` method. They should probably be added in the `__init__`
        method. Maybe this is related to the link behavior of this class.

        For PathLinkArray, DraftLink.attach creates the link to the Base.

        Realthunder: before the big merge, there was only the attach() method
        in the view object proxy, not the object proxy.
        I added that to allow the proxy to override the C++ view provider
        type. The view provider type is normally determined by object's
        C++ API getViewProviderName(), and cannot be overridden by the proxy.
        I introduced the attach() method in proxy to allow the core
        to attach the proxy before creating the C++ view provider.
        """
        self.set_properties(obj)
        super().attach(obj)

    def set_properties(self, obj):
        """Set properties only if they don't exist."""
        if not obj:
            return

        if hasattr(obj, "PropertiesList"):
            properties = obj.PropertiesList
        else:
            properties = []

        self.set_general_properties(obj, properties)
        self.set_spacing_properties(obj, properties)
        self.set_align_properties(obj, properties)

    def set_general_properties(self, obj, properties):
        """Set general properties only if they don't exist."""
        if "Base" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","The base object that will be duplicated")
            obj.addProperty("App::PropertyLinkGlobal",
                            "Base",
                            "Objects",
                            _tip,
                            locked=True)
            obj.Base = None

        if "PathObject" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","The object along which the copies will be distributed. It must contain 'Edges'.")
            obj.addProperty("App::PropertyLinkGlobal",
                            "PathObject",
                            "Objects",
                            _tip,
                            locked=True)
            obj.PathObject = None

        # TODO: the 'PathSubelements' property must be changed,
        # as it does not need to be an 'App::PropertyLinkSubList'.
        #
        # In fact, both 'PathObject' and 'PathSubelements'
        # could be handled with a single 'App::PropertyLinkSub' property,
        # as this property can be used to select a single object,
        # or a single object with its subelements.
        if "PathSubelements" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","List of connected edges in the 'Path Object'.\nIf these are present, the copies will be created along these subelements only.\nLeave this property empty to create copies along the entire 'Path Object'.")
            obj.addProperty("App::PropertyLinkSubListGlobal",
                            "PathSubelements",
                            "Objects",
                            _tip,
                            locked=True)
            obj.PathSubelements = []

        if "Fuse" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Specifies if the copies "
                                     "should be fused together "
                                     "if they touch each other (slower)")
            obj.addProperty("App::PropertyBool",
                            "Fuse",
                            "Objects",
                            _tip,
                            locked=True)
            obj.Fuse = False

        if self.use_link and "ExpandArray" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Show the individual array elements (only for Link arrays)")
            obj.addProperty("App::PropertyBool",
                            "ExpandArray",
                            "Objects",
                            _tip,
                            locked=True)
            obj.ExpandArray = False
            obj.setPropertyStatus('Shape', 'Transient')

        if not self.use_link:
            if "PlacementList" not in properties:
                _tip = QT_TRANSLATE_NOOP("App::Property",
                                         "The placement for each array element")
                obj.addProperty("App::PropertyPlacementList",
                                "PlacementList",
                                "Objects",
                                _tip,
                                locked=True)
                obj.PlacementList = []

    def set_align_properties(self, obj, properties):
        """Set general properties only if they don't exist."""
        if "ExtraTranslation" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Additional translation that will be applied to each copy.\nThis is useful to adjust for the difference between shape centre and shape reference point.")
            obj.addProperty("App::PropertyVectorDistance",
                            "ExtraTranslation",
                            "Alignment",
                            _tip,
                            locked=True)
            obj.ExtraTranslation = App.Vector(0, 0, 0)

        if "TangentVector" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Alignment vector for 'Tangent' mode")
            obj.addProperty("App::PropertyVector",
                            "TangentVector",
                            "Alignment",
                            _tip,
                            locked=True)
            obj.TangentVector = App.Vector(1, 0, 0)

        if "ForceVertical" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Force use of 'Vertical Vector' as local Z direction when using 'Original' or 'Tangent' alignment mode")
            obj.addProperty("App::PropertyBool",
                            "ForceVertical",
                            "Alignment",
                            _tip,
                            locked=True)
            obj.ForceVertical = False

        if "VerticalVector" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Direction of the local Z axis when 'Force Vertical' is true")
            obj.addProperty("App::PropertyVector",
                            "VerticalVector",
                            "Alignment",
                            _tip,
                            locked=True)
            obj.VerticalVector = App.Vector(0, 0, 1)

        if "AlignMode" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Method to orient the copies along the path.\n- Original: X is curve tangent, Y is normal, and Z is the cross product.\n- Frenet: aligns the object following the local coordinate system along the path.\n- Tangent: similar to 'Original' but the local X axis is pre-aligned to 'Tangent Vector'.\n\nTo get better results with 'Original' or 'Tangent' you may have to set 'Force Vertical' to true.")
            obj.addProperty("App::PropertyEnumeration",
                            "AlignMode",
                            "Alignment",
                            _tip,
                            locked=True)
            obj.AlignMode = ["Original", "Frenet", "Tangent"]
            obj.AlignMode = "Original"

        if "ReversePath" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Walk the path backwards.")
            obj.addProperty("App::PropertyBool",
                            "ReversePath",
                            "Alignment",
                            _tip,
                            locked=True)
            obj.ReversePath = False

        # The Align property must be attached after other align properties
        # so that onChanged works properly
        if "Align" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Orient the copies along the path depending on the 'Align Mode'.\nOtherwise the copies will have the same orientation as the original Base object.")
            obj.addProperty("App::PropertyBool",
                            "Align",
                            "Alignment",
                            _tip,
                            locked=True)
            obj.Align = False

    def set_spacing_properties(self, obj, properties):

        if "Count" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Number of copies to create")
            obj.addProperty("App::PropertyInteger",
                            "Count",
                            "Spacing",
                            _tip,
                            locked=True)
            obj.Count = 4

        if "SpacingMode" not in properties:
            _tip = QT_TRANSLATE_NOOP(
                "App::Property",
                "How copies are spaced.\n" +
                " - Fixed count: available path length (minus start and end offsets) is evenly divided into n.\n" +
                " - Fixed spacing: start at \"Start offset\" and place new copies after traveling a fixed distance along the path.\n" +
                " - Fixed count and spacing: same as \"Fixed spacing\", but also stop at given number of copies."
                )
            obj.addProperty("App::PropertyEnumeration",
                            "SpacingMode",
                            "Spacing",
                            _tip,
                            locked=True)
            obj.SpacingMode = ["Fixed count", "Fixed spacing", "Fixed count and spacing"]
            obj.SpacingMode = "Fixed count"

        if "SpacingUnit" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Base fixed distance between elements.")
            obj.addProperty("App::PropertyLength",
                            "SpacingUnit",
                            "Spacing",
                            _tip,
                            locked=True)
            obj.SpacingUnit = 20.0
            obj.setPropertyStatus("SpacingUnit", "Hidden")

        if "UseSpacingPattern" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Use repeating spacing patterns instead of uniform spacing.")
            obj.addProperty("App::PropertyBool",
                            "UseSpacingPattern",
                            "Spacing",
                            _tip,
                            locked=True)
            obj.UseSpacingPattern = False

        if "SpacingPattern" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Spacing is multiplied by a corresponding number in this sequence.")
            obj.addProperty("App::PropertyFloatList",
                            "SpacingPattern",
                            "Spacing",
                            _tip,
                            locked=True)
            obj.SpacingPattern = [1, 2]
            obj.setPropertyStatus("SpacingPattern", "Hidden")

        if "StartOffset" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Length from the start of the path to the first copy.")
            obj.addProperty("App::PropertyLength",
                            "StartOffset",
                            "Spacing",
                            _tip,
                            locked=True)
            obj.StartOffset = 0.0

        if "EndOffset" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Length from the end of the path to the last copy.")
            obj.addProperty("App::PropertyLength",
                            "EndOffset",
                            "Spacing",
                            _tip,
                            locked=True)
            obj.EndOffset = 0.0

    def linkSetup(self, obj):
        """Set up the object as a link object."""
        super().linkSetup(obj)
        obj.configLinkProperty(ElementCount='Count')

    def execute(self, obj):
        """Execute when the object is created or recomputed."""
        if self.props_changed_placement_only(obj) \
                or not obj.Base \
                or not obj.PathObject:
            self.props_changed_clear()
            return

        # placement of entire PathArray object
        array_placement = obj.Placement

        w = self.get_wires(obj.PathObject, obj.PathSubelements)
        if not w:
            _err(obj.PathObject.Label
                 + translate("draft",", path object doesn't have 'Edges'."))
            return

        base_rotation = obj.Base.Shape.Placement.Rotation
        final_rotation = base_rotation

        if (obj.Align and obj.AlignMode == "Tangent"
                and hasattr(obj, "TangentVector")):
            Xaxis = App.Vector(1.0, 0.0, 0.0)  # default TangentVector

            if not DraftVecUtils.equals(Xaxis, obj.TangentVector):
                # make rotation from TangentVector to X
                pre_rotation = App.Rotation(obj.TangentVector, Xaxis)
                final_rotation = base_rotation.multiply(pre_rotation)

        copy_placements = placements_on_path(final_rotation,
                                             w, obj.Count,
                                             obj.ExtraTranslation,
                                             obj.Align, obj.AlignMode,
                                             obj.ForceVertical,
                                             obj.VerticalVector,
                                             obj.StartOffset.Value,
                                             obj.EndOffset.Value,
                                             obj.ReversePath,
                                             obj.SpacingMode,
                                             obj.SpacingUnit.Value,
                                             obj.UseSpacingPattern,
                                             obj.SpacingPattern)

        self.buildShape(obj, array_placement, copy_placements)
        self.props_changed_clear()
        return (not self.use_link)

    def get_wires(self, path_object, subelements):
        """Get wires from the path object."""
        if subelements:
            w = self.get_wire_from_subelements(subelements)
        elif (hasattr(path_object.Shape, 'Wires')
              and path_object.Shape.Wires):
            w = path_object.Shape.Wires[0]
        elif path_object.Shape.Edges:
            w = Part.Wire(path_object.Shape.Edges)
        else:
            w = None
        return w

    def get_wire_from_subelements(self, subelements):
        """Make a wire from the path object subelements."""
        sl = []
        for sub in subelements:
            edgeNames = sub[1]
            for n in edgeNames:
                e = sub[0].Shape.getElement(n)
                sl.append(e)
        return Part.Wire(Part.__sortEdges__(sl))

    def onChanged(self, obj, prop):
        """Execute when a property is changed."""
        super().onChanged(obj, prop)
        self.show_and_hide(obj, prop)

    def show_and_hide(self, obj, prop):
        """Show and hide the properties depending on the touched property.

        Note that when the array is created, some properties will change
        more than once in a seemingly random order.
        """
        # The minus sign removes the Hidden property (show).

        if prop == "SpacingMode":

            # Check if all referenced properties are available:
            for pr in ("SpacingMode",
                "SpacingUnit",
                "UseSpacingPattern",
                "SpacingPattern"):
                if not hasattr(obj, pr):
                    return

            if obj.SpacingMode == "Fixed spacing":
                obj.setPropertyStatus("Count", "Hidden")
                obj.setPropertyStatus("SpacingUnit", "-Hidden")

            elif obj.SpacingMode == "Fixed count":
                obj.setPropertyStatus("Count", "-Hidden")
                obj.setPropertyStatus("SpacingUnit", "Hidden")

            elif obj.SpacingMode == "Fixed count and spacing":
                obj.setPropertyStatus("Count", "-Hidden")
                obj.setPropertyStatus("SpacingUnit", "-Hidden")

        if prop == "UseSpacingPattern":

            # Check if referenced property is available:
            if not hasattr(obj, "SpacingPattern"):
                return

            if obj.UseSpacingPattern:
                obj.setPropertyStatus("SpacingPattern", "-Hidden")
            else:
                obj.setPropertyStatus("SpacingPattern", "Hidden")

        if prop in ("Align", "AlignMode"):

            # Check if all referenced properties are available:
            for pr in ("Align", "AlignMode", "ForceVertical",
                       "VerticalVector", "TangentVector"):
                if not hasattr(obj, pr):
                    return

            if obj.Align:
                obj.setPropertyStatus("AlignMode", "-Hidden")

                if obj.AlignMode == "Frenet":
                    for pr in ("ForceVertical", "VerticalVector"):
                        obj.setPropertyStatus(pr, "Hidden")
                else:
                    for pr in ("ForceVertical", "VerticalVector"):
                        obj.setPropertyStatus(pr, "-Hidden")

                if obj.AlignMode == "Tangent":
                    obj.setPropertyStatus("TangentVector", "-Hidden")
                else:
                    obj.setPropertyStatus("TangentVector", "Hidden")

            else:
                for pr in ("AlignMode", "ForceVertical",
                           "VerticalVector", "TangentVector"):
                    obj.setPropertyStatus(pr, "Hidden")

    def onDocumentRestored(self, obj):
        super().onDocumentRestored(obj)
        # ReversePath was added together with several Spacing properties in v1.1,
        # and PlacementList property was added for non-link arrays in v1.1,
        # obj should be OK if both are present:
        if hasattr(obj, "ReversePath") and hasattr(obj, "PlacementList"):
            return

        if hasattr(obj, "PathObj"):
            _wrn("v0.19, " + obj.Label + ", " + translate("draft", "migrated 'PathObj' property to 'PathObject'"))
        if hasattr(obj, "PathSubs"):
            _wrn("v0.19, " + obj.Label + ", " + translate("draft", "migrated 'PathSubs' property to 'PathSubelements'"))
        if hasattr(obj, "Xlate"):
            _wrn("v0.19, " + obj.Label + ", " + translate("draft", "migrated 'Xlate' property to 'ExtraTranslation'"))
        if not hasattr(obj, "Fuse"):
            _wrn("v1.0, " + obj.Label + ", " + translate("draft", "added 'Fuse' property"))
        if obj.getGroupOfProperty("Count") != "Spacing":
            _wrn("v1.1, " + obj.Label + ", " + translate("draft", "moved 'Count' property to 'Spacing' subsection"))
        if not hasattr(obj, "ReversePath"):
            _wrn("v1.1, " + obj.Label + ", " + translate("draft", "added 'ReversePath', 'SpacingMode', 'SpacingUnit', 'UseSpacingPattern' and 'SpacingPattern' properties"))
        if not hasattr(obj, "PlacementList"):
            _wrn("v1.1, " + obj.Label + ", " + translate("draft", "added hidden property 'PlacementList'"))

        self.set_properties(obj)
        obj.setGroupOfProperty("Count", "Spacing")
        if hasattr(obj, "PathObj"):
            obj.PathObject = obj.PathObj
            obj.removeProperty("PathObj")
        if hasattr(obj, "PathSubs"):
            obj.PathSubelements = obj.PathSubs
            obj.removeProperty("PathSubs")
        if hasattr(obj, "Xlate"):
            obj.ExtraTranslation = obj.Xlate
            obj.removeProperty("Xlate")
        self.execute(obj) # Required to update PlacementList.


# Alias for compatibility with v0.18 and earlier
_PathArray = PathArray


def placements_on_path(shapeRotation, pathwire, count, xlate, align,
                       mode="Original", forceNormal=False,
                       normalOverride=None,
                       startOffset=0.0, endOffset=0.0,
                       reversePath=False,
                       spacingMode="Fixed count",
                       spacingUnit=20.0,
                       useSpacingPattern=False,
                       spacingPattern=[1, 1, 1, 1]):
    """Calculate the placements of a shape along a given path.

    Copies will be distributed according to spacing mode - evenly or in fixed offsets.
    """

    if mode == "Frenet":
        forceNormal = False

    if forceNormal and normalOverride:
        normal = normalOverride
    else:
        normal = DraftGeomUtils.get_normal(pathwire)
        if normal is None:
            normal = App.Vector(0, 0, 1)

    path = Part.__sortEdges__(pathwire.Edges)

    # if ReversePath is on, walk the path backwards:
    if reversePath:
        path = path[::-1]

    # find cumulative edge end distance
    totalDist = 0
    ends = []
    for e in path:
        totalDist += e.Length
        ends.append(totalDist)

    if startOffset > (totalDist - 1e-6):
        if startOffset != 0:
            _wrn(
                translate(
                    "draft",
                    "Start Offset too large for path length. Using zero instead."
                )
            )
        startOffset = 0

    if endOffset > (totalDist - startOffset - 1e-6):
        if endOffset != 0:
            _wrn(
                translate(
                    "draft",
                    "End Offset too large for path length minus Start Offset. Using zero instead."
                )
            )
        endOffset = 0

    totalDist = totalDist - startOffset - endOffset

    useFlexibleSpacing = spacingMode == "Fixed count"
    useFixedSpacing = spacingMode in ("Fixed spacing", "Fixed count and spacing")

    stopAfterCount = spacingMode in ("Fixed count", "Fixed count and spacing")
    stopAfterDistance = spacingMode in ("Fixed spacing", "Fixed count and spacing")

    spacingUnit = max(spacingUnit, 0)
    # protect from infinite loop when step = 0
    if spacingUnit == 0:
        _wrn(translate("draft", "Spacing unit of 0 is not allowed, using default"))
        spacingUnit = totalDist

    # negative spacing steps are not defined
    spacingPattern = [abs(w) for w in spacingPattern]

    # protect from infinite loop when pattern weights are all zeros
    if sum(spacingPattern) == 0:
        spacingPattern = [spacingUnit]

    isClosedPath = DraftGeomUtils.isReallyClosed(pathwire) and not (startOffset or endOffset)

    count = max(count, 1)

    if useFlexibleSpacing:
        # Spaces between objects will stretch to fill available length

        segCount = count if isClosedPath else count - 1
        segCount = max(segCount, 1)

        if useSpacingPattern:
            # Available length will be non-uniformly divided in proportions from SpacingPattern:
            fullSpacingPattern = [spacingPattern[i % len(spacingPattern)] for i in range(segCount)]
            sumWeights = sum(fullSpacingPattern)
            distPerWeightUnit = totalDist / sumWeights
            steps = [distPerWeightUnit * weight for weight in fullSpacingPattern]

        else:
            # Available length will be evenly divided (the original spacing method):
            steps = [totalDist / segCount]

    if useFixedSpacing:
        # Objects will be placed in specified intervals

        if useSpacingPattern:
            # Intervals will be fixed, but follow a repeating pattern:
            steps = [spacingUnit * mult for mult in spacingPattern]
        else:
            # Each interval will be the same:
            steps = [spacingUnit]


    remains = 0
    travel = startOffset
    endTravel = startOffset + totalDist
    placements = []

    i = 0
    while True:
        # which edge in path should contain this shape?
        for j in range(len(ends)):
            if travel <= ends[j]:
                iend = j
                remains = ends[iend] - travel
                offset = path[iend].Length - remains if not reversePath else remains
                break
        else:
            # avoids problems with float math travel > ends[-1]
            iend = len(ends) - 1
            offset = path[iend].Length if not reversePath else 0

        # place shape at proper spot on proper edge
        pt = path[iend].valueAt(get_parameter_from_v0(path[iend], offset))
        place = calculate_placement(shapeRotation,
                                    path[iend], offset,
                                    pt, xlate, align, normal,
                                    mode, forceNormal,
                                    reversePath)
        placements.append(place)
        travel += steps[i % len(steps)]
        i = i + 1

        # End conditions:
        if stopAfterDistance and travel > endTravel: break
        if stopAfterCount and i >= count: break

        # Failsafe:
        if i > 10_000:
            _wrn(translate("draft", "Operation would generate too many objects. Aborting"))
            return placements[0:1]

    return placements


calculatePlacementsOnPath = placements_on_path


def calculate_placement(globalRotation,
                        edge, offset, RefPt, xlate, align,
                        normal=App.Vector(0.0, 0.0, 1.0),
                        mode="Original", overrideNormal=False,
                        reversePath=False):
    """Orient shape in the local coordinate system at parameter offset.

    http://en.wikipedia.org/wiki/Euler_angles (previous version)
    http://en.wikipedia.org/wiki/Quaternions
    """
    # Default Placement:
    placement = App.Placement()
    placement.Rotation = globalRotation.inverted() if reversePath else globalRotation
    placement.Base = RefPt + placement.Rotation.multVec(xlate)

    if not align:
        return placement

    tol = 1e-6 # App.Rotation() tolerance is 1e-7. Shorter vectors are ignored.
    nullv = App.Vector()

    t = edge.tangentAt(get_parameter_from_v0(edge, offset))

    if t.isEqual(nullv, tol):
        _wrn(translate("draft", "Length of tangent vector is zero. Copy not aligned."))
        return placement

    if reversePath:
        t.multiply(-1)

    # If the length of the normal is zero or if it is parallel to the tangent,
    # we make the vectors equal (n = t). The App.Rotation() algorithm will
    # then replace the normal with a default axis.
    # For the vector with the lowest App.Rotation() priority we use a null
    # vector. Calculating this binormal would not make sense in the mentioned
    # cases. And in all other cases calculating it is not necessary as
    # App.Rotation() will ignore it.

    if mode in ("Original", "Tangent"):
        n = normal
        if n.isEqual(nullv, tol):
            _wrn(translate("draft", "Length of normal vector is zero. Using a default axis instead."))
            n = t
        else:
            n_nor = n.normalize()
            t_nor = t.normalize()
            if n_nor.isEqual(t_nor, tol) or n_nor.isEqual(t_nor.negative(), tol):
                _wrn(translate("draft", "Tangent and normal vectors are parallel. Normal replaced by a default axis."))
                n = t

        if overrideNormal:
            onPathRotation = App.Rotation(t, nullv, n, "XZY") # priority = "XZY"
        else:
            onPathRotation = App.Rotation(t, n, nullv, "XYZ") # priority = "XYZ"

    elif mode == "Frenet":
        try:
            n = edge.normalAt(get_parameter_from_v0(edge, offset))
        except App.Base.FreeCADError: # no/infinite normals here
            _wrn(translate("draft", "Cannot calculate normal vector. Using the default normal instead."))
            n = normal

        if n.isEqual(nullv, tol):
            _wrn(translate("draft", "Length of normal vector is zero. Using a default axis instead."))
            n = t
        else:
            n_nor = n.normalize()
            t_nor = t.normalize()
            if n_nor.isEqual(t_nor, tol) or n_nor.isEqual(t_nor.negative(), tol):
                _wrn(translate("draft", "Tangent and normal vectors are parallel. Normal replaced by a default axis."))
                n = t

        onPathRotation = App.Rotation(t, n, nullv, "XYZ") # priority = "XYZ"

    else:
        _err(translate("draft", "AlignMode {} is not implemented").format(mode))
        return placement

    placement.Rotation = onPathRotation.multiply(globalRotation)
    placement.Base = RefPt + placement.Rotation.multVec(xlate)

    return placement


calculatePlacement = calculate_placement

def get_parameter_from_v0(edge, offset):
    """Return parameter at distance offset from edge.Vertexes[0].

    sb method in Part.TopoShapeEdge???
    """
    lpt = edge.valueAt(edge.getParameterByLength(0))
    vpt = edge.Vertexes[0].Point

    if not DraftVecUtils.equals(vpt, lpt):
        # this edge is flipped
        length = edge.Length - offset
    else:
        # this edge is right way around
        length = offset

    return edge.getParameterByLength(length)


getParameterFromV0 = get_parameter_from_v0

## @}
