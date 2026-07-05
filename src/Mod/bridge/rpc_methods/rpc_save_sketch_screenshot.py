from __future__ import annotations

from rpc import *


@rpc_method
def _save_sketch_screenshot(
    self,
    save_path: str,
    doc_name: str,
    sketch_name: str,
    width: int | None = None,
    height: int | None = None,
):
    saved_state: dict[str, dict[str, Any]] = {}
    view = None
    previous_animation: bool | None = None
    previous_navi_cube: bool | None = None
    try:
        doc = FreeCAD.getDocument(doc_name) if doc_name else FreeCAD.ActiveDocument
        if doc is None:
            return f"Document '{doc_name}' not found"
        sketch = doc.getObject(sketch_name)
        if sketch is None:
            return f"Object '{sketch_name}' not found in '{doc_name}'"
        if not str(getattr(sketch, "TypeId", "")).startswith("Sketcher::SketchObject"):
            return f"Object '{sketch_name}' is not a Sketcher::SketchObject"

        view = FreeCADGui.ActiveDocument.ActiveView
        if not hasattr(view, "saveImage"):
            return "Current view does not support screenshots"

        previous_animation = _disable_view_animation(view)
        previous_navi_cube = _disable_navi_cube(view)

        try:
            FreeCADGui.activateWorkbench("SketcherWorkbench")
        except Exception:
            pass

        saved_state = _apply_view_overrides(doc, None, None, [sketch.Name], None, None)

        normal = sketch.Placement.Rotation.multVec(FreeCAD.Vector(0, 0, 1))
        try:
            view.setViewDirection((-normal.x, -normal.y, -normal.z))
        except Exception:
            view.viewTop()

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(sketch)
        FreeCADGui.SendMsgToActiveView("ViewSelection")

        _settle_view(view)

        if width is not None and height is not None:
            view.saveImage(save_path, width, height)
        else:
            view.saveImage(save_path)
        return True
    except Exception as e:
        return str(e)
    finally:
        try:
            _restore_view_overrides(FreeCAD.ActiveDocument, saved_state)
            if view is not None:
                _restore_navi_cube(view, previous_navi_cube)
                _restore_view_animation(view, previous_animation)
            FreeCADGui.updateGui()
        except Exception:
            pass
