# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                          *
# *   Copyright (c) 2026 Regis Benoit Brice Nde Tene <regisndetene@gmail.com>*
# *                                                                          *
# *   This file is part of FreeCAD.                                          *
# *                                                                          *
# *   FreeCAD is free software: you can redistribute it and/or modify it     *
# *   under the terms of the GNU Lesser General Public License as            *
# *   published by the Free Software Foundation, either version 2.1 of the   *
# *   License, or (at your option) any later version.                        *
# *                                                                          *
# *   FreeCAD is distributed in the hope that it will be useful, but         *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
# *   Lesser General Public License for more details.                        *
# *                                                                          *
# *   You should have received a copy of the GNU Lesser General Public       *
# *   License along with FreeCAD. If not, see                                *
# *   <https://www.gnu.org/licenses/>.                                       *
# *                                                                          *
# ***************************************************************************

import FreeCAD
import Part

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:

    def QT_TRANSLATE_NOOP(context, text):
        return text


def _safe_normalized_vector(vector):
    """Return a normalized FreeCAD.Vector or None."""
    if vector is None:
        return None
    try:
        out = FreeCAD.Vector(vector.x, vector.y, vector.z)
    except Exception:
        return None
    if out.Length <= 1e-12:
        return None
    try:
        out.normalize()
    except Exception:
        return None
    return out


def _safe_face_normal(face):
    """Get a reliable normalized face normal."""
    if face is None:
        return None

    try:
        umin, umax, vmin, vmax = face.ParameterRange
        u_mid = (umin + umax) * 0.5
        v_mid = (vmin + vmax) * 0.5
        normal = _safe_normalized_vector(face.normalAt(u_mid, v_mid))
        if normal is not None:
            return normal
    except Exception:
        pass

    try:
        return _safe_normalized_vector(face.normalAt(0, 0))
    except Exception:
        return None


def apply_support_face_view_style(view_object):
    """Apply consistent visual identity for support/extracted faces."""
    if not FreeCAD.GuiUp or not view_object:
        return

    try:
        view_object.ShapeColor = (1.0, 0.8, 0.3)
    except Exception:
        pass

    try:
        view_object.LineColor = (1.0, 0.5, 0.0)
    except Exception:
        pass

    try:
        view_object.Transparency = 60
    except Exception:
        pass

    try:
        modes = view_object.getDisplayModes()
    except Exception:
        modes = []

    try:
        if "Flat Lines" in modes:
            view_object.DisplayMode = "Flat Lines"
        elif modes:
            view_object.DisplayMode = modes[0]
    except Exception:
        pass


def _is_face_extractor(obj):
    """Identify FaceExtractor objects robustly — survives module reloads."""
    proxy = getattr(obj, "Proxy", None)
    if proxy is None:
        return False
    if getattr(proxy, "Type", "") == "FaceExtractor":
        return True
    if isinstance(proxy, FaceExtractorFeature):
        return True
    return False


def _placements_are_close(pl1, pl2, tol_mm=1.0, tol_rad=0.001):
    """
    Return True if two placements represent nearly the same transform.

    Used to detect the FreeCAD convention difference between:
      - Part / Draft objects  → obj.Shape is stored in LOCAL space; the
        object's document Placement positions it.  The shape's internal OCCT
        TopLoc_Location IS (or closely mirrors) the document Placement.
      - Arch objects          → obj.Shape is stored relative to a BASE object
        (e.g. a sketch) whose placement is baked in as the shape's internal
        TopLoc_Location, independently of the Arch object's own Placement.
    """
    try:
        if (pl1.Base - pl2.Base).Length > tol_mm:
            return False
        rel_angle = pl1.inverse().multiply(pl2).Rotation.Angle
        return abs(rel_angle) < tol_rad
    except Exception:
        return False


def _face_total_placement(face_copy, parent_pl):
    """
    Compute the full world Placement for a face extracted from a parent object.

    face_copy.Placement exposes the shape's internal OCCT TopLoc_Location.

    Two conventions exist in FreeCAD:
      A) Part / Draft objects store their shape in local space and carry the
         document Placement as the shape's internal location.  Here
         shape_internal_pl ≈ parent_pl, so the total world transform is
         simply parent_pl (no double-application).
      B) Arch objects store their shape in the BASE SKETCH's local space and
         carry the sketch's placement as the shape's internal location,
         independently of the Arch object's own Placement.  Here
         shape_internal_pl ≠ parent_pl, so we compose both:
         total = parent_pl × shape_internal_pl.

    Returns total_pl (FreeCAD.Placement).
    """
    shape_internal_pl = face_copy.Placement
    if _placements_are_close(shape_internal_pl, parent_pl):
        # Convention A — shape already positioned by parent_pl
        return parent_pl
    else:
        # Convention B — compose independent transforms
        return parent_pl.multiply(shape_internal_pl)


def _cleanup_old_face_extractors(current_obj=None):
    """Remove temporary FaceExtractor objects not marked as permanent."""
    doc = FreeCAD.ActiveDocument
    if not doc:
        return

    to_remove = []
    for obj in doc.Objects:
        if not _is_face_extractor(obj):
            continue
        if not getattr(obj, "IsTemporary", False):
            continue
        if current_obj is not None and obj == current_obj:
            continue
        to_remove.append(obj.Name)

    for name in to_remove:
        try:
            doc.removeObject(name)
        except Exception:
            pass


class FaceExtractorFeature:
    def __init__(self, obj):
        obj.Proxy = self
        self.Type = "FaceExtractor"
        self._setup_properties(obj)

    def _setup_properties(self, obj):
        properties_list = obj.PropertiesList

        if "SourceFace" not in properties_list:
            obj.addProperty(
                "App::PropertyLinkSub",
                "SourceFace",
                "FaceExtractor",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Parent object and sub-element name to extract.",
                ),
                locked=True,
            )

        if "StoredNormal" not in properties_list:
            obj.addProperty(
                "App::PropertyVector",
                "StoredNormal",
                "FaceExtractor",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Face normal saved at last successful extraction. "
                    "Used to re-find the face if its index changes after a wall resize.",
                ),
                locked=True,
            )
            obj.setEditorMode("StoredNormal", 1)

        if "StoredCenter" not in properties_list:
            obj.addProperty(
                "App::PropertyVector",
                "StoredCenter",
                "FaceExtractor",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Face center-of-mass saved at last successful extraction.",
                ),
                locked=True,
            )
            obj.setEditorMode("StoredCenter", 1)

        if "StoredArea" not in properties_list:
            obj.addProperty(
                "App::PropertyFloat",
                "StoredArea",
                "FaceExtractor",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Face area saved at last successful extraction. "
                    "Used to validate that a resolved face is still the same logical face.",
                ),
                locked=True,
            )
            obj.setEditorMode("StoredArea", 1)

        if "FaceIndex" not in properties_list:
            obj.addProperty(
                "App::PropertyInteger",
                "FaceIndex",
                "FaceExtractor",
                QT_TRANSLATE_NOOP("App::Property", "Resolved face index (1-based)."),
                locked=True,
            )
            obj.setEditorMode("FaceIndex", 1)

        if "FaceArea" not in properties_list:
            obj.addProperty(
                "App::PropertyArea",
                "FaceArea",
                "FaceExtractor",
                QT_TRANSLATE_NOOP("App::Property", "Area of the extracted face."),
                locked=True,
            )
            obj.setEditorMode("FaceArea", 1)

        if "FaceNormal" not in properties_list:
            obj.addProperty(
                "App::PropertyVector",
                "FaceNormal",
                "FaceExtractor",
                QT_TRANSLATE_NOOP("App::Property", "Normal of the extracted face."),
                locked=True,
            )
            obj.setEditorMode("FaceNormal", 1)

        if "IsTemporary" not in properties_list:
            obj.addProperty(
                "App::PropertyBool",
                "IsTemporary",
                "FaceExtractor",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If True, this extractor will be replaced by the next face selection.",
                ),
            )
            obj.IsTemporary = False

    def _score_face_match(
        self, face, stored_normal, stored_center, stored_area=0.0, parent_pl=None
    ):
        """
        Score how well a candidate face matches the stored fingerprint.
        All values compared in world space. Normal 55%, area 25%, center 20%.
        """
        if parent_pl is not None:
            try:
                ln = _safe_face_normal(face)
                normal = parent_pl.Rotation.multVec(ln) if ln is not None else None
            except Exception:
                normal = _safe_face_normal(face)
            try:
                center = parent_pl.multVec(face.CenterOfMass)
            except Exception:
                center = face.CenterOfMass
        else:
            normal = _safe_face_normal(face)
            center = face.CenterOfMass

        try:
            normal_score = max(normal.dot(stored_normal), 0.0) if normal is not None else 0.0
        except Exception:
            normal_score = 0.0

        try:
            distance = (center - stored_center).Length
            center_score = 1.0 / (1.0 + (distance / 100.0))
        except Exception:
            center_score = 0.0

        area_score = 0.5
        try:
            face_area = float(face.Area)
            if stored_area and stored_area > 1e-9:
                area_score = min(face_area, stored_area) / max(face_area, stored_area)
        except Exception:
            pass

        return (0.55 * normal_score) + (0.25 * area_score) + (0.20 * center_score)

    def _find_face_by_fingerprint(
        self, parent_shape, stored_normal, stored_center, stored_area=0.0, parent_pl=None
    ):
        """
        Scan all faces and return the best match (normal + center + area in world space).
        Returns (face, sub_name_str, score) or (None, None, -1.0).
        """
        if not parent_shape.Faces:
            return None, None, -1.0

        best_face = None
        best_name = None
        best_score = -1.0

        for index, face in enumerate(parent_shape.Faces):
            score = self._score_face_match(
                face, stored_normal, stored_center, stored_area, parent_pl
            )
            if score > best_score:
                best_score = score
                best_face = face
                best_name = f"Face{index + 1}"

        return best_face, best_name, best_score

    def execute(self, obj):
        if not obj.SourceFace:
            obj.Shape = Part.Shape()
            return

        parent_obj, sub_names = obj.SourceFace
        if not parent_obj or not hasattr(parent_obj, "Shape"):
            obj.Shape = Part.Shape()
            return
        if not sub_names:
            obj.Shape = Part.Shape()
            return

        sub_name = sub_names[0]
        parent_shape = parent_obj.Shape
        if parent_shape.isNull():
            obj.Shape = Part.Shape()
            return

        element = None
        resolved_name = sub_name

        stored_normal = getattr(obj, "StoredNormal", FreeCAD.Vector(0, 0, 0))
        stored_center = getattr(obj, "StoredCenter", FreeCAD.Vector(0, 0, 0))
        stored_area = float(getattr(obj, "StoredArea", 0.0) or 0.0)

        parent_pl = parent_obj.Placement

        candidate = None
        candidate_score = -1.0
        try:
            maybe = parent_shape.getElement(sub_name)
            if isinstance(maybe, Part.Face):
                candidate = maybe
                if stored_normal.Length > 1e-6:
                    candidate_score = self._score_face_match(
                        candidate, stored_normal, stored_center, stored_area, parent_pl
                    )
        except Exception:
            pass

        if candidate is not None:
            element = candidate

        if stored_normal.Length > 1e-6:
            found_face, found_name, best_score = self._find_face_by_fingerprint(
                parent_shape, stored_normal, stored_center, stored_area, parent_pl
            )

            use_fallback = False
            if element is None and found_face is not None:
                use_fallback = True
            elif found_face is not None and candidate is not None:
                if candidate_score < 0.72 or (
                    best_score > candidate_score + 0.05 and found_name != sub_name
                ):
                    use_fallback = True

            if use_fallback and found_face is not None:
                element = found_face
                resolved_name = found_name
                obj.SourceFace = (parent_obj, (found_name,))

        if element is None:
            FreeCAD.Console.PrintWarning(
                f"FaceExtractor '{obj.Label}': cannot resolve face '{sub_name}' "
                f"in '{parent_obj.Label}'. Topology may have changed too much.\n"
            )
            obj.Shape = Part.Shape()
            return

        # ------------------------------------------------------------------ #
        # World-space shape and fingerprint                                    #
        #                                                                      #
        # FreeCAD uses two conflicting conventions for how a shape's OCCT     #
        # TopLoc_Location (exposed via shape.Placement) relates to the        #
        # document object's Placement:                                         #
        #                                                                      #
        #  A) Part / Draft objects: shape internal loc ≈ document Placement.  #
        #     The shape is already positioned by its own internal location;   #
        #     total world transform = parent_pl (once, not twice).            #
        #                                                                      #
        #  B) Arch objects: shape internal loc = base sketch's Placement,     #
        #     independent of the Arch object's own Placement.                 #
        #     total world transform = parent_pl × shape_internal_pl.          #
        #                                                                      #
        # _face_total_placement() detects which convention applies.           #
        # We then:                                                             #
        #  1. Clear the shape's internal location (Placement → identity).     #
        #  2. Compute local geometry properties from the cleared copy.        #
        #  3. Set obj.Placement = total_pl so FreeCAD positions it correctly. #
        # ------------------------------------------------------------------ #
        try:
            face_copy = element.copy()
            total_pl = _face_total_placement(face_copy, parent_pl)
            # Clear the internal location so it does not conflict with
            # obj.Placement (whether or not Part::FeaturePython absorbs it).
            face_copy.Placement = FreeCAD.Placement()
            obj.Shape = face_copy
            obj.Placement = total_pl
        except Exception as err:
            FreeCAD.Console.PrintWarning(
                f"FaceExtractor '{obj.Label}': shape placement failed ({err}), "
                "falling back to parent placement.\n"
            )
            try:
                obj.Shape = element.copy()
                obj.Placement = parent_pl
            except Exception:
                obj.Shape = element

        # Store fingerprint in world space.
        # After clearing the shape's internal location, face_copy.CenterOfMass
        # gives the raw geometric center.  total_pl converts it to world space.
        try:
            local_normal = _safe_face_normal(face_copy)
            if local_normal is not None:
                world_normal = total_pl.Rotation.multVec(local_normal)
                obj.StoredNormal = world_normal
                obj.FaceNormal = world_normal
        except Exception:
            pass

        try:
            obj.StoredCenter = total_pl.multVec(face_copy.CenterOfMass)
        except Exception:
            pass

        try:
            obj.StoredArea = float(element.Area)
            obj.FaceArea = element.Area
        except Exception:
            pass

        try:
            index_str = resolved_name.replace("Face", "")
            obj.FaceIndex = int(index_str) if index_str.isdigit() else -1
        except Exception:
            pass

    def onChanged(self, obj, prop):
        if prop == "SourceFace":
            obj.touch()

    def dumps(self):
        return self.Type

    def loads(self, state):
        self.Type = state if state else "FaceExtractor"

    def onDocumentRestored(self, obj):
        obj.Proxy = self
        self._setup_properties(obj)


class FaceExtractorViewProvider:
    def __init__(self, view_object):
        view_object.Proxy = self
        self.Object = view_object.Object
        apply_support_face_view_style(view_object)

    def attach(self, view_object):
        self.ViewObject = view_object
        self.Object = view_object.Object
        apply_support_face_view_style(view_object)

    def getIcon(self):
        return ":/icons/Draft_Facebinder.svg"

    def getDefaultDisplayMode(self):
        return "Flat Lines"

    def getDisplayModes(self, obj):
        return ["Flat Lines", "Shaded", "Wireframe"]

    def setDisplayMode(self, mode):
        return mode

    def onChanged(self, view_object, prop):
        if prop in ("Visibility", "DisplayMode"):
            apply_support_face_view_style(view_object)

    def doubleClicked(self, view_object):
        return True

    def __getstate__(self):
        return {"Object": self.Object.Name if self.Object else ""}

    def __setstate__(self, state):
        if state and "Object" in state:
            doc = FreeCAD.ActiveDocument
            if doc:
                self.Object = doc.getObject(state["Object"])


def make_face_extractor(parent_obj, sub_name, name=None, temporary=True, _skip_cleanup=False):
    """Create a FaceExtractor for a specific face of parent_obj."""
    doc = FreeCAD.ActiveDocument
    if not doc:
        raise RuntimeError("No active document")

    if temporary and not _skip_cleanup:
        _cleanup_old_face_extractors()

    if name is None:
        name = f"{parent_obj.Label}_{sub_name}"
    safe_name = "".join(char if char.isalnum() or char == "_" else "_" for char in name)

    obj = doc.addObject("Part::FeaturePython", safe_name)
    FaceExtractorFeature(obj)

    obj.IsTemporary = temporary
    obj.SourceFace = (parent_obj, (sub_name,))

    # Seed world-space fingerprint so the first execute() comparison is valid.
    try:
        face = parent_obj.Shape.getElement(sub_name)
        if isinstance(face, Part.Face):
            parent_pl = parent_obj.Placement
            face_copy = face.copy()
            total_pl = _face_total_placement(face_copy, parent_pl)
            face_copy.Placement = FreeCAD.Placement()  # clear for raw geom

            local_normal = _safe_face_normal(face_copy)
            if local_normal is not None:
                obj.StoredNormal = total_pl.Rotation.multVec(local_normal)
            obj.StoredCenter = total_pl.multVec(face_copy.CenterOfMass)
            obj.StoredArea = float(face.Area)
    except Exception:
        pass

    if FreeCAD.GuiUp:
        try:
            FaceExtractorViewProvider(obj.ViewObject)
            apply_support_face_view_style(obj.ViewObject)
        except Exception as e:
            FreeCAD.Console.PrintError(f"Failed to create FaceExtractor view provider: {e}\n")

    return obj


def make_permanent_face_extractor(parent_obj, sub_name, name=None):
    """Create a permanent FaceExtractor that won't be auto-cleaned."""
    return make_face_extractor(parent_obj, sub_name, name, temporary=False)


def make_face_extractor_from_selection(permanent=False):
    """Create FaceExtractor objects for every selected face sub-element."""
    if not FreeCAD.GuiUp:
        return []

    if not permanent:
        _cleanup_old_face_extractors()

    selection = FreeCADGui.Selection.getSelectionEx()
    created = []
    for sel in selection:
        parent = sel.Object
        for sub_name in sel.SubElementNames:
            if not sub_name.startswith("Face"):
                continue
            try:
                face_extractor = make_face_extractor(
                    parent, sub_name, temporary=not permanent, _skip_cleanup=True
                )
                created.append(face_extractor)
            except Exception as e:
                FreeCAD.Console.PrintError(f"FaceExtractor failed for {sub_name}: {e}\n")
    if created:
        FreeCAD.ActiveDocument.recompute()
    else:
        FreeCAD.Console.PrintWarning(
            "No face sub-elements selected. Click a face surface in the 3D view first.\n"
        )
    return created
