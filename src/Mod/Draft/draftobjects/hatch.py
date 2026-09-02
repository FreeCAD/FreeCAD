# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2021 Yorik van Havre <yorik@uncreated.net>              *
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


"""This module contains FreeCAD commands for the Draft workbench"""

import os
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
from draftgeoutils.general import geomType
from draftobjects.base import DraftObject
from draftutils import gui_utils
from draftutils.messages import _err, _log
from draftutils.translate import translate


class Hatch(DraftObject):

    def __init__(self, obj):

        obj.Proxy = self
        self.Type = "Hatch"
        self.setProperties(obj)

    def setProperties(self, obj):

        pl = obj.PropertiesList
        if not "Faces" in pl:
            obj.addProperty(
                "App::PropertyLinkSubList",
                "Faces",
                "Hatch",
                QT_TRANSLATE_NOOP("App::Property", "The objects and faces used by this object"),
                locked=True,
            )
        if not "File" in pl:
            obj.addProperty(
                "App::PropertyFile",
                "File",
                "Hatch",
                QT_TRANSLATE_NOOP("App::Property", "The PAT file used by this object"),
                locked=True,
            )
        if not "Pattern" in pl:
            obj.addProperty(
                "App::PropertyString",
                "Pattern",
                "Hatch",
                QT_TRANSLATE_NOOP("App::Property", "The pattern name used by this object"),
                locked=True,
            )
        if not "Scale" in pl:
            obj.addProperty(
                "App::PropertyFloat",
                "Scale",
                "Hatch",
                QT_TRANSLATE_NOOP("App::Property", "The pattern scale used by this object"),
                locked=True,
            )
        if not "Rotation" in pl:
            obj.addProperty(
                "App::PropertyAngle",
                "Rotation",
                "Hatch",
                QT_TRANSLATE_NOOP("App::Property", "The pattern rotation used by this object"),
                locked=True,
            )
        if not "Translate" in pl:
            obj.addProperty(
                "App::PropertyBool",
                "Translate",
                "Hatch",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If set to False, hatch is applied as is to the faces, without translation (this might give wrong results for non-XY faces)",
                ),
                locked=True,
            )
            obj.Translate = True

    def onDocumentRestored(self, obj):
        self.setProperties(obj)
        if hasattr(obj, "Base"):
            self.update_properties_26v3(obj)
        super().onDocumentRestored(obj)
        gui_utils.restore_view_object(
            obj, vp_module="view_hatch", vp_class="ViewProviderDraftHatch"
        )

    def update_properties_26v3(self, obj):
        """Update Base to Faces property."""
        obj.Faces = obj.Base  # Conversion to list of tuples happens automatically.
        obj.setPropertyStatus("Base", "-LockDynamic")
        obj.removeProperty("Base")
        _log(f"v1.2, {obj.Name} changed 'Base' to 'Faces' property")

    def dumps(self):
        return

    def loads(self, state):
        self.Type = "Hatch"

    def execute(self, obj):

        if (
            self.props_changed_placement_only(obj)
            or not obj.Faces
            or not obj.File
            or not obj.Pattern
            or not obj.Scale
        ):
            self.props_changed_clear()
            return

        self.props_changed_clear()

        if obj.File[0] == ".":
            # File path relative to the FreeCAD file directory.
            pat_file = os.path.join(os.path.dirname(obj.Document.FileName), obj.File)
            # We need the absolute path to do some file checks.
            pat_file = os.path.abspath(pat_file)
        else:
            pat_file = obj.File

        # File checks:
        if not os.path.exists(pat_file):
            _err(obj.Label + ": " + translate("draft", "PAT file not found"))
            return
        if not os.path.isfile(pat_file):
            _err(obj.Label + ": " + translate("draft", "Specified PAT file is not a file"))
            return
        if os.path.splitext(pat_file)[1].lower() != ".pat":
            _err(obj.Label + ": " + translate("draft", "Specified file type is not supported"))
            return
        if not obj.Pattern in self.getPatterns(pat_file):
            _err(obj.Label + ": " + translate("draft", "Pattern not found in PAT file"))
            return

        import Part
        import TechDraw

        faces = []
        try:
            for sel in obj.Faces:
                if not hasattr(sel[0], "Shape"):
                    pass
                elif sel[1] == ("",):
                    faces.extend(sel[0].Shape.Faces)
                else:
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

        # In TechDraw edges longer than 9999.9 (ca. 10m) are considered 'crazy'.
        # Lines in hatch patterns are also checked. We need to change a parameter:
        param_grp = App.ParamGet("User parameter:BaseApp/Preferences/Mod/TechDraw/debug")
        if "allowCrazyEdge" not in param_grp.GetBools():
            old_allow_crazy_edge = None
        else:
            old_allow_crazy_edge = param_grp.GetBool("allowCrazyEdge")
        param_grp.SetBool("allowCrazyEdge", True)

        shapes = []
        for face in faces:
            if face.findPlane():  # Only planar faces.
                face = face.copy()
                if obj.Translate:
                    mtx = None
                    w = face.normalAt(0, 0)
                    # Try to base a matrix on the first straight edge with
                    # a reasonable length (> 0.001):
                    for e in face.Edges:
                        if geomType(e) == "Line":
                            sta = e.firstVertex().Point
                            end = e.lastVertex().Point
                            u = end.sub(sta)
                            if u.Length > 0.001:
                                u = u.normalize()
                                v = w.cross(u)
                                # fmt: off
                                mtx = App.Matrix(u.x, v.x, w.x, sta.x,
                                                 u.y, v.y, w.y, sta.y,
                                                 u.z, v.z, w.z, sta.z,
                                                 0.0, 0.0, 0.0, 1.0)
                                # fmt: on
                                break
                    # If no suitable straight edge was found use a default matrix:
                    if not mtx:
                        cen = face.CenterOfMass
                        rot = App.Rotation(App.Vector(0, 0, 1), w)
                        mtx = App.Placement(cen, rot).Matrix
                    face = face.transformShape(mtx.inverse()).Faces[0]
                if obj.Rotation.Value:
                    face.rotate(App.Vector(), App.Vector(0, 0, 1), -obj.Rotation)

                shape = TechDraw.makeGeomHatch(face, obj.Scale, obj.Pattern, pat_file)

                if obj.Rotation.Value:
                    shape.rotate(App.Vector(), App.Vector(0, 0, 1), obj.Rotation)
                if obj.Translate:
                    shape = shape.transformShape(mtx)
                shapes.append(shape)

        if old_allow_crazy_edge is None:
            param_grp.RemBool("allowCrazyEdge")
        else:
            param_grp.SetBool("allowCrazyEdge", old_allow_crazy_edge)

        if shapes:
            obj.Shape = Part.makeCompound(shapes)

    def _report_face_error(self, obj):
        _wrn(obj.Label + ": " + translate("draft", "No valid faces for hatch"))

    def onChanged(self, obj, prop):

        self.props_changed_store(prop)

    def getPatterns(self, filename):
        """returns a list of pattern names found in a PAT file"""
        patterns = []
        if os.path.exists(filename):
            with open(filename) as patfile:
                for line in patfile:
                    if line.startswith("*"):
                        patterns.append(line.split(",")[0][1:])
        return patterns

    def add_faces(self, obj, face_links):
        """adds face_links to this hatch (compare addSubobjects in facebinder.py)"""
        # face_links is an iterable or a selection set:
        # [(<Part::Feature>, ("3.Face3", "3.Face6"))]
        # or:
        # Gui.Selection.getSelectionEx("", 0)
        # or:
        # Gui.Selection.getSelection()
        sels = obj.Faces
        for sel in face_links:
            if isinstance(sel, list) or isinstance(sel, tuple):
                sel_obj, sel_subs = sel
            elif sel.isDerivedFrom("Gui::SelectionObject"):
                sel_obj = sel.Object
                sel_subs = sel.SubElementNames
            else:
                sel_obj = sel
                sel_subs = ("",)
            if sel_obj.Name != obj.Name:
                if sel_subs in ((), ("",)):
                    # Use all faces of the object.
                    sels.append((sel_obj, ""))
                else:
                    for sub in sel_subs:
                        if "Face" in sub:
                            sels.append((sel_obj, sub))
        obj.Faces = sels
