from __future__ import annotations

from rpc import *


@rpc_method
def get_ortho_views(
    self,
    views: list[str] | None = None,
    tile_width: int = 800,
    tile_height: int = 600,
    focus_object: str | None = None,
    display_mode: str | None = None,
    hide: list[str] | None = None,
    show_only: list[str] | None = None,
    highlight: list[str] | None = None,
    highlight_color: tuple | None = None,
    camera_mode: str | None = None,
) -> dict[str, str]:
    if views is None:
        views = ["Front", "Right", "Top", "Back", "Left", "Bottom", "Isometric"]

    def check_view():
        try:
            active_view = FreeCADGui.ActiveDocument.ActiveView
            return active_view is not None and hasattr(active_view, "saveImage")
        except Exception:
            return False

    rpc_request_queue.put(check_view)
    if not rpc_response_queue.get(timeout=self.TIMEOUT):
        return {}

    targets: list[tuple[str, str]] = []
    for view_name in views:
        if view_name not in _VIEW_PRESETS and view_name != "current":
            continue
        fd, tmp_path = tempfile.mkstemp(suffix=".png")
        os.close(fd)
        targets.append((view_name, tmp_path))

    results: dict[str, str] = {}
    if not targets:
        return results

    rpc_request_queue.put(
        lambda: self._capture_ortho_batch(
            targets,
            tile_width,
            tile_height,
            focus_object,
            display_mode,
            hide,
            show_only,
            highlight,
            highlight_color,
            camera_mode,
        )
    )
    batch_timeout = self.TIMEOUT * (len(targets) + 1)
    try:
        outcomes = rpc_response_queue.get(timeout=batch_timeout)
    except queue.Empty:
        outcomes = None

    try:
        if isinstance(outcomes, dict):
            for view_name, tmp_path in targets:
                res = outcomes.get(view_name)
                if res is True:
                    encoded = _read_valid_screenshot(tmp_path)
                    if encoded is not None:
                        results[view_name] = encoded
                    else:
                        FreeCAD.Console.PrintWarning(
                            f"Captured {view_name} tile was empty or corrupt; skipping.\n"
                        )
                else:
                    FreeCAD.Console.PrintWarning(
                        f"Failed to capture {view_name}: {res}\n"
                    )
        else:
            FreeCAD.Console.PrintWarning(f"Failed to capture ortho views: {outcomes}\n")
    finally:
        for _view_name, tmp_path in targets:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)

    return results
