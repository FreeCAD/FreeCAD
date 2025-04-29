# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
# *   Copyright (c) 2023-2025 FreeCAD Project Association                   *
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
from draftgeoutils import geometry
from draftobjects.base import DraftObject
from draftutils import gui_utils
from draftutils.messages import _err, _msg, _wrn
from draftutils.translate import translate


class Facebinder(DraftObject):
    """The Draft Facebinder object"""
    def __init__(self,obj):
        super().__init__(obj, "Facebinder")

        _tip = QT_TRANSLATE_NOOP("App::Property","Linked faces")
        obj.addProperty("App::PropertyLinkSubList", "Faces", "Draft", _tip, locked=True)

        _tip = QT_TRANSLATE_NOOP("App::Property","Specifies if splitter lines must be removed")
        obj.addProperty("App::PropertyBool","RemoveSplitter", "Draft", _tip, locked=True)

        _tip = QT_TRANSLATE_NOOP("App::Property","An optional extrusion value to be applied to all faces")
        obj.addProperty("App::PropertyDistance","Extrusion", "Draft" , _tip, locked=True)

        _tip = QT_TRANSLATE_NOOP("App::Property","An optional offset value to be applied to all faces")
        obj.addProperty("App::PropertyDistance","Offset", "Draft" , _tip, locked=True)

        _tip = QT_TRANSLATE_NOOP("App::Property","This specifies if the shapes sew")
        obj.addProperty("App::PropertyBool","Sew", "Draft", _tip, locked=True)

        _tip = QT_TRANSLATE_NOOP("App::Property","The area of the faces of this Facebinder")
        obj.addProperty("App::PropertyArea","Area", "Draft", _tip, locked=True)
        obj.setEditorMode("Area", 1)

    def onDocumentRestored(self, obj):
        super().onDocumentRestored(obj)
        gui_utils.restore_view_object(
            obj, vp_module="view_facebinder", vp_class="ViewProviderFacebinder", format=False
        )

    def execute(self, obj):
        if self.props_changed_placement_only(obj):
            self.props_changed_clear()
            return

        if not obj.Faces:
            self._report_face_error(obj)
            return

        import Part
        faces = []
        try:
            for sel in obj.Faces:
                for sub in sel[1]:
                    if "Face" in sub:
                        face = Part.getShape(sel[0], sub, needSubElement=True, retType=0)
                        faces.append(face)
        except Part.OCCError:
            self._report_face_error(obj)
            return

        if not faces:
            self._report_face_error(obj)
            return

        obj_sew = getattr(obj, "Sew", True)
        try:
            shp, area = self._build_shape(obj, faces, sew=obj_sew)
        except Exception:
            if not obj_sew:
                self._report_build_error(obj)
                return
            self._report_sew_error(obj)
            try:
                shp, area = self._build_shape(obj, faces, sew=False)
            except Exception:
                self._report_build_error(obj)
                return
        if not shp.isValid():
            if not obj_sew:
                self._report_build_error(obj)
                return
            self._report_sew_error(obj)
            try:
                shp, area = self._build_shape(obj, faces, sew=False)
            except Exception:
                self._report_build_error(obj)
                return

        if shp.ShapeType == "Compound":
            obj.Shape = shp
        else:
            obj.Shape = Part.Compound([shp])  # nest in compound to ensure default Placement
        obj.Area = area
        self.props_changed_clear()

    def _report_build_error(self, obj):
        _err(obj.Label + ": " + translate("draft", "Unable to build Facebinder"))

    def _report_face_error(self, obj):
        _wrn(obj.Label + ": " + translate("draft", "No valid faces for Facebinder"))

    def _report_sew_error(self, obj):
        _wrn(obj.Label + ": " + translate("draft", "Unable to build Facebinder, resuming with Sew disabled"))

    def _build_shape(self, obj, faces, sew=False):
        """returns the built shape and the area of the offset faces"""
        import Part
        offs_val = getattr(obj, "Offset", 0)
        extr_val = getattr(obj, "Extrusion", 0)

        shp = Part.Compound(faces)
        # Sew before offsetting to ensure corners stay connected:
        if sew:
            shp.sewShape()
            if shp.ShapeType != "Compound":
                shp = Part.Compound([shp])

        if offs_val:
            offsets = []
            for sub in shp.SubShapes:
                offsets.append(sub.makeOffsetShape(offs_val, 1e-7, join=2))
            shp = Part.Compound(offsets)

        area = shp.Area  # take area after offsetting original faces, but before extruding

        if extr_val:
            extrudes = []
            for sub in shp.SubShapes:
                ext = sub.makeOffsetShape(extr_val, 1e-7, inter=True, join=2, fill=True)
                extrudes.append(self._convert_to_planar(obj, ext))
            shp = Part.Compound(extrudes)

        subs = shp.SubShapes
        shp = subs.pop()
        if subs:
            shp = shp.fuse(subs)

        if len(shp.Faces) > 1:
            if getattr(obj, "RemoveSplitter", True):
                shp = shp.removeSplitter()

        return shp, area

    def _convert_to_planar(self, obj, shp):
        """convert flat B-spline faces to planar faces if possible"""
        import Part
        faces = []
        for face in shp.Faces:
            if face.Surface.TypeId == "Part::GeomPlane":
                faces.append(face)
            elif not geometry.is_planar(face):
                faces.append(face)
            else:
                edges = []
                for edge in face.Edges:
                    if edge.Curve.TypeId == "Part::GeomLine" or geometry.is_straight_line(edge):
                        verts = edge.Vertexes
                        edges.append(Part.makeLine(verts[0].Point, verts[1].Point))
                    else:
                        edges.append(edge)
                wires = [Part.Wire(x) for x in Part.sortEdges(edges)]
                face = Part.makeFace(wires, "Part::FaceMakerCheese")
                face.fix(1e-7, 0, 1)
                faces.append(face)
        solid = Part.makeSolid(Part.makeShell(faces))
        if solid.isValid():
            return solid
        _msg(obj.Label + ": " + translate("draft",
            "Converting flat B-spline faces of Facebinder to planar faces failed"
        ))
        return shp

    def onChanged(self, obj, prop):
        self.props_changed_store(prop)

    def addSubobjects(self, obj, facelinks):
        """adds facelinks to this facebinder"""
        # facelinks is an iterable or a selection set:
        # [(<Part::Feature>, ("3.Face3", "3.Face6"))]
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
