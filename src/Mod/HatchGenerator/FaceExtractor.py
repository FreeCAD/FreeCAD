# SPDX-License-Identifier: LGPL-2.1-or-later
# ============================================================================
# FaceExtractor.py
# Parametric object that extracts and tracks a specific face from a parent
# 3D object (wall, slab, solid, etc.) and rebuilds when the parent changes.
#
# Usage:
#   fe = makeFaceExtractor(wall_obj, "Face5")
#   # fe.Shape is now the face as a shell
#   # when wall_obj rebuilds, fe rebuilds automatically
# ============================================================================

import FreeCAD
import Part

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    def QT_TRANSLATE_NOOP(ctx, txt):
        return txt


# ============================================================================
# Parametric Feature
# ============================================================================
class FaceExtractorFeature:
    """
    Parametric object that holds a PropertyLinkSub pointing to a specific
    face (e.g. "Face5") on a parent object. Its execute() pulls the current
    face from the parent's live Shape, so it updates automatically when the
    parent rebuilds.
    """

    def __init__(self, obj):
        obj.Proxy = self
        self.Type = "FaceExtractor"
        self._setup_properties(obj)

    def _setup_properties(self, obj):
        pl = obj.PropertiesList

        if "SourceFace" not in pl:
            obj.addProperty(
                "App::PropertyLinkSub",
                "SourceFace",
                "FaceExtractor",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The parent object and sub-element name (e.g. Face5) to extract.",
                ),
                locked=True,
            )

        if "FaceIndex" not in pl:
            obj.addProperty(
                "App::PropertyInteger",
                "FaceIndex",
                "FaceExtractor",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Read-only: the resolved face index (1-based) from the last recompute.",
                ),
                locked=True,
            )
            obj.setEditorMode("FaceIndex", 1)  # read-only in GUI

        if "FaceArea" not in pl:
            obj.addProperty(
                "App::PropertyArea",
                "FaceArea",
                "FaceExtractor",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Area of the extracted face (updated on recompute).",
                ),
                locked=True,
            )
            obj.setEditorMode("FaceArea", 1)

        if "FaceNormal" not in pl:
            obj.addProperty(
                "App::PropertyVector",
                "FaceNormal",
                "FaceExtractor",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Normal vector of the extracted face (updated on recompute).",
                ),
                locked=True,
            )
            obj.setEditorMode("FaceNormal", 1)

    # ------------------------------------------------------------------
    def execute(self, obj):
        """Pull the face from the parent object's current shape."""
        if not obj.SourceFace:
            obj.Shape = Part.Shape()
            return

        parent_obj, sub_names = obj.SourceFace

        if not parent_obj or not hasattr(parent_obj, "Shape"):
            FreeCAD.Console.PrintWarning(
                f"FaceExtractor '{obj.Label}': parent has no Shape.\n"
            )
            obj.Shape = Part.Shape()
            return

        # sub_names is a tuple like ("Face5",) or ("Face5", "Face7")
        # We only handle a single face reference here.
        if not sub_names:
            FreeCAD.Console.PrintWarning(
                f"FaceExtractor '{obj.Label}': no sub-element specified.\n"
            )
            obj.Shape = Part.Shape()
            return

        sub_name = sub_names[0]  # e.g. "Face5"

        try:
            element = parent_obj.Shape.getElement(sub_name)
        except Exception as e:
            FreeCAD.Console.PrintWarning(
                f"FaceExtractor '{obj.Label}': could not get element "
                f"'{sub_name}' from '{parent_obj.Label}': {e}\n"
            )
            obj.Shape = Part.Shape()
            return

        if not isinstance(element, Part.Face):
            FreeCAD.Console.PrintWarning(
                f"FaceExtractor '{obj.Label}': sub-element '{sub_name}' "
                f"is not a Face.\n"
            )
            obj.Shape = Part.Shape()
            return

        # Wrap the face in a Shell so it is a valid solid-like Shape
        try:
            shell = Part.Shell([element])
            obj.Shape = shell
        except Exception:
            # Fallback: assign the face directly
            obj.Shape = element

        # Update statistics
        try:
            idx_str = sub_name.replace("Face", "")
            obj.FaceIndex = int(idx_str) if idx_str.isdigit() else -1
        except Exception:
            obj.FaceIndex = -1

        try:
            obj.FaceArea = element.Area
        except Exception:
            pass

        try:
            normal = element.normalAt(0, 0)
            obj.FaceNormal = normal
        except Exception:
            pass

    # ------------------------------------------------------------------
    def onChanged(self, obj, prop):
        if prop == "SourceFace":
            # Trigger recompute when the link changes
            obj.touch()

    def dumps(self):
        return self.Type

    def loads(self, state):
        self.Type = state if state else "FaceExtractor"

    def onDocumentRestored(self, obj):
        obj.Proxy = self
        self._setup_properties(obj)


# ============================================================================
# View Provider
# ============================================================================
class FaceExtractorViewProvider:
    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def getIcon(self):
        # Reuse a generic face/surface icon from FreeCAD's built-ins
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


# ============================================================================
# Factory function
# ============================================================================
def makeFaceExtractor(parent_obj, sub_name, name=None):
    """
    Create a FaceExtractor object linked to a specific face of parent_obj.

    Parameters
    ----------
    parent_obj : FreeCAD document object
        The wall, slab, solid, etc. that owns the face.
    sub_name : str
        The sub-element name, e.g. "Face5".
    name : str, optional
        Name for the new object. Defaults to parent label + "_" + sub_name.

    Returns
    -------
    FreeCAD document object
        The new FaceExtractor feature.

    Example
    -------
    wall = App.ActiveDocument.getObject("Wall002")
    fe = makeFaceExtractor(wall, "Face5")
    App.ActiveDocument.recompute()
    # fe.Shape is now wall.Shape.getElement("Face5") as a Shell
    """
    doc = FreeCAD.ActiveDocument
    if not doc:
        raise RuntimeError("No active document")

    if name is None:
        name = f"{parent_obj.Label}_{sub_name}"
    # Sanitize: object names cannot have spaces or special chars
    safe_name = "".join(c if c.isalnum() or c == "_" else "_" for c in name)

    obj = doc.addObject("Part::FeaturePython", safe_name)
    FaceExtractorFeature(obj)

    if FreeCAD.GuiUp:
        FaceExtractorViewProvider(obj.ViewObject)
        # Show semi-transparent so the parent surface is still visible
        obj.ViewObject.Transparency = 60
        obj.ViewObject.LineColor = (1.0, 0.5, 0.0)  # orange outline
        obj.ViewObject.ShapeColor = (1.0, 0.8, 0.3)

    # Set the link — must be a tuple (object, (sub_element_name,))
    obj.SourceFace = (parent_obj, (sub_name,))

    return obj


# ============================================================================
# Convenience: create from current GUI selection
# ============================================================================
def makeFaceExtractorFromSelection():
    """
    Read the current FreeCAD GUI selection and create a FaceExtractor
    for each selected face sub-element.

    Returns
    -------
    list of FaceExtractor objects created, or empty list if no face selected.
    """
    if not FreeCAD.GuiUp:
        FreeCAD.Console.PrintWarning("makeFaceExtractorFromSelection requires GUI.\n")
        return []

    sel = FreeCADGui.Selection.getSelectionEx()
    created = []

    for s in sel:
        parent = s.Object
        for sub_name in s.SubElementNames:
            if not sub_name.startswith("Face"):
                FreeCAD.Console.PrintWarning(
                    f"Skipping non-face sub-element: {sub_name}\n"
                )
                continue
            try:
                fe = makeFaceExtractor(parent, sub_name)
                created.append(fe)
                FreeCAD.Console.PrintMessage(
                    f"Created FaceExtractor: {fe.Name} "
                    f"({parent.Label}.{sub_name})\n"
                )
            except Exception as e:
                FreeCAD.Console.PrintError(
                    f"Failed to create FaceExtractor for "
                    f"{parent.Label}.{sub_name}: {e}\n"
                )

    if created:
        FreeCAD.ActiveDocument.recompute()
    else:
        FreeCAD.Console.PrintWarning(
            "No face sub-elements found in selection. "
            "Click a face on a 3D object, then run this command.\n"
        )

    return created
