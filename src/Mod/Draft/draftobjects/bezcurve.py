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
"""Provides the object code for the BezCurve object."""
## @package bezcurve
# \ingroup draftobjects
# \brief Provides the object code for the BezCurve object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import draftutils.utils as utils

from draftobjects.base import DraftObject


class BezCurve(DraftObject):
    """The BezCurve object"""

    def __init__(self, obj):
        super(BezCurve, self).__init__(obj, "BezCurve")

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The points of the Bezier curve")
        obj.addProperty("App::PropertyVectorList", "Points", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The degree of the Bezier function")
        obj.addProperty("App::PropertyInteger", "Degree", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Continuity")
        obj.addProperty("App::PropertyIntegerList", "Continuity", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "If the Bezier curve should be closed or not")
        obj.addProperty("App::PropertyBool", "Closed", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Create a face if this curve is closed")
        obj.addProperty("App::PropertyBool", "MakeFace", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The length of this object")
        obj.addProperty("App::PropertyLength", "Length", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The area of this object")
        obj.addProperty("App::PropertyArea", "Area", "Draft", _tip)

        obj.MakeFace = utils.get_param("fillmode", True)
        obj.Closed = False
        obj.Degree = 3
        obj.Continuity = []
        #obj.setEditorMode("Degree", 2)
        obj.setEditorMode("Continuity", 1)

    def execute(self, fp):
        if self.props_changed_placement_only():
            fp.positionBySupport()
            self.props_changed_clear()
            return

        self.createGeometry(fp)
        fp.positionBySupport()
        self.props_changed_clear()

    def _segpoleslst(self,fp):
        """Split the points into segments."""
        if not fp.Closed and len(fp.Points) >= 2: #allow lower degree segment
            poles=fp.Points[1:]
        elif fp.Closed and len(fp.Points) >= fp.Degree: #drawable
            #poles=fp.Points[1:(fp.Degree*(len(fp.Points)//fp.Degree))]+fp.Points[0:1]
            poles=fp.Points[1:]+fp.Points[0:1]
        else:
            poles=[]
        return [poles[x:x+fp.Degree] for x in \
            range(0, len(poles), (fp.Degree or 1))]

    def resetcontinuity(self,fp):
        fp.Continuity = [0]*(len(self._segpoleslst(fp))-1+1*fp.Closed)
        #nump= len(fp.Points)-1+fp.Closed*1
        #numsegments = (nump // fp.Degree) + 1 * (nump % fp.Degree > 0) -1
        #fp.Continuity = [0]*numsegments

    def onChanged(self, fp, prop):
        self.props_changed_store(prop)

        if prop == 'Closed':
            # if remove the last entry when curve gets opened
            oldlen = len(fp.Continuity)
            newlen = (len(self._segpoleslst(fp))-1+1*fp.Closed)
            if oldlen > newlen:
                fp.Continuity = fp.Continuity[:newlen]
            if oldlen < newlen:
                fp.Continuity = fp.Continuity + [0]*(newlen-oldlen)

        if (hasattr(fp,'Closed') and
                fp.Closed and
                prop in  ['Points','Degree','Closed'] and
                len(fp.Points) % fp.Degree):
            # the curve editing tools can't handle extra points
            fp.Points=fp.Points[:(fp.Degree*(len(fp.Points)//fp.Degree))]
            #for closed curves

        if prop in ["Degree"] and fp.Degree >= 1:
            self.resetcontinuity(fp)

        if prop in ["Points","Degree","Continuity","Closed"]:
            self.createGeometry(fp)

    def createGeometry(self,fp):
        import Part
        plm = fp.Placement
        if fp.Points:
            startpoint = fp.Points[0]
            edges = []
            for segpoles in self._segpoleslst(fp):
#               if len(segpoles) == fp.Degree # would skip additional poles
                c = Part.BezierCurve() #last segment may have lower degree
                c.increase(len(segpoles))
                c.setPoles([startpoint]+segpoles)
                edges.append(Part.Edge(c))
                startpoint = segpoles[-1]
            w = Part.Wire(edges)
            if fp.Closed and w.isClosed():
                try:
                    if hasattr(fp,"MakeFace"):
                        if fp.MakeFace:
                            w = Part.Face(w)
                    else:
                        w = Part.Face(w)
                except Part.OCCError:
                    pass
            fp.Shape = w
            if hasattr(fp,"Area") and hasattr(w,"Area"):
                fp.Area = w.Area
            if hasattr(fp,"Length") and hasattr(w,"Length"):
                fp.Length = w.Length
        fp.Placement = plm

    @classmethod
    def symmetricpoles(cls,knot, p1, p2):
        """Make two poles symmetric respective to the knot."""
        p1h = App.Vector(p1)
        p2h = App.Vector(p2)
        p1h.multiply(0.5)
        p2h.multiply(0.5)
        return ( knot+p1h-p2h , knot+p2h-p1h )

    @classmethod
    def tangentpoles(cls,knot, p1, p2,allowsameside=False):
        """Make two poles have the same tangent at knot."""
        p12n = p2.sub(p1)
        p12n.normalize()
        p1k = knot-p1
        p2k = knot-p2
        p1k_= App.Vector(p12n)
        kon12=(p1k * p12n)
        if allowsameside or not (kon12 < 0 or p2k * p12n > 0):# instead of moving
            p1k_.multiply(kon12)
            pk_k = knot - p1 - p1k_
            return (p1 + pk_k, p2 + pk_k)
        else:
            return cls.symmetricpoles(knot, p1, p2)

    @staticmethod
    def modifysymmetricpole(knot,p1):
        """calculate the coordinates of the opposite pole
        of a symmetric knot"""
        return knot + knot - p1

    @staticmethod
    def modifytangentpole(knot,p1,oldp2):
        """calculate the coordinates of the opposite pole
        of a tangent knot"""
        pn = knot - p1
        pn.normalize()
        pn.multiply((knot - oldp2).Length)
        return pn + knot


# Alias for compatibility with v0.18 and earlier
_BezCurve = BezCurve

## @}
