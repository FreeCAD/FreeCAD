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

from draftutils.messages import _msg, _wrn, _err
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
        It is the length from the end of the path to the last copy.
    """

    def __init__(self, obj):
        super(PathArray, self).__init__(obj, "PathArray")

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
        super(PathArray, self).attach(obj)

    def set_properties(self, obj):
        """Set properties only if they don't exist."""
        if not obj:
            return

        if hasattr(obj, "PropertiesList"):
            properties = obj.PropertiesList
        else:
            properties = []

        self.set_general_properties(obj, properties)
        self.set_align_properties(obj, properties)

    def set_general_properties(self, obj, properties):
        """Set general properties only if they don't exist."""
        if "Base" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","The base object that will be duplicated")
            obj.addProperty("App::PropertyLinkGlobal",
                            "Base",
                            "Objects",
                            _tip)
            obj.Base = None

        if "PathObject" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","The object along which the copies will be distributed. It must contain 'Edges'.")
            obj.addProperty("App::PropertyLinkGlobal",
                            "PathObject",
                            "Objects",
                            _tip)
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
                            _tip)
            obj.PathSubelements = []

        if "Count" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Number of copies to create")
            obj.addProperty("App::PropertyInteger",
                            "Count",
                            "Objects",
                            _tip)
            obj.Count = 4

        if self.use_link and "ExpandArray" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Show the individual array elements (only for Link arrays)")
            obj.addProperty("App::PropertyBool",
                            "ExpandArray",
                            "Objects",
                            _tip)
            obj.ExpandArray = False
            obj.setPropertyStatus('Shape', 'Transient')

    def set_align_properties(self, obj, properties):
        """Set general properties only if they don't exist."""
        if "ExtraTranslation" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Additional translation that will be applied to each copy.\nThis is useful to adjust for the difference between shape centre and shape reference point.")
            obj.addProperty("App::PropertyVectorDistance",
                            "ExtraTranslation",
                            "Alignment",
                            _tip)
            obj.ExtraTranslation = App.Vector(0, 0, 0)

        if "TangentVector" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Alignment vector for 'Tangent' mode")
            obj.addProperty("App::PropertyVector",
                            "TangentVector",
                            "Alignment",
                            _tip)
            obj.TangentVector = App.Vector(1, 0, 0)

        if "ForceVertical" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Force use of 'Vertical Vector' as local Z direction when using 'Original' or 'Tangent' alignment mode")
            obj.addProperty("App::PropertyBool",
                            "ForceVertical",
                            "Alignment",
                            _tip)
            obj.ForceVertical = False

        if "VerticalVector" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Direction of the local Z axis when 'Force Vertical' is true")
            obj.addProperty("App::PropertyVector",
                            "VerticalVector",
                            "Alignment",
                            _tip)
            obj.VerticalVector = App.Vector(0, 0, 1)

        if "AlignMode" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Method to orient the copies along the path.\n- Original: X is curve tangent, Y is normal, and Z is the cross product.\n- Frenet: aligns the object following the local coordinate system along the path.\n- Tangent: similar to 'Original' but the local X axis is pre-aligned to 'Tangent Vector'.\n\nTo get better results with 'Original' or 'Tangent' you may have to set 'Force Vertical' to true.")
            obj.addProperty("App::PropertyEnumeration",
                            "AlignMode",
                            "Alignment",
                            _tip)
            obj.AlignMode = ['Original', 'Frenet', 'Tangent']
            obj.AlignMode = 'Original'

        if "StartOffset" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Length from the start of the path to the first copy.")
            obj.addProperty("App::PropertyLength",
                            "StartOffset",
                            "Alignment",
                            _tip)
            obj.StartOffset = 0.0

        if "EndOffset" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Length from the end of the path to the last copy.")
            obj.addProperty("App::PropertyLength",
                            "EndOffset",
                            "Alignment",
                            _tip)
            obj.EndOffset = 0.0

        # The Align property must be attached after other align properties
        # so that onChanged works properly
        if "Align" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property","Orient the copies along the path depending on the 'Align Mode'.\nOtherwise the copies will have the same orientation as the original Base object.")
            obj.addProperty("App::PropertyBool",
                            "Align",
                            "Alignment",
                            _tip)
            obj.Align = False

    def linkSetup(self, obj):
        """Set up the object as a link object."""
        super(PathArray, self).linkSetup(obj)
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
                                             obj.EndOffset.Value)

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
        super(PathArray, self).onChanged(obj, prop)
        self.show_and_hide(obj, prop)

    def show_and_hide(self, obj, prop):
        """Show and hide the properties depending on the touched property.

        Note that when the array is created, some properties will change
        more than once in a seemingly random order.
        """
        # The minus sign removes the Hidden property (show).
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
        """Execute code when the document is restored.

        Add properties that don't exist.
        """
        self.set_properties(obj)
        self.migrate_properties_0v19(obj)
        super(PathArray, self).onDocumentRestored(obj)

    def migrate_properties_0v19(self, obj):
        """Migrate properties of this class, not from the parent class."""
        properties = obj.PropertiesList

        if "PathObj" in properties:
            obj.PathObject = obj.PathObj
            obj.removeProperty("PathObj")
            _wrn("v0.19, " + obj.Label + ", " + translate("draft","'PathObj' property will be migrated to 'PathObject'"))

        if "PathSubs" in properties:
            obj.PathSubelements = obj.PathSubs
            obj.removeProperty("PathSubs")
            _info = "'PathSubs' property will be migrated to 'PathSubelements'"
            _wrn("v0.19, " + obj.Label + ", " + translate("draft","'PathObj' property will be migrated to 'PathObject'"))

        if "Xlate" in properties:
            obj.ExtraTranslation = obj.Xlate
            obj.removeProperty("Xlate")
            _info = "'Xlate' property will be migrated to 'ExtraTranslation'"
            _wrn("v0.19, " + obj.Label + ", " + translate("draft","'PathObj' property will be migrated to 'PathObject'"))


# Alias for compatibility with v0.18 and earlier
_PathArray = PathArray


def placements_on_path(shapeRotation, pathwire, count, xlate, align,
                       mode="Original", forceNormal=False,
                       normalOverride=None,
                       startOffset=0.0, endOffset=0.0):
    """Calculate the placements of a shape along a given path.

    Copies will be distributed evenly.
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

    # find cumulative edge end distance
    cdist = 0
    ends = []
    for e in path:
        cdist += e.Length
        ends.append(cdist)

    if startOffset > (cdist - 1e-6):
        _wrn(translate("draft", "Start Offset too large for path length. Using zero instead."))
        start = 0
    else:
        start = startOffset

    if endOffset > (cdist - start - 1e-6):
        _wrn(translate("draft", "End Offset too large for path length minus Start Offset. Using zero instead."))
        end = 0
    else:
        end = endOffset

    cdist = cdist - start - end
    step = cdist / (count if (DraftGeomUtils.isReallyClosed(pathwire) and not (start or end)) else count - 1)
    remains = 0
    travel = start
    placements = []

    for i in range(0, count):
        # which edge in path should contain this shape?
        for j in range(0, len(ends)):
            if travel <= ends[j]:
                iend = j
                remains = ends[iend] - travel
                offset = path[iend].Length - remains
                break
        else:
            # avoids problems with float math travel > ends[-1]
            iend = len(ends) - 1
            offset = path[iend].Length

        # place shape at proper spot on proper edge
        pt = path[iend].valueAt(get_parameter_from_v0(path[iend], offset))
        place = calculate_placement(shapeRotation,
                                    path[iend], offset,
                                    pt, xlate, align, normal,
                                    mode, forceNormal)
        placements.append(place)

        travel += step

    return placements


calculatePlacementsOnPath = placements_on_path


def calculate_placement(globalRotation,
                        edge, offset, RefPt, xlate, align,
                        normal=App.Vector(0.0, 0.0, 1.0),
                        mode="Original", overrideNormal=False):
    """Orient shape in the local coordinate system at parameter offset.

    http://en.wikipedia.org/wiki/Euler_angles (previous version)
    http://en.wikipedia.org/wiki/Quaternions
    """
    # Default Placement:
    placement = App.Placement()
    placement.Rotation = globalRotation
    placement.Base = RefPt + placement.Rotation.multVec(xlate)

    if not align:
        return placement

    tol = 1e-6 # App.Rotation() tolerance is 1e-7. Shorter vectors are ignored.
    nullv = App.Vector()

    t = edge.tangentAt(get_parameter_from_v0(edge, offset))
    if t.isEqual(nullv, tol):
        _wrn(translate("draft", "Length of tangent vector is zero. Copy not aligned."))
        return placement

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
            newRot = App.Rotation(t, nullv, n, "XZY") # priority = "XZY"
        else:
            newRot = App.Rotation(t, n, nullv, "XYZ") # priority = "XYZ"

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

        newRot = App.Rotation(t, n, nullv, "XYZ") # priority = "XYZ"

    else:
        _msg(translate("draft", "AlignMode {} is not implemented").format(mode))
        return placement

    placement.Rotation = newRot.multiply(globalRotation)
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
