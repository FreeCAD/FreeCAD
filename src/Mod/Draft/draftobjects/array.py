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
"""Provides the object code for the Draft Array object.

The `Array` class currently handles three types of arrays,
orthogonal, polar, and circular. In the future, probably they should be
split in separate classes so that they are easier to manage.
"""
## @package array
# \ingroup DRAFT
# \brief Provides the object code for the Draft Array object.

import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils

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
        _tip = "The base object that must be duplicated"
        obj.addProperty("App::PropertyLink",
                        "Base",
                        "Objects",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "The type of array to create"
        obj.addProperty("App::PropertyEnumeration",
                        "ArrayType",
                        "Draft",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "The axis (e.g. DatumLine) overriding Axis/Center"
        obj.addProperty("App::PropertyLinkGlobal",
                        "AxisReference",
                        "Objects",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "The axis direction"
        obj.addProperty("App::PropertyVector",
                        "Axis",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Number of copies in X direction"
        obj.addProperty("App::PropertyInteger",
                        "NumberX",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Number of copies in Y direction"
        obj.addProperty("App::PropertyInteger",
                        "NumberY",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Number of copies in Z direction"
        obj.addProperty("App::PropertyInteger",
                        "NumberZ",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Number of copies"
        obj.addProperty("App::PropertyInteger",
                        "NumberPolar",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Distance and orientation of intervals in X direction"
        obj.addProperty("App::PropertyVectorDistance",
                        "IntervalX",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Distance and orientation of intervals in Y direction"
        obj.addProperty("App::PropertyVectorDistance",
                        "IntervalY",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Distance and orientation of intervals in Z direction"
        obj.addProperty("App::PropertyVectorDistance",
                        "IntervalZ",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Distance and orientation of intervals in Axis direction"
        obj.addProperty("App::PropertyVectorDistance",
                        "IntervalAxis",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Center point"
        obj.addProperty("App::PropertyVectorDistance",
                        "Center",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Angle to cover with copies"
        obj.addProperty("App::PropertyAngle",
                        "Angle",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Distance between copies in a circle"
        obj.addProperty("App::PropertyDistance",
                        "RadialDistance",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Distance between circles"
        obj.addProperty("App::PropertyDistance",
                        "TangentialDistance",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "number of circles"
        obj.addProperty("App::PropertyInteger",
                        "NumberCircles",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "number of circles"
        obj.addProperty("App::PropertyInteger",
                        "Symmetry",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Specifies if copies must be fused (slower)"
        obj.addProperty("App::PropertyBool",
                        "Fuse",
                        "Parameters",
                        QT_TRANSLATE_NOOP("App::Property", _tip))
        obj.Fuse = False

        if self.use_link:
            _tip = "Number of elements in the array, read only"
            obj.addProperty("App::PropertyInteger",
                            "Count",
                            "Draft",
                            QT_TRANSLATE_NOOP("App::Property", _tip))

            _tip = ("Show the individual array elements "
                    "(only for Link arrays)")
            obj.addProperty("App::PropertyBool",
                            "ExpandArray",
                            "Draft",
                            QT_TRANSLATE_NOOP("App::Property", _tip))
            obj.ExpandArray = False

        obj.ArrayType = ['ortho', 'polar', 'circular']
        obj.NumberX = 1
        obj.NumberY = 1
        obj.NumberZ = 1
        obj.NumberPolar = 1
        obj.IntervalX = App.Vector(1, 0, 0)
        obj.IntervalY = App.Vector(0, 1, 0)
        obj.IntervalZ = App.Vector(0, 0, 1)
        obj.Angle = 360
        obj.Axis = App.Vector(0, 0, 1)
        obj.RadialDistance = 1.0
        obj.TangentialDistance = 1.0
        obj.NumberCircles = 2
        obj.Symmetry = 1

        super(Array, self).attach(obj)

    def linkSetup(self, obj):
        """Set up the object as a link object."""
        super(Array, self).linkSetup(obj)
        obj.configLinkProperty(ElementCount='Count')
        obj.setPropertyStatus('Count', 'Hidden')

    def onChanged(self, obj, prop):
        """Execute when a property is changed."""
        super(Array, self).onChanged(obj, prop)
        if prop == "AxisReference":
            if obj.AxisReference:
                obj.setEditorMode("Center", 1)
                obj.setEditorMode("Axis", 1)
            else:
                obj.setEditorMode("Center", 0)
                obj.setEditorMode("Axis", 0)

    def execute(self, obj):
        """Execture when the object is created or recomputed."""
        if obj.Base:
            pl = obj.Placement
            axis = obj.Axis
            center = obj.Center

            if hasattr(obj, "AxisReference") and obj.AxisReference:
                if hasattr(obj.AxisReference, "Placement"):
                    axis = obj.AxisReference.Placement.Rotation * App.Vector(0, 0, 1)
                    center = obj.AxisReference.Placement.Base
                else:
                    _info = ("'AxisReference' has no 'Placement' property. "
                             "Please select a different object to use as "
                             "reference.")
                    raise TypeError(_info)

            if obj.ArrayType == "ortho":
                pls = self.rectArray(obj.Base.Placement,
                                     obj.IntervalX,
                                     obj.IntervalY,
                                     obj.IntervalZ,
                                     obj.NumberX,
                                     obj.NumberY,
                                     obj.NumberZ)
            elif obj.ArrayType == "polar":
                av = obj.IntervalAxis if hasattr(obj, "IntervalAxis") else None
                pls = self.polarArray(obj.Base.Placement,
                                      center, obj.Angle.Value,
                                      obj.NumberPolar, axis, av)
            elif obj.ArrayType == "circular":
                pls = self.circArray(obj.Base.Placement,
                                     obj.RadialDistance,
                                     obj.TangentialDistance,
                                     axis, center,
                                     obj.NumberCircles, obj.Symmetry)

            return super(Array, self).buildShape(obj, pl, pls)

    def rectArray(self, pl, xvector, yvector, zvector, xnum, ynum, znum):
        """Determine the placements where the rectangular copies will be."""
        base = [pl.copy()]
        for xcount in range(xnum):
            currentxvector = App.Vector(xvector).multiply(xcount)
            if not xcount == 0:
                npl = pl.copy()
                npl.translate(currentxvector)
                base.append(npl)
            for ycount in range(ynum):
                currentyvector = App.Vector(currentxvector)
                currentyvector = currentyvector.add(App.Vector(yvector).multiply(ycount))
                if not ycount == 0:
                    npl = pl.copy()
                    npl.translate(currentyvector)
                    base.append(npl)
                for zcount in range(znum):
                    currentzvector = App.Vector(currentyvector)
                    currentzvector = currentzvector.add(App.Vector(zvector).multiply(zcount))
                    if not zcount == 0:
                        npl = pl.copy()
                        npl.translate(currentzvector)
                        base.append(npl)
        return base

    def circArray(self, pl, rdist, tdist, axis, center, cnum, sym):
        """Determine the placements where the circular copies will be."""
        sym = max(1, sym)
        lead = (0, 1, 0)
        if axis.x == 0 and axis.z == 0:
            lead = (1, 0, 0)
        direction = axis.cross(App.Vector(lead)).normalize()
        base = [pl.copy()]
        for xcount in range(1, cnum):
            rc = xcount * rdist
            c = 2 * rc * math.pi
            n = math.floor(c / tdist)
            n = int(math.floor(n / sym) * sym)
            if n == 0:
                continue
            angle = 360.0/n
            for ycount in range(0, n):
                npl = pl.copy()
                trans = App.Vector(direction).multiply(rc)
                npl.translate(trans)
                npl.rotate(npl.Rotation.inverted().multVec(center-trans),
                           axis, ycount * angle)
                base.append(npl)
        return base

    def polarArray(self, spl, center, angle, num, axis, axisvector):
        """Determine the placements where the polar copies will be."""
        # print("angle ",angle," num ",num)
        spin = App.Placement(App.Vector(), spl.Rotation)
        pl = App.Placement(spl.Base, App.Rotation())
        center = center.sub(spl.Base)
        base = [spl.copy()]
        if angle == 360:
            fraction = float(angle)/num
        else:
            if num == 0:
                return base
            fraction = float(angle)/(num-1)
        ctr = DraftVecUtils.tup(center)
        axs = DraftVecUtils.tup(axis)
        for i in range(num-1):
            currangle = fraction + (i*fraction)
            npl = pl.copy()
            npl.rotate(ctr, axs, currangle)
            npl = npl.multiply(spin)
            if axisvector:
                if not DraftVecUtils.isNull(axisvector):
                    npl.translate(App.Vector(axisvector).multiply(i+1))
            base.append(npl)
        return base


_Array = Array
