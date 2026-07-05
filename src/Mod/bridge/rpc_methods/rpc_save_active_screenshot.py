from __future__ import annotations

from rpc import *


@rpc_method
def _save_active_screenshot(
    self,
    save_path: str,
    view_name: str = "Isometric",
    width: int | None = None,
    height: int | None = None,
    focus_object: str | None = None,
    display_mode: str | None = None,
    hide: list[str] | None = None,
    show_only: list[str] | None = None,
    highlight: list[str] | None = None,
    highlight_color: tuple | None = None,
    camera_mode: str | None = None,
):
    saved_state: dict[str, dict[str, Any]] = {}
    view = None
    previous_animation: bool | None = None
    previous_navi_cube: bool | None = None
    previous_camera_type: str | None = None
    requested_camera: str | None = None
    try:
        view = FreeCADGui.ActiveDocument.ActiveView
        if not hasattr(view, "saveImage"):
            return "Current view does not support screenshots"

        previous_animation = _disable_view_animation(view)
        previous_navi_cube = _disable_navi_cube(view)

        if camera_mode:
            requested_camera = _CAMERA_MODES.get(camera_mode.strip().lower())
            if requested_camera is None:
                raise ValueError(f"Invalid camera mode: {camera_mode}")
            previous_camera_type = _get_camera_type(view)
            if previous_camera_type is None or previous_camera_type != requested_camera:
                _set_camera_type(view, requested_camera)

        doc = FreeCAD.ActiveDocument
        color_tuple = None
        if highlight_color is not None:
            try:
                color_tuple = (
                    float(highlight_color[0]),
                    float(highlight_color[1]),
                    float(highlight_color[2]),
                )
            except Exception:
                color_tuple = None

        saved_state = _apply_view_overrides(
            doc, display_mode, hide, show_only, highlight, color_tuple
        )

        fn = _VIEW_PRESETS.get(view_name)
        if fn:
            fn(view)
        elif view_name != "current":
            raise ValueError(f"Invalid view name: {view_name}")

        if focus_object:
            ref, _ = _resolve_target(doc, focus_object) if doc else (None, None)
            if ref is not None:
                FreeCADGui.Selection.clearSelection()
                FreeCADGui.Selection.addSelection(ref)
                FreeCADGui.SendMsgToActiveView("ViewSelection")
            else:
                view.fitAll()
        else:
            view.fitAll()

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
                if (
                    previous_camera_type is not None
                    and requested_camera is not None
                    and previous_camera_type != requested_camera
                ):
                    _set_camera_type(view, previous_camera_type)
                _restore_navi_cube(view, previous_navi_cube)
                _restore_view_animation(view, previous_animation)
            FreeCADGui.updateGui()
        except Exception:
            pass
