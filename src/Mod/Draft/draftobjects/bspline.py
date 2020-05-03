# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""This module provides the object code for Draft BSpline.
"""
## @package bspline
# \ingroup DRAFT
# \brief This module provides the object code for Draft BSpline.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App

from draftutils.utils import get_param

from draftobjects.base import DraftObject


class BSpline(DraftObject):
    """The BSpline object"""

    def __init__(self, obj):
        super(BSpline, self).__init__(obj, "BSpline")

        _tip = "The points of the B-spline"
        obj.addProperty("App::PropertyVectorList","Points",
                        "Draft", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "If the B-spline is closed or not"
        obj.addProperty("App::PropertyBool","Closed",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Create a face if this spline is closed"
        obj.addProperty("App::PropertyBool","MakeFace",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "The area of this object"
        obj.addProperty("App::PropertyArea","Area",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))

        obj.MakeFace = get_param("fillmode",True)
        obj.Closed = False
        obj.Points = []
        self.assureProperties(obj)

    def assureProperties(self, obj): # for Compatibility with older versions
        if not hasattr(obj, "Parameterization"):
            obj.addProperty("App::PropertyFloat","Parameterization","Draft",QT_TRANSLATE_NOOP("App::Property","Parameterization factor"))
            obj.Parameterization = 1.0
            self.knotSeq = []

    def parameterization (self, pts, a, closed):
        """Computes a knot Sequence for a set of points.
        fac (0-1) : parameterization factor
        fac = 0 -> Uniform / fac=0.5 -> Centripetal / fac=1.0 -> Chord-Length
        """
        if closed: # we need to add the first point as the end point
            pts.append(pts[0])
        params = [0]
        for i in range(1,len(pts)):
            p = pts[i].sub(pts[i-1])
            pl = pow(p.Length,a)
            params.append(params[-1] + pl)
        return params

    def onChanged(self, fp, prop):
        if prop == "Parameterization":
            if fp.Parameterization < 0.:
                fp.Parameterization = 0.
            if fp.Parameterization > 1.0:
                fp.Parameterization = 1.0

    def execute(self, obj):
        import Part

        self.assureProperties(obj)

        if not obj.Points:
            obj.positionBySupport()

        self.knotSeq = self.parameterization(obj.Points, obj.Parameterization, obj.Closed)
        plm = obj.Placement
        if obj.Closed and (len(obj.Points) > 2):
            if obj.Points[0] == obj.Points[-1]:  # should not occur, but OCC will crash
                _err = "_BSpline.createGeometry: \
                        Closed with same first/last Point. Geometry not updated."
                App.Console.PrintError(QT_TRANSLATE_NOOP('Draft', _err)+"\n")
                return
            spline = Part.BSplineCurve()
            spline.interpolate(obj.Points, PeriodicFlag = True, Parameters = self.knotSeq)
            # DNC: bug fix: convert to face if closed
            shape = Part.Wire(spline.toShape())
            # Creating a face from a closed spline cannot be expected to always work
            # Usually, if the spline is not flat the call of Part.Face() fails
            try:
                if hasattr(obj,"MakeFace"):
                    if obj.MakeFace:
                        shape = Part.Face(shape)
                else:
                    shape = Part.Face(shape)
            except Part.OCCError:
                pass
            obj.Shape = shape
            if hasattr(obj,"Area") and hasattr(shape,"Area"):
                obj.Area = shape.Area
        else:
            spline = Part.BSplineCurve()
            spline.interpolate(obj.Points, PeriodicFlag = False, Parameters = self.knotSeq)
            shape = spline.toShape()
            obj.Shape = shape
            if hasattr(obj,"Area") and hasattr(shape,"Area"):
                obj.Area = shape.Area
        obj.Placement = plm
        obj.positionBySupport()


_BSpline = BSpline