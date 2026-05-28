# SPDX-License-Identifier: LGPL-2.1-or-later
"""Apply Copilot command plans to FreeCAD documents."""

import os

import FreeCAD as App
try:
    import FreeCADGui as Gui
except ImportError:
    Gui = None


class CopilotExecutor:
    """Execute the small command language produced by provider.py."""

    def run(self, plan):
        results = []
        for step in plan:
            action = step.get("action")
            handler = getattr(self, "_{0}".format(action), None)
            if handler is None:
                raise ValueError("Unsupported action: {0}".format(action))
            results.append(handler(step))
        return results

    def _document(self):
        doc = App.ActiveDocument
        if doc is None:
            doc = App.newDocument("CopilotModel")
        return doc

    def _selection(self):
        if Gui is None:
            return []
        return list(Gui.Selection.getSelection())

    def _create_box(self, step):
        doc = self._document()
        obj = doc.addObject("Part::Box", step.get("name", "CopilotBox"))
        obj.Length = float(step.get("length", 10.0))
        obj.Width = float(step.get("width", 10.0))
        obj.Height = float(step.get("height", 10.0))
        self._apply_placement(obj, step)
        doc.recompute()
        self._select(obj)
        return "Created box {0}".format(obj.Label)

    def _create_cylinder(self, step):
        doc = self._document()
        obj = doc.addObject("Part::Cylinder", step.get("name", "CopilotCylinder"))
        obj.Radius = float(step.get("radius", 5.0))
        obj.Height = float(step.get("height", 10.0))
        self._apply_placement(obj, step)
        doc.recompute()
        self._select(obj)
        return "Created cylinder {0}".format(obj.Label)

    def _create_sphere(self, step):
        doc = self._document()
        obj = doc.addObject("Part::Sphere", step.get("name", "CopilotSphere"))
        obj.Radius = float(step.get("radius", 5.0))
        self._apply_placement(obj, step)
        doc.recompute()
        self._select(obj)
        return "Created sphere {0}".format(obj.Label)

    def _create_cone(self, step):
        doc = self._document()
        obj = doc.addObject("Part::Cone", step.get("name", "CopilotCone"))
        obj.Radius1 = float(step.get("radius1", 5.0))
        obj.Radius2 = float(step.get("radius2", 0.0))
        obj.Height = float(step.get("height", 10.0))
        self._apply_placement(obj, step)
        doc.recompute()
        self._select(obj)
        return "Created cone {0}".format(obj.Label)

    def _create_torus(self, step):
        doc = self._document()
        obj = doc.addObject("Part::Torus", step.get("name", "CopilotTorus"))
        obj.Radius1 = float(step.get("radius1", 10.0))
        obj.Radius2 = float(step.get("radius2", 2.0))
        self._apply_placement(obj, step)
        doc.recompute()
        self._select(obj)
        return "Created torus {0}".format(obj.Label)

    def _move_selected(self, step):
        objects = self._require_selection()
        vector = App.Vector(float(step.get("x", 0.0)), float(step.get("y", 0.0)), float(step.get("z", 0.0)))
        for obj in objects:
            obj.Placement.Base = obj.Placement.Base + vector
        App.ActiveDocument.recompute()
        return "Moved {0} selected object(s)".format(len(objects))

    def _rotate_selected(self, step):
        objects = self._require_selection()
        axis_name = step.get("axis", "z").lower()
        axis = {"x": App.Vector(1, 0, 0), "y": App.Vector(0, 1, 0), "z": App.Vector(0, 0, 1)}.get(axis_name)
        if axis is None:
            raise ValueError("Axis must be x, y, or z.")
        angle = float(step.get("angle", 90.0))
        for obj in objects:
            obj.Placement.rotate(App.Vector(0, 0, 0), axis, angle)
        App.ActiveDocument.recompute()
        return "Rotated {0} selected object(s)".format(len(objects))

    def _scale_selected(self, step):
        objects = self._require_selection()
        factor = float(step.get("factor", 1.0))
        if factor <= 0:
            raise ValueError("Scale factor must be greater than zero.")
        for obj in objects:
            obj.Placement.Base = obj.Placement.Base.multiply(factor)
            for prop in ("Length", "Width", "Height", "Radius", "Radius1", "Radius2"):
                if hasattr(obj, prop):
                    setattr(obj, prop, getattr(obj, prop) * factor)
        App.ActiveDocument.recompute()
        return "Scaled {0} selected object(s)".format(len(objects))

    def _delete_selected(self, step):
        objects = self._require_selection()
        doc = App.ActiveDocument
        for obj in objects:
            doc.removeObject(obj.Name)
        doc.recompute()
        return "Deleted {0} selected object(s)".format(len(objects))

    def _set_color(self, step):
        objects = self._objects_from_step(step) or self._require_selection()
        color = self._coerce_color(step.get("color", [0.8, 0.8, 0.8]))
        for obj in objects:
            if hasattr(obj, "ViewObject"):
                obj.ViewObject.ShapeColor = color
        return "Colored {0} object(s)".format(len(objects))

    def _select_by_name(self, step):
        name = step.get("name")
        if not name:
            raise ValueError("select_by_name requires a name.")
        obj = self._find_object(name)
        self._select(obj)
        return "Selected {0}".format(obj.Label)

    def _boolean_fuse(self, step):
        doc = self._document()
        objects = self._objects_from_names(step.get("objects"))
        if len(objects) < 2:
            raise ValueError("boolean_fuse requires at least two objects.")
        obj = doc.addObject("Part::MultiFuse", step.get("name", "CopilotFuse"))
        obj.Shapes = objects
        doc.recompute()
        self._hide(objects)
        self._select(obj)
        return "Fused {0} object(s) into {1}".format(len(objects), obj.Label)

    def _boolean_cut(self, step):
        doc = self._document()
        base = self._find_object(step.get("base"))
        tool = self._find_object(step.get("tool"))
        obj = doc.addObject("Part::Cut", step.get("name", "CopilotCut"))
        obj.Base = base
        obj.Tool = tool
        doc.recompute()
        self._hide([base, tool])
        self._select(obj)
        return "Cut {0} with {1}".format(base.Label, tool.Label)

    def _fit_view(self, step):
        if Gui is not None:
            Gui.SendMsgToActiveView("ViewFit")
        return "Fit active view"

    def _save(self, step):
        doc = self._document()
        path = step.get("path")
        if not path:
            if doc.FileName:
                doc.save()
                return "Saved {0}".format(doc.FileName)
            raise ValueError("Please provide a path, for example: save as /workspace/FreeCAD/model.FCStd")
        path = os.path.abspath(os.path.expanduser(path))
        doc.saveAs(path)
        return "Saved {0}".format(path)

    def _open(self, step):
        path = step.get("path")
        if not path:
            raise ValueError("Please provide a file path to open.")
        App.openDocument(os.path.abspath(os.path.expanduser(path)))
        return "Opened {0}".format(path)

    def _require_selection(self):
        objects = self._selection()
        if not objects:
            raise ValueError("Select one or more objects first.")
        return objects

    def _apply_placement(self, obj, step):
        obj.Placement.Base = App.Vector(
            float(step.get("x", 0.0)),
            float(step.get("y", 0.0)),
            float(step.get("z", 0.0)),
        )

    def _objects_from_step(self, step):
        if step.get("name"):
            return [self._find_object(step.get("name"))]
        if step.get("objects"):
            return self._objects_from_names(step.get("objects"))
        return []

    def _objects_from_names(self, names):
        if isinstance(names, str):
            names = [name.strip() for name in names.split(",") if name.strip()]
        if not isinstance(names, (list, tuple)):
            raise ValueError("Object names must be a list or comma-separated string.")
        return [self._find_object(name) for name in names]

    def _find_object(self, name):
        if not name:
            raise ValueError("Object name is required.")
        doc = self._document()
        for obj in doc.Objects:
            if obj.Name == name or obj.Label == name:
                return obj
        raise ValueError("Object not found: {0}".format(name))

    def _hide(self, objects):
        for obj in objects:
            if hasattr(obj, "ViewObject"):
                obj.ViewObject.Visibility = False

    def _coerce_color(self, color):
        if isinstance(color, str):
            colors = {
                "red": (1.0, 0.0, 0.0),
                "green": (0.0, 0.8, 0.0),
                "blue": (0.0, 0.2, 1.0),
                "yellow": (1.0, 0.9, 0.0),
                "orange": (1.0, 0.45, 0.0),
                "white": (1.0, 1.0, 1.0),
                "black": (0.0, 0.0, 0.0),
                "gray": (0.45, 0.45, 0.45),
                "grey": (0.45, 0.45, 0.45),
            }
            return colors.get(color.lower(), (0.8, 0.8, 0.8))
        return tuple(float(part) for part in color)

    def _select(self, obj):
        if Gui is not None:
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(obj)
            Gui.SendMsgToActiveView("ViewFit")
