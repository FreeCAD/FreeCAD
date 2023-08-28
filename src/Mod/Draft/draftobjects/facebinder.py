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
"""Provides the object code for the Facebinder object."""
## @package facebinder
# \ingroup draftobjects
# \brief Provides the object code for the Facebinder object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App

from draftobjects.base import DraftObject


class Facebinder(DraftObject):
    """The Draft Facebinder object"""
    def __init__(self,obj):
        super(Facebinder, self).__init__(obj, "Facebinder")

        _tip = QT_TRANSLATE_NOOP("App::Property","Linked faces")
        obj.addProperty("App::PropertyLinkSubList", "Faces", "Draft",  _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property","Specifies if splitter lines must be removed")
        obj.addProperty("App::PropertyBool","RemoveSplitter", "Draft",  _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property","An optional extrusion value to be applied to all faces")
        obj.addProperty("App::PropertyDistance","Extrusion", "Draft" , _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property","An optional offset value to be applied to all faces")
        obj.addProperty("App::PropertyDistance","Offset", "Draft" , _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property","This specifies if the shapes sew")
        obj.addProperty("App::PropertyBool","Sew", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property","The area of the faces of this Facebinder")
        obj.addProperty("App::PropertyArea","Area", "Draft", _tip)
        obj.setEditorMode("Area",1)

    def execute(self,obj):
        if self.props_changed_placement_only(obj):
            self.props_changed_clear()
            return

        import Part
        pl = obj.Placement
        if not obj.Faces:
            return
        faces = []
        area = 0
        for sel in obj.Faces:
            for f in sel[1]:
                if "Face" in f:
                    try:
                        fnum = int(f[4:])-1
                        face = sel[0].Shape.Faces[fnum]
                        if hasattr(obj,"Offset") and obj.Offset.Value:
                            norm = face.normalAt(0,0)
                            dist = norm.multiply(obj.Offset.Value)
                            face.translate(dist)
                        faces.append(face)
                        area += face.Area
                    except(IndexError, Part.OCCError):
                        print("Draft: wrong face index")
                        return
        if not faces:
            return
        try:
            if len(faces) > 1:
                sh = None
                if hasattr(obj, "Extrusion"):
                    if obj.Extrusion.Value:
                        for f in faces:
                            f = f.extrude(f.normalAt(0,0).multiply(obj.Extrusion.Value))
                            if sh:
                                sh = sh.fuse(f)
                            else:
                                sh = f
                if not sh:
                    sh = faces.pop()
                    sh = sh.multiFuse(faces)
                if hasattr(obj, "Sew"):
                    if obj.Sew:
                        sh = sh.copy()
                        sh.sewShape()
                if hasattr(obj, "RemoveSplitter"):
                    if obj.RemoveSplitter:
                        sh = sh.removeSplitter()
                else:
                    sh = sh.removeSplitter()
            else:
                sh = faces[0]
                if hasattr(obj, "Extrusion"):
                    if obj.Extrusion.Value:
                        sh = sh.extrude(sh.normalAt(0,0).multiply(obj.Extrusion.Value))
                sh.transformShape(sh.Matrix, True)
        except Part.OCCError:
            print("Draft: error building facebinder")
            return
        obj.Shape = sh
        obj.Placement = pl
        obj.Area = area
        self.props_changed_clear()

    def onChanged(self, obj, prop):
        self.props_changed_store(prop)

    def addSubobjects(self,obj,facelinks):
        """adds facelinks to this facebinder"""
        objs = obj.Faces
        for o in facelinks:
            if isinstance(o, tuple) or isinstance(o, list):
                if o[0].Name != obj.Name:
                    objs.append(tuple(o))
            else:
                for el in o.SubElementNames:
                    if "Face" in el:
                        if o.Object.Name != obj.Name:
                            objs.append((o.Object, el))
        obj.Faces = objs
        self.execute(obj)


# Alias for compatibility with v0.18 and earlier
_Facebinder = Facebinder

## @}
