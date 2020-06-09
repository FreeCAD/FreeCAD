# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""This module provides the object code for Draft Dimension."""
## @package dimension
# \ingroup DRAFT
# \brief This module provides the object code for Draft Dimension.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils
import DraftGeomUtils
import draftutils.utils as utils

from draftobjects.draft_annotation import DraftAnnotation


class DimensionBase(DraftAnnotation):
    """The base objects for dimension objects.

    This class inherits `DraftAnnotation` to define the basic properties
    of all annotation type objects, like a scaling multiplier.

    This class is not used directly, but inherited by all dimension
    objects.
    """

    def __init__(self, obj, tp="Dimension"):
        super(DimensionBase, self).__init__(obj, tp)

        # Draft
        _tip = QT_TRANSLATE_NOOP("App::Property", "The normal direction of this dimension")
        obj.addProperty("App::PropertyVector", "Normal", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "The object measured by this dimension")
        obj.addProperty("App::PropertyLink", "Support", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "The geometry this dimension is linked to")
        obj.addProperty("App::PropertyLinkSubList", "LinkedGeometry", "Draft", _tip)
                                          
        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Point on which the dimension\nline is placed.")
        obj.addProperty("App::PropertyVectorDistance", "Dimline", "Draft", _tip)
                                          
        obj.Dimline = App.Vector(0,1,0)
        obj.Normal = App.Vector(0,0,1)

    def onDocumentRestored(self, obj):
        super(DimensionBase, self).onDocumentRestored(obj)


class LinearDimension(DimensionBase):
    """The linear dimension object.

    This inherits `DimensionBase` to provide the basic functionality of
    a dimension.
    """

    def __init__(self, obj):

        super(LinearDimension, self).__init__(obj, "LinearDimension")

        obj.Proxy = self

        self.init_properties(obj)


    def init_properties(self, obj):
        """Add Linear Dimension specific properties to the object and set them"""

        # Draft
        _tip = QT_TRANSLATE_NOOP("App::Property", "Startpoint of dimension")
        obj.addProperty("App::PropertyVectorDistance", "Start", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Endpoint of dimension")
        obj.addProperty("App::PropertyVectorDistance", "End", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "The normal direction of this dimension")
        obj.addProperty("App::PropertyVector", "Direction", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "The measurement of this dimension")
        obj.addProperty("App::PropertyLength", "Distance", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "For arc/circle measurements, false = radius, true = diameter")
        obj.addProperty("App::PropertyBool", "Diameter", "Draft", _tip)

        obj.Start = App.Vector(0,0,0)
        obj.End = App.Vector(1,0,0)

    def onDocumentRestored(self, obj):
        super(LinearDimension, self).onDocumentRestored(obj)

    def onChanged(self,obj,prop):
        '''Do something when a property has changed'''
        if hasattr(obj, "Distance"):
            obj.setEditorMode('Distance', 1)
        #if hasattr(obj,"Normal"):
        #    obj.setEditorMode('Normal', 2)
        if hasattr(obj, "Support"):
            obj.setEditorMode('Support', 2)

    def execute(self, obj):
        """ Set start point and end point according to the linked geometry"""
        if obj.LinkedGeometry:
            if len(obj.LinkedGeometry) == 1:
                lobj = obj.LinkedGeometry[0][0]
                lsub = obj.LinkedGeometry[0][1]
                if len(lsub) == 1:
                    if "Edge" in lsub[0]:
                        n = int(lsub[0][4:])-1
                        edge = lobj.Shape.Edges[n]
                        if DraftGeomUtils.geomType(edge) == "Line":
                            obj.Start = edge.Vertexes[0].Point
                            obj.End = edge.Vertexes[-1].Point
                        elif DraftGeomUtils.geomType(edge) == "Circle":
                            c = edge.Curve.Center
                            r = edge.Curve.Radius
                            a = edge.Curve.Axis
                            ray = obj.Dimline.sub(c).projectToPlane(App.Vector(0,0,0),a)
                            if (ray.Length == 0):
                                ray = a.cross(App.Vector(1,0,0))
                                if (ray.Length == 0):
                                    ray = a.cross(App.Vector(0,1,0))
                            ray = DraftVecUtils.scaleTo(ray,r)
                            if hasattr(obj,"Diameter"):
                                if obj.Diameter:
                                    obj.Start = c.add(ray.negative())
                                    obj.End = c.add(ray)
                                else:
                                    obj.Start = c
                                    obj.End = c.add(ray)
                elif len(lsub) == 2:
                    if ("Vertex" in lsub[0]) and ("Vertex" in lsub[1]):
                        n1 = int(lsub[0][6:])-1
                        n2 = int(lsub[1][6:])-1
                        obj.Start = lobj.Shape.Vertexes[n1].Point
                        obj.End = lobj.Shape.Vertexes[n2].Point
            elif len(obj.LinkedGeometry) == 2:
                lobj1 = obj.LinkedGeometry[0][0]
                lobj2 = obj.LinkedGeometry[1][0]
                lsub1 = obj.LinkedGeometry[0][1]
                lsub2 = obj.LinkedGeometry[1][1]
                if (len(lsub1) == 1) and (len(lsub2) == 1):
                    if ("Vertex" in lsub1[0]) and ("Vertex" in lsub2[1]):
                        n1 = int(lsub1[0][6:])-1
                        n2 = int(lsub2[0][6:])-1
                        obj.Start = lobj1.Shape.Vertexes[n1].Point
                        obj.End = lobj2.Shape.Vertexes[n2].Point
        # set the distance property
        total_len = (obj.Start.sub(obj.End)).Length
        if round(obj.Distance.Value, utils.precision()) != round(total_len, utils.precision()):
            obj.Distance = total_len
        if App.GuiUp:
            if obj.ViewObject:
                obj.ViewObject.update()


# Alias for compatibility with v0.18 and earlier
_Dimension = LinearDimension


class AngularDimension(DimensionBase):
    """The angular dimension object.

    This inherits `DimensionBase` to provide the basic functionality of
    a dimension.
    """

    def __init__(self, obj):
        super(AngularDimension, self).__init__(obj, "AngularDimension")
        self.init_properties(obj)
        obj.Proxy = self

    def init_properties(self, obj):
        """Add Angular Dimension specific properties to the object and set them"""

        _tip = QT_TRANSLATE_NOOP("App::Property","Start angle of the dimension")
        obj.addProperty("App::PropertyAngle", "FirstAngle", "Draft", )

        _tip = QT_TRANSLATE_NOOP("App::Property","End angle of the dimension")
        obj.addProperty("App::PropertyAngle", "LastAngle", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "The center point of this dimension")
        obj.addProperty("App::PropertyVectorDistance", "Center", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "The measurement of this dimension")
        obj.addProperty("App::PropertyAngle", "Angle", "Draft", _tip)

        obj.FirstAngle = 0
        obj.LastAngle = 90
        obj.Dimline = App.Vector(0,1,0)
        obj.Center = App.Vector(0,0,0)
        obj.Normal = App.Vector(0,0,1)

    def onDocumentRestored(self, obj):
        super(AngularDimension, self).onDocumentRestored(obj)

    def execute(self, fp):
        '''Do something when recompute object'''
        if fp.ViewObject:
            fp.ViewObject.update()

    def onChanged(self,obj,prop):
        '''Do something when a property has changed'''
        super(AngularDimension, self).onChanged(obj, prop)
        if hasattr(obj,"Angle"):
            obj.setEditorMode('Angle',1)
        if hasattr(obj,"Normal"):
            obj.setEditorMode('Normal',2)
        if hasattr(obj,"Support"):
            obj.setEditorMode('Support',2)


# Alias for compatibility with v0.18 and earlier
_AngularDimension = AngularDimension
