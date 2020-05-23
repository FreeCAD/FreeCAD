# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2013 Wandererfan <wandererfan@gmail.com>                *
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
"""Provides the object code for the Draft PathArray object.

The copies will be placed along a path like a polyline, spline, or bezier
curve.
"""
## @package patharray
# \ingroup DRAFT
# \brief Provides the object code for the Draft PathArray object.

import FreeCAD as App
import DraftVecUtils
import lazy_loader.lazy_loader as lz

from draftutils.messages import _msg, _wrn, _err
from draftutils.translate import _tr

from draftobjects.draftlink import DraftLink

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")


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

        if "Base" not in properties:
            _tip = _tr("The base object that will be duplicated")
            obj.addProperty("App::PropertyLinkGlobal",
                            "Base",
                            "Objects",
                            _tip)
            obj.Base = None

        if "PathObj" not in properties:
            _tip = _tr("The object along which "
                       "the copies will be distributed. "
                       "It must contain 'Edges'.")
            obj.addProperty("App::PropertyLinkGlobal",
                            "PathObj",
                            "Objects",
                            _tip)
            obj.PathObj = None

        if "PathSubs" not in properties:
            _tip = _tr("List of connected edges in the 'Path Object'.\n"
                       "If these are present, the copies will be created "
                       "along these subelements only.\n"
                       "Leave this property empty to create copies along "
                       "the entire 'Path Object'.")
            obj.addProperty("App::PropertyLinkSubListGlobal",
                            "PathSubs",
                            "Objects",
                            _tip)
            obj.PathSubs = []

        if "Count" not in properties:
            _tip = _tr("Number of copies to create")
            obj.addProperty("App::PropertyInteger",
                            "Count",
                            "General",
                            _tip)
            obj.Count = 4

        if "Align" not in properties:
            _tip = _tr("Orient the copies along the path depending "
                       "on the 'Align Mode'.\n"
                       "Otherwise the copies will have the same orientation "
                       "as the original Base object.")
            obj.addProperty("App::PropertyBool",
                            "Align",
                            "Alignment",
                            _tip)
            obj.Align = False

        if "AlignMode" not in properties:
            _tip = _tr("Method to orient the copies along the path.\n"
                       "- Original, X is curve tangent, Y is normal, "
                       "and Z is the cross product.\n"
                       "- Frenet uses a local coordinate system along "
                       "the path.\n"
                       "- Tangent is similar to 'Original' but the local X "
                       "axis is pre-aligned to 'Tangent Vector'.\n"
                       "To get better results with 'Original' and 'Tangent' "
                       "you may have to set 'Force Vertical' to true.")
            obj.addProperty("App::PropertyEnumeration",
                            "AlignMode",
                            "Alignment",
                            _tip)
            obj.AlignMode = ['Original', 'Frenet', 'Tangent']
            obj.AlignMode = 'Original'

        if "Xlate" not in properties:
            _tip = _tr("Additional translation "
                       "that will be applied to each copy.\n"
                       "This is useful to adjust for the difference "
                       "between shape centre and shape reference point.")
            obj.addProperty("App::PropertyVectorDistance",
                            "Xlate",
                            "Alignment",
                            _tip)
            obj.Xlate = App.Vector(0, 0, 0)

        if "TangentVector" not in properties:
            _tip = _tr("Alignment vector for 'Tangent' mode")
            obj.addProperty("App::PropertyVector",
                            "TangentVector",
                            "Alignment",
                            _tip)
            obj.TangentVector = App.Vector(1, 0, 0)

        if "ForceVertical" not in properties:
            _tip = _tr("Force use of 'Vertical Vector' as Z direction "
                       "when using 'Original' or 'Tangent' align mode")
            obj.addProperty("App::PropertyBool",
                            "ForceVertical",
                            "Alignment",
                            _tip)
            obj.ForceVertical = False

        if "VerticalVector" not in properties:
            _tip = _tr("Direction of the local Z axis "
                       "when 'Force Vertical' is true")
            obj.addProperty("App::PropertyVector",
                            "VerticalVector",
                            "Alignment",
                            _tip)
            obj.VerticalVector = App.Vector(0, 0, 1)

        if self.use_link and "ExpandArray" not in properties:
            _tip = _tr("Show the individual array elements "
                       "(only for Link arrays)")
            obj.addProperty("App::PropertyBool",
                            "ExpandArray",
                            "General",
                            _tip)
            obj.ExpandArray = False
            obj.setPropertyStatus('Shape', 'Transient')

    def linkSetup(self, obj):
        """Set up the object as a link object."""
        super(PathArray, self).linkSetup(obj)
        obj.configLinkProperty(ElementCount='Count')

    def execute(self, obj):
        """Execute when the object is created or recomputed."""
        if obj.Base and obj.PathObj:
            pl = obj.Placement  # placement of entire PathArray object
            if obj.PathSubs:
                w = self.getWireFromSubs(obj)
            elif (hasattr(obj.PathObj.Shape, 'Wires')
                  and obj.PathObj.Shape.Wires):
                w = obj.PathObj.Shape.Wires[0]
            elif obj.PathObj.Shape.Edges:
                w = Part.Wire(obj.PathObj.Shape.Edges)
            else:
                _err(obj.PathObj.Name
                     + _tr(", path object doesn't have 'Edges'."))
                return

            if (hasattr(obj, "TangentVector")
                    and obj.AlignMode == "Tangent" and obj.Align):
                basePlacement = obj.Base.Shape.Placement
                baseRotation = basePlacement.Rotation
                stdX = App.Vector(1.0, 0.0, 0.0)  # default TangentVector

                if not DraftVecUtils.equals(stdX, obj.TangentVector):
                    # make rotation from TangentVector to X
                    preRotation = App.Rotation(obj.TangentVector, stdX)
                    netRotation = baseRotation.multiply(preRotation)
                else:
                    netRotation = baseRotation

                base = placements_on_path(netRotation,
                                          w, obj.Count, obj.Xlate,
                                          obj.Align, obj.AlignMode,
                                          obj.ForceVertical,
                                          obj.VerticalVector)
            else:
                base = placements_on_path(obj.Base.Shape.Placement.Rotation,
                                          w, obj.Count, obj.Xlate,
                                          obj.Align, obj.AlignMode,
                                          obj.ForceVertical,
                                          obj.VerticalVector)

            return super(PathArray, self).buildShape(obj, pl, base)

    def getWireFromSubs(self, obj):
        """Make a wire from PathObj subelements."""
        sl = []
        for sub in obj.PathSubs:
            edgeNames = sub[1]
            for n in edgeNames:
                e = sub[0].Shape.getElement(n)
                sl.append(e)
        return Part.Wire(sl)

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored.

        Add properties that don't exist.
        """
        self.migrate_attributes(obj)
        self.set_properties(obj)

        if self.use_link:
            self.linkSetup(obj)
        else:
            obj.setPropertyStatus('Shape', '-Transient')

        if obj.Shape.isNull():
            if getattr(obj, 'PlacementList', None):
                self.buildShape(obj, obj.Placement, obj.PlacementList)
            else:
                self.execute(obj)


_PathArray = PathArray


def placements_on_path(shapeRotation, pathwire, count, xlate, align,
                       mode='Original', forceNormal=False,
                       normalOverride=None):
    """Calculate the placements of a shape along a given path.

    Each copy will be distributed evenly.
    """
    closedpath = DraftGeomUtils.isReallyClosed(pathwire)
    normal = DraftGeomUtils.getNormal(pathwire)

    if forceNormal and normalOverride:
        normal = normalOverride

    path = Part.__sortEdges__(pathwire.Edges)
    ends = []
    cdist = 0

    for e in path:  # find cumulative edge end distance
        cdist += e.Length
        ends.append(cdist)

    placements = []

    # place the start shape
    pt = path[0].Vertexes[0].Point
    _place = calculate_placement(shapeRotation,
                                 path[0], 0, pt, xlate, align, normal,
                                 mode, forceNormal)
    placements.append(_place)

    # closed path doesn't need shape on last vertex
    if not closedpath:
        # place the end shape
        pt = path[-1].Vertexes[-1].Point
        _place = calculate_placement(shapeRotation,
                                     path[-1], path[-1].Length,
                                     pt, xlate, align, normal,
                                     mode, forceNormal)
        placements.append(_place)

    if count < 3:
        return placements

    # place the middle shapes
    if closedpath:
        stop = count
    else:
        stop = count - 1
    step = float(cdist) / stop
    remains = 0
    travel = step
    for i in range(1, stop):
        # which edge in path should contain this shape?
        # avoids problems with float math travel > ends[-1]
        iend = len(ends) - 1

        for j in range(0, len(ends)):
            if travel <= ends[j]:
                iend = j
                break

        # place shape at proper spot on proper edge
        remains = ends[iend] - travel
        offset = path[iend].Length - remains
        pt = path[iend].valueAt(get_parameter_from_v0(path[iend], offset))

        _place = calculate_placement(shapeRotation,
                                     path[iend], offset,
                                     pt, xlate, align, normal,
                                     mode, forceNormal)
        placements.append(_place)

        travel += step

    return placements


calculatePlacementsOnPath = placements_on_path


def calculate_placement(globalRotation,
                        edge, offset, RefPt, xlate, align, normal=None,
                        mode='Original', overrideNormal=False):
    """Orient shape to a local coordinate system (tangent, normal, binormal).

    Orient shape at parameter offset, normally length.

    http://en.wikipedia.org/wiki/Euler_angles (previous version)
    http://en.wikipedia.org/wiki/Quaternions
    """
    # Start with a null Placement so the translation goes to the right place.
    # Then apply the global orientation.
    placement = App.Placement()
    placement.Rotation = globalRotation

    placement.move(RefPt + xlate)
    if not align:
        return placement

    nullv = App.Vector(0, 0, 0)
    defNormal = App.Vector(0.0, 0.0, 1.0)
    if normal:
        defNormal = normal

    try:
        t = edge.tangentAt(get_parameter_from_v0(edge, offset))
        t.normalize()
    except:
        _wrn(_tr("Cannot calculate path tangent. Copy not aligned."))
        return placement

    if mode in ('Original', 'Tangent'):
        if normal is None:
            n = defNormal
        else:
            n = normal
            n.normalize()

        try:
            b = t.cross(n)
            b.normalize()
        except:
            # weird special case, tangent and normal parallel
            b = nullv
            _wrn(_tr("Tangent and normal are parallel. Copy not aligned."))
            return placement

        if overrideNormal:
            priority = "XZY"
            newRot = App.Rotation(t, b, n, priority)  # t/x, b/y, n/z
        else:
            # must follow X, try to follow Z, Y is what it is
            priority = "XZY"
            newRot = App.Rotation(t, n, b, priority)

    elif mode == 'Frenet':
        try:
            n = edge.normalAt(get_parameter_from_v0(edge, offset))
            n.normalize()
        except App.Base.FreeCADError:  # no/infinite normals here
            n = defNormal
            _msg(_tr("Cannot calculate path normal, using default."))

        try:
            b = t.cross(n)
            b.normalize()
        except:
            b = nullv
            _wrn(_tr("Cannot calculate path binormal. Copy not aligned."))
            return placement

        priority = "XZY"
        newRot = App.Rotation(t, n, b, priority)  # t/x, n/y, b/z
    else:
        _msg(_tr("AlignMode {} is not implemented".format(mode)))
        return placement

    # Have valid tangent, normal, binormal
    newGRot = newRot.multiply(globalRotation)

    placement.Rotation = newGRot
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
