from __future__ import annotations

from rpc import *


@rpc_method
def get_active_screenshot(
    self,
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
) -> str | None:
    def check_view():
        try:
            active_view = FreeCADGui.ActiveDocument.ActiveView
            if active_view is None:
                return False
            has_save = hasattr(active_view, "saveImage")
            FreeCAD.Console.PrintMessage(
                f"View type: {type(active_view).__name__}, Has saveImage: {has_save}\n"
            )
            return has_save
        except Exception as e:
            FreeCAD.Console.PrintError(f"Error checking view: {e}\n")
            return False

    rpc_request_queue.put(check_view)
    supports = rpc_response_queue.get(timeout=self.TIMEOUT)
    if not supports:
        return None

    fd, tmp_path = tempfile.mkstemp(suffix=".png")
    os.close(fd)

    def screenshot_task():
        _engage_viewport_lock()
        try:
            return self._save_active_screenshot(
                tmp_path,
                view_name,
                width,
                height,
                focus_object,
                display_mode,
                hide,
                show_only,
                highlight,
                highlight_color,
                camera_mode,
            )
        finally:
            _release_viewport_lock()

    rpc_request_queue.put(screenshot_task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if res is True:
        try:
            encoded = _read_valid_screenshot(tmp_path)
        finally:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)
        if encoded is None:
            FreeCAD.Console.PrintWarning(
                "Captured screenshot file was empty or corrupt; discarding.\n"
            )
            return None
        return encoded
    if os.path.exists(tmp_path):
        os.remove(tmp_path)
    FreeCAD.Console.PrintWarning(f"Failed to capture screenshot: {res}\n")
    return None
