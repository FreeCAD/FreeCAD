# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
# *   Copyright (c) 2023 FreeCAD Project Association                        *
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
        obj.addProperty("App::PropertyLinkSubList", "Faces", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property","Specifies if splitter lines must be removed")
        obj.addProperty("App::PropertyBool","RemoveSplitter", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property","An optional extrusion value to be applied to all faces")
        obj.addProperty("App::PropertyDistance","Extrusion", "Draft" , _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property","An optional offset value to be applied to all faces")
        obj.addProperty("App::PropertyDistance","Offset", "Draft" , _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property","This specifies if the shapes sew")
        obj.addProperty("App::PropertyBool","Sew", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property","The area of the faces of this Facebinder")
        obj.addProperty("App::PropertyArea","Area", "Draft", _tip)
        obj.setEditorMode("Area",1)

    def execute(self, obj):
        if self.props_changed_placement_only(obj):
            self.props_changed_clear()
            return

        import Part
        pl = obj.Placement
        if not obj.Faces:
            return
        faces = []
        area = 0
        offs_val = obj.Offset.Value if hasattr(obj, "Offset") else 0
        extr_val = obj.Extrusion.Value if hasattr(obj, "Extrusion") else 0
        for sel in obj.Faces:
            for sub in sel[1]:
                if "Face" in sub:
                    try:
                        face = Part.getShape(sel[0], sub, needSubElement=True, retType=0)
                        area += face.Area
                        if offs_val:
                            if face.Surface.isPlanar():
                                norm = face.normalAt(0, 0)
                                dist = norm.multiply(offs_val)
                                face.translate(dist)
                                faces.append(face)
                            else:
                                offs = face.makeOffsetShape(offs_val, 1e-7)
                                faces.extend(offs.Faces)
                        else:
                            faces.append(face)
                    except Part.OCCError:
                        print("Draft: error building facebinder")
                        return
        if not faces:
            return
        try:
            if extr_val:
                extrs = []
                for face in faces:
                    if face.Surface.isPlanar():
                        extr = face.extrude(face.normalAt(0, 0).multiply(extr_val))
                        extrs.append(extr)
                    else:
                        extr = face.makeOffsetShape(extr_val, 1e-7, fill=True)
                        extrs.extend(extr.Solids)
                sh = extrs.pop()
                if extrs:
                    sh = sh.multiFuse(extrs)
            else:
                sh = faces.pop()
                if faces:
                    sh = sh.multiFuse(faces)
            if len(faces) > 1:
                if hasattr(obj, "Sew") and obj.Sew:
                    sh.sewShape()
                if not hasattr(obj, "RemoveSplitter"):
                    sh = sh.removeSplitter()
                elif obj.RemoveSplitter:
                    sh = sh.removeSplitter()
        except Part.OCCError:
            print("Draft: error building facebinder")
            return
        obj.Shape = sh
        obj.Placement = pl
        obj.Area = area
        self.props_changed_clear()

    def onChanged(self, obj, prop):
        self.props_changed_store(prop)

    def addSubobjects(self, obj, facelinks):
        """adds facelinks to this facebinder"""
        # facelinks is an iterable or a selection set:
        # [(<Part::PartFeature>, ("3.Face3", "3.Face6"))]
        # or:
        # Gui.Selection.getSelectionEx("", 0)
        sels = obj.Faces
        for sel in facelinks:
            if isinstance(sel, list) or isinstance(sel, tuple):
                sel_obj, sel_subs = sel
            else:
                sel_obj = sel.Object
                sel_subs = sel.SubElementNames
            if sel_obj.Name != obj.Name:
                for sub in sel_subs:
                    if "Face" in sub:
                        sels.append((sel_obj, sub))
        obj.Faces = sels
        self.execute(obj)


# Alias for compatibility with v0.18 and earlier
_Facebinder = Facebinder

## @}
