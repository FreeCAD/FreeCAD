# SPDX-License-Identifier: LGPL-2.1-or-later
# FaceExtractor.py v2.1 — resilient face lookup (handles wall resizing / toponaming)

import FreeCAD
import Part

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    def QT_TRANSLATE_NOOP(ctx, txt):
        return txt


class FaceExtractorFeature:

    def __init__(self, obj):
        obj.Proxy = self
        self.Type = "FaceExtractor"
        self._setup_properties(obj)

    def _setup_properties(self, obj):
        pl = obj.PropertiesList

        if "SourceFace" not in pl:
            obj.addProperty("App::PropertyLinkSub", "SourceFace", "FaceExtractor",
                QT_TRANSLATE_NOOP("App::Property",
                    "Parent object and sub-element name to extract."), locked=True)

        # Resilience fingerprint — stored after every successful extract
        if "StoredNormal" not in pl:
            obj.addProperty("App::PropertyVector", "StoredNormal", "FaceExtractor",
                QT_TRANSLATE_NOOP("App::Property",
                    "Face normal saved at last successful extraction. "
                    "Used to re-find the face if its index changes after a wall resize."),
                locked=True)
            obj.setEditorMode("StoredNormal", 1)

        if "StoredCenter" not in pl:
            obj.addProperty("App::PropertyVector", "StoredCenter", "FaceExtractor",
                QT_TRANSLATE_NOOP("App::Property",
                    "Face center-of-mass saved at last successful extraction."),
                locked=True)
            obj.setEditorMode("StoredCenter", 1)

        if "FaceIndex" not in pl:
            obj.addProperty("App::PropertyInteger", "FaceIndex", "FaceExtractor",
                QT_TRANSLATE_NOOP("App::Property", "Resolved face index (1-based)."),
                locked=True)
            obj.setEditorMode("FaceIndex", 1)

        if "FaceArea" not in pl:
            obj.addProperty("App::PropertyArea", "FaceArea", "FaceExtractor",
                QT_TRANSLATE_NOOP("App::Property", "Area of the extracted face."),
                locked=True)
            obj.setEditorMode("FaceArea", 1)

        if "FaceNormal" not in pl:
            obj.addProperty("App::PropertyVector", "FaceNormal", "FaceExtractor",
                QT_TRANSLATE_NOOP("App::Property", "Normal of the extracted face."),
                locked=True)
            obj.setEditorMode("FaceNormal", 1)

    def _find_face_by_fingerprint(self, parent_shape, stored_normal, stored_center):
        """
        Scan all faces and return the best match by normal direction + center proximity.
        Handles the FreeCAD toponaming problem when walls are resized.
        Returns (face, sub_name_str) or (None, None).
        """
        if not parent_shape.Faces:
            return None, None

        best_face, best_name, best_score = None, None, -1.0
        sn = stored_normal
        sc = stored_center

        for i, face in enumerate(parent_shape.Faces):
            try:
                n = face.normalAt(0, 0).normalize()
            except Exception:
                continue
            dot = n.dot(sn)  # 1.0 = identical direction
            try:
                dist = (face.CenterOfMass - sc).Length
            except Exception:
                dist = 1e9
            center_score = 1.0 / (1.0 + dist)
            # Weight: normal direction 70%, center proximity 30%
            score = 0.7 * max(dot, 0.0) + 0.3 * center_score
            if score > best_score:
                best_score = score
                best_face = face
                best_name = f"Face{i + 1}"

        return best_face, best_name

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
        element = None

        # Primary lookup by sub-element name
        try:
            candidate = parent_shape.getElement(sub_name)
            if isinstance(candidate, Part.Face):
                element = candidate
        except Exception:
            pass

        # Resilient fallback: re-find by stored normal + center
        if element is None:
            sn = getattr(obj, "StoredNormal", FreeCAD.Vector(0, 0, 0))
            sc = getattr(obj, "StoredCenter", FreeCAD.Vector(0, 0, 0))
            if sn.Length > 1e-6:
                found_face, found_name = self._find_face_by_fingerprint(parent_shape, sn, sc)
                if found_face is not None:
                    element = found_face
                    FreeCAD.Console.PrintMessage(
                        f"FaceExtractor '{obj.Label}': re-found face as "
                        f"'{found_name}' (was '{sub_name}'). Updating reference.\n"
                    )
                    # Write corrected name back so next recompute is fast
                    obj.SourceFace = (parent_obj, (found_name,))

        if element is None:
            FreeCAD.Console.PrintWarning(
                f"FaceExtractor '{obj.Label}': cannot resolve face '{sub_name}' "
                f"in '{parent_obj.Label}'. Topology may have changed too much.\n"
            )
            obj.Shape = Part.Shape()
            return

        # Build output shape
        try:
            obj.Shape = Part.Shell([element])
        except Exception:
            obj.Shape = element

        # Update fingerprint from live face
        try:
            obj.StoredNormal = element.normalAt(0, 0).normalize()
            obj.StoredCenter = element.CenterOfMass
        except Exception:
            pass

        # Update read-only statistics
        try:
            idx_str = sub_names[0].replace("Face", "")
            obj.FaceIndex = int(idx_str) if idx_str.isdigit() else -1
        except Exception:
            pass
        try:
            obj.FaceArea = element.Area
        except Exception:
            pass
        try:
            obj.FaceNormal = element.normalAt(0, 0)
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
    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def getIcon(self):
        return ":/icons/Part_Face.svg"

    def getDefaultDisplayMode(self):
        return "Flat Lines"

    def getDisplayModes(self, obj):
        return ["Flat Lines", "Shaded", "Wireframe"]

    def setDisplayMode(self, mode):
        return mode

    def onChanged(self, vobj, prop):
        pass

    def doubleClicked(self, vobj):
        return True

    def __getstate__(self):
        return {"Object": self.Object.Name if self.Object else ""}

    def __setstate__(self, state):
        if state and "Object" in state:
            doc = FreeCAD.ActiveDocument
            if doc:
                self.Object = doc.getObject(state["Object"])


def makeFaceExtractor(parent_obj, sub_name, name=None):
    """Create a FaceExtractor for a specific face of parent_obj."""
    doc = FreeCAD.ActiveDocument
    if not doc:
        raise RuntimeError("No active document")
    if name is None:
        name = f"{parent_obj.Label}_{sub_name}"
    safe_name = "".join(c if c.isalnum() or c == "_" else "_" for c in name)
    obj = doc.addObject("Part::FeaturePython", safe_name)
    FaceExtractorFeature(obj)
    if FreeCAD.GuiUp:
        FaceExtractorViewProvider(obj.ViewObject)
        obj.ViewObject.Transparency = 60
        obj.ViewObject.LineColor = (1.0, 0.5, 0.0)
        obj.ViewObject.ShapeColor = (1.0, 0.8, 0.3)
    obj.SourceFace = (parent_obj, (sub_name,))
    # Pre-seed fingerprint immediately
    try:
        face = parent_obj.Shape.getElement(sub_name)
        if isinstance(face, Part.Face):
            obj.StoredNormal = face.normalAt(0, 0).normalize()
            obj.StoredCenter = face.CenterOfMass
    except Exception:
        pass
    return obj


def makeFaceExtractorFromSelection():
    """Create FaceExtractor objects for every selected face sub-element."""
    if not FreeCAD.GuiUp:
        return []
    sel = FreeCADGui.Selection.getSelectionEx()
    created = []
    for s in sel:
        parent = s.Object
        for sub_name in s.SubElementNames:
            if not sub_name.startswith("Face"):
                continue
            try:
                fe = makeFaceExtractor(parent, sub_name)
                created.append(fe)
                FreeCAD.Console.PrintMessage(
                    f"Created FaceExtractor: {fe.Name} ({parent.Label}.{sub_name})\n"
                )
            except Exception as e:
                FreeCAD.Console.PrintError(f"FaceExtractor failed for {sub_name}: {e}\n")
    if created:
        FreeCAD.ActiveDocument.recompute()
    else:
        FreeCAD.Console.PrintWarning(
            "No face sub-elements selected. Click a face surface in the 3D view first.\n"
        )
    return created