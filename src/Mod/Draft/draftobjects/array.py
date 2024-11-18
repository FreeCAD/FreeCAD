# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2019, M G Berberich (berberic2, rynn)                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides the object code for the Array object.

The `Array` class currently handles three types of arrays,
orthogonal, polar, and circular. In the future, probably they should be
split in separate classes so that they are easier to manage.
"""
## @package array
# \ingroup draftobjects
# \brief Provides the object code for the Array object.

## \addtogroup draftobjects
# @{
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils

from draftutils.messages import _wrn
from draftutils.translate import translate

from draftobjects.draftlink import DraftLink


class Array(DraftLink):
    """The Draft Array object.

    To Do
    -----
    The `Array` class currently handles three types of arrays,
    orthogonal, polar, and circular. In the future, probably they should be
    split in separate classes so that they are easier to manage.
    """

    def __init__(self, obj):
        super(Array, self).__init__(obj, "Array")

    def attach(self, obj):
        """Set up the properties when the object is attached."""
        self.set_properties(obj)
        super(Array, self).attach(obj)

    def onDocumentRestored(self, obj):
        super(Array, self).onDocumentRestored(obj)
        if hasattr(obj, "Count"):
            return
        self.update_properties_0v21(obj)

    def update_properties_0v21(self, obj):
        self.set_general_properties(obj)
        self.execute(obj) # Required to update Count to the correct value.
        _wrn("v0.21, " + obj.Label + ", "
             + translate("draft", "added property 'Count'"))

    def set_properties(self, obj):
        """Set properties only if they don't exist."""
        self.set_ortho_properties(obj)
        self.set_polar_circular_properties(obj)
        self.set_polar_properties(obj)
        self.set_circular_properties(obj)

        # The ArrayType property (general) must be attached
        # after the other array properties so that onChanged works properly
        self.set_general_properties(obj)
        self.set_link_properties(obj)

    def set_general_properties(self, obj):
        """Set general properties only if they don't exist."""
        properties = obj.PropertiesList

        if "Base" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The base object that will be duplicated")
            obj.addProperty("App::PropertyLink", "Base", "Objects", _tip)
            obj.Base = None

        if "ArrayType" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The type of array to create.\n"
                                     "- Ortho: places the copies "
                                     "in the direction of the global X, "
                                     "Y, Z axes.\n"
                                     "- Polar: places the copies along "
                                     "a circular arc, up to a specified "
                                     "angle, and with certain orientation "
                                     "defined by a center and an axis.\n"
                                     "- Circular: places the copies "
                                     "in concentric circular layers "
                                     "around the base object.")
            obj.addProperty("App::PropertyEnumeration",
                            "ArrayType",
                            "Objects",
                            _tip)
            obj.ArrayType = ['ortho', 'polar', 'circular']

        if "Fuse" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Specifies if the copies "
                                     "should be fused together "
                                     "if they touch each other (slower)")
            obj.addProperty("App::PropertyBool",
                            "Fuse",
                            "Objects",
                            _tip)
            obj.Fuse = False

        if "Count" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Total number of elements "
                                     "in the array.\n"
                                     "This property is read-only, "
                                     "as the number depends "
                                     "on the parameters of the array.")
            obj.addProperty("App::PropertyInteger",
                            "Count",
                            "Objects",
                            _tip)
            obj.Count = 0
            obj.setEditorMode("Count", 1)  # Read only

    def set_ortho_properties(self, obj):
        """Set orthogonal properties only if they don't exist."""
        properties = obj.PropertiesList

        if "NumberX" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Number of copies in X direction")
            obj.addProperty("App::PropertyInteger",
                            "NumberX",
                            "Orthogonal array",
                            _tip)
            obj.NumberX = 2

        if "NumberY" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Number of copies in Y direction")
            obj.addProperty("App::PropertyInteger",
                            "NumberY",
                            "Orthogonal array",
                            _tip)
            obj.NumberY = 2

        if "NumberZ" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Number of copies in Z direction")
            obj.addProperty("App::PropertyInteger",
                            "NumberZ",
                            "Orthogonal array",
                            _tip)
            obj.NumberZ = 1

        if "IntervalX" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Distance and orientation "
                                     "of intervals in X direction")
            obj.addProperty("App::PropertyVectorDistance",
                            "IntervalX",
                            "Orthogonal array",
                            _tip)
            obj.IntervalX = App.Vector(50, 0, 0)

        if "IntervalY" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Distance and orientation "
                                     "of intervals in Y direction")
            obj.addProperty("App::PropertyVectorDistance",
                            "IntervalY",
                            "Orthogonal array",
                            _tip)
            obj.IntervalY = App.Vector(0, 50, 0)

        if "IntervalZ" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Distance and orientation "
                                     "of intervals in Z direction")
            obj.addProperty("App::PropertyVectorDistance",
                            "IntervalZ",
                            "Orthogonal array",
                            _tip)
            obj.IntervalZ = App.Vector(0, 0, 50)

    def set_polar_circular_properties(self, obj):
        """Set general polar and circular properties if they don't exist."""
        properties = obj.PropertiesList

        if "Axis" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The axis direction around which "
                                     "the elements in a polar or "
                                     "a circular array will be created")
            obj.addProperty("App::PropertyVector",
                            "Axis",
                            "Polar/circular array",
                            _tip)
            obj.Axis = App.Vector(0, 0, 1)

        if "Center" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Center point for polar and "
                                     "circular arrays.\n"
                                     "The 'Axis' passes through this point.")
            obj.addProperty("App::PropertyVectorDistance",
                            "Center",
                            "Polar/circular array",
                            _tip)
            obj.Center = App.Vector(0, 0, 0)

        # The AxisReference property must be attached after Axis and Center
        # so that onChanged works properly
        if "AxisReference" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The axis object that overrides "
                                     "the value of 'Axis' and 'Center', "
                                     "for example, a datum line.\n"
                                     "Its placement, position and rotation, "
                                     "will be used when creating polar "
                                     "and circular arrays.\n"
                                     "Leave this property empty "
                                     "to be able to set 'Axis' and 'Center' "
                                     "manually.")
            obj.addProperty("App::PropertyLinkGlobal",
                            "AxisReference",
                            "Objects",
                            _tip)
            obj.AxisReference = None

    def set_polar_properties(self, obj):
        """Set polar properties only if they don't exist."""
        properties = obj.PropertiesList

        if "NumberPolar" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Number of copies in the polar direction")
            obj.addProperty("App::PropertyInteger",
                            "NumberPolar",
                            "Polar array",
                            _tip)
            obj.NumberPolar = 5

        if "IntervalAxis" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Distance and orientation "
                                     "of intervals in 'Axis' direction")
            obj.addProperty("App::PropertyVectorDistance",
                            "IntervalAxis",
                            "Polar array",
                            _tip)
            obj.IntervalAxis = App.Vector(0, 0, 0)

        if "Angle" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Angle to cover with copies")
            obj.addProperty("App::PropertyAngle",
                            "Angle",
                            "Polar array",
                            _tip)
            obj.Angle = 360

    def set_circular_properties(self, obj):
        """Set circular properties only if they don't exist."""
        properties = obj.PropertiesList

        if "RadialDistance" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Distance between circular layers")
            obj.addProperty("App::PropertyDistance",
                            "RadialDistance",
                            "Circular array",
                            _tip)
            obj.RadialDistance = 50

        if "TangentialDistance" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Distance between copies "
                                     "in the same circular layer")
            obj.addProperty("App::PropertyDistance",
                            "TangentialDistance",
                            "Circular array",
                            _tip)
            obj.TangentialDistance = 25

        if "NumberCircles" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Number of circular layers. "
                                     "The 'Base' object counts as one layer.")
            obj.addProperty("App::PropertyInteger",
                            "NumberCircles",
                            "Circular array",
                            _tip)
            obj.NumberCircles = 3

        if "Symmetry" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "A parameter that determines "
                                     "how many symmetry planes "
                                     "the circular array will have.")
            obj.addProperty("App::PropertyInteger",
                            "Symmetry",
                            "Circular array",
                            _tip)
            obj.Symmetry = 1

    def set_link_properties(self, obj):
        """Set link properties only if they don't exist."""
        properties = obj.PropertiesList

        if self.use_link:
            if "ExpandArray" not in properties:
                _tip = QT_TRANSLATE_NOOP("App::Property",
                                         "Show the individual array elements "
                                         "(only for Link arrays)")
                obj.addProperty("App::PropertyBool",
                                "ExpandArray",
                                "Objects",
                                _tip)
                obj.ExpandArray = False

    def linkSetup(self, obj):
        """Set up the object as a link object."""
        super(Array, self).linkSetup(obj)
        obj.configLinkProperty(ElementCount='Count')
        obj.setPropertyStatus('Count', 'Hidden')

    def onChanged(self, obj, prop):
        """Execute when a property is changed."""
        super(Array, self).onChanged(obj, prop)
        # print(prop, ": ", getattr(obj, prop))
        self.show_and_hide(obj, prop)

    def show_and_hide(self, obj, prop):
        """Show and hide the properties depending on the touched property."""
        if prop == "AxisReference":
            if obj.AxisReference:
                obj.setEditorMode("Center", 1)
                obj.setEditorMode("Axis", 1)
            else:
                obj.setEditorMode("Center", 0)
                obj.setEditorMode("Axis", 0)

        if prop == "ArrayType":
            if obj.ArrayType == "ortho":
                for pr in ("NumberX", "NumberY", "NumberZ",
                           "IntervalX", "IntervalY", "IntervalZ"):
                    obj.setPropertyStatus(pr, "-Hidden")

                for pr in ("Axis", "Center", "NumberPolar", "Angle",
                           "IntervalAxis", "NumberCircles",
                           "RadialDistance", "TangentialDistance",
                           "Symmetry"):
                    obj.setPropertyStatus(pr, "Hidden")

            if obj.ArrayType == "polar":
                for pr in ("Axis", "Center", "NumberPolar",
                           "Angle", "IntervalAxis"):
                    obj.setPropertyStatus(pr, "-Hidden")

                for pr in ("NumberX", "NumberY", "NumberZ",
                           "IntervalX", "IntervalY", "IntervalZ",
                           "NumberCircles", "RadialDistance",
                           "TangentialDistance", "Symmetry"):
                    obj.setPropertyStatus(pr, "Hidden")

            if obj.ArrayType == "circular":
                for pr in ("Axis", "Center", "NumberCircles",
                           "RadialDistance", "TangentialDistance",
                           "Symmetry"):
                    obj.setPropertyStatus(pr, "-Hidden")

                for pr in ("NumberX", "NumberY", "NumberZ",
                           "IntervalX", "IntervalY", "IntervalZ",
                           "NumberPolar", "Angle", "IntervalAxis"):
                    obj.setPropertyStatus(pr, "Hidden")

    def execute(self, obj):
        """Execute when the object is created or recomputed."""
        if self.props_changed_placement_only(obj) \
                or not obj.Base:
            self.props_changed_clear()
            return

        pl = obj.Placement
        axis = obj.Axis
        center = obj.Center

        if hasattr(obj, "AxisReference") and obj.AxisReference:
            if hasattr(obj.AxisReference, "Placement"):
                reference = obj.AxisReference.Placement
                axis = reference.Rotation * App.Vector(0, 0, 1)
                center = reference.Base
            else:
                _info = ("'AxisReference' has no 'Placement' property. "
                         "Please select a different object to use as "
                         "reference.")
                raise TypeError(_info)

        if obj.ArrayType == "ortho":
            pls = rect_placements(obj.Base.Placement,
                                  obj.IntervalX,
                                  obj.IntervalY,
                                  obj.IntervalZ,
                                  obj.NumberX,
                                  obj.NumberY,
                                  obj.NumberZ)
        elif obj.ArrayType == "polar":
            av = obj.IntervalAxis if hasattr(obj, "IntervalAxis") else None
            pls = polar_placements(obj.Base.Placement,
                                   center, obj.Angle.Value,
                                   obj.NumberPolar, axis, av)
        elif obj.ArrayType == "circular":
            pls = circ_placements(obj.Base.Placement,
                                  obj.RadialDistance,
                                  obj.TangentialDistance,
                                  axis, center,
                                  obj.NumberCircles, obj.Symmetry)

        self.buildShape(obj, pl, pls)
        self.props_changed_clear()
        return (not self.use_link)


# Alias for compatibility with v0.18 and earlier
_Array = Array


def rect_placements(base_placement,
                    xvector, yvector, zvector,
                    xnum, ynum, znum):
    """Determine the placements where the rectangular copies will be."""
    pl = base_placement
    placements = [pl.copy()]

    for xcount in range(xnum):
        currentxvector = App.Vector(xvector).multiply(xcount)
        if xcount != 0:
            npl = pl.copy()
            npl.translate(currentxvector)
            placements.append(npl)

        for ycount in range(ynum):
            currentyvector = App.Vector(currentxvector)
            _y_shift = App.Vector(yvector).multiply(ycount)
            currentyvector = currentyvector.add(_y_shift)
            if ycount != 0:
                npl = pl.copy()
                npl.translate(currentyvector)
                placements.append(npl)

            for zcount in range(znum):
                currentzvector = App.Vector(currentyvector)
                _z_shift = App.Vector(zvector).multiply(zcount)
                currentzvector = currentzvector.add(_z_shift)
                if zcount != 0:
                    npl = pl.copy()
                    npl.translate(currentzvector)
                    placements.append(npl)

    return placements


def polar_placements(base_placement,
                     center, angle,
                     number, axis, axisvector):
    """Determine the placements where the polar copies will be."""
    # print("angle ",angle," num ",num)
    placements = [base_placement.copy()]

    if number <= 1:
        return placements

    if angle == 360:
        fraction = float(angle) / number
    else:
        fraction = float(angle) / (number - 1)

    for i in range(number - 1):
        currangle = fraction + (i * fraction)
        npl = base_placement.copy()
        npl.rotate(center, axis, currangle, comp=True)
        if axisvector:
            if not DraftVecUtils.isNull(axisvector):
                npl.translate(App.Vector(axisvector).multiply(i + 1))
        placements.append(npl)

    return placements


def circ_placements(base_placement,
                    r_distance, tan_distance,
                    axis, center,
                    circle_number, symmetry):
    """Determine the placements where the circular copies will be."""
    symmetry = max(1, symmetry)
    lead = (0, 1, 0)

    if axis.x == 0 and axis.z == 0:
        lead = (1, 0, 0)

    direction = axis.cross(App.Vector(lead)).normalize()
    placements = [base_placement.copy()]

    for xcount in range(1, circle_number):
        rc = xcount * r_distance
        trans = App.Vector(direction).multiply(rc)
        c = 2 * rc * math.pi
        n = math.floor(c / tan_distance)
        n = int(math.floor(n / symmetry) * symmetry)
        if n == 0:
            continue

        angle = 360.0 / n
        for ycount in range(0, n):
            npl = base_placement.copy()
            npl.translate(trans)
            npl.rotate(center, axis, ycount * angle, comp=True)
            placements.append(npl)

    return placements

## @}
