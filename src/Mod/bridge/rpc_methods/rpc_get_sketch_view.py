from __future__ import annotations

from rpc import *


@rpc_method
def get_sketch_view(
    self,
    doc_name: str,
    sketch_name: str,
    width: int | None = None,
    height: int | None = None,
) -> str | None:
    def check_view():
        try:
            active_view = FreeCADGui.ActiveDocument.ActiveView
            return active_view is not None and hasattr(active_view, "saveImage")
        except Exception:
            return False

    rpc_request_queue.put(check_view)
    if not rpc_response_queue.get(timeout=self.TIMEOUT):
        return None

    fd, tmp_path = tempfile.mkstemp(suffix=".png")
    os.close(fd)

    def screenshot_task():
        _engage_viewport_lock()
        try:
            return self._save_sketch_screenshot(
                tmp_path, doc_name, sketch_name, width, height
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
                "Captured sketch screenshot was empty or corrupt; discarding.\n"
            )
            return None
        return encoded
    if os.path.exists(tmp_path):
        os.remove(tmp_path)
    FreeCAD.Console.PrintWarning(f"Failed to capture sketch screenshot: {res}\n")
    return None
