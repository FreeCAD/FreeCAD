from __future__ import annotations

from rpc import *


@rpc_method
def reset_view(
    self,
    take_screenshot: bool = True,
    width: int | None = None,
    height: int | None = None,
) -> dict[str, Any]:
    def gui_task():
        try:
            view = FreeCADGui.ActiveDocument.ActiveView
            if not hasattr(view, "viewIsometric"):
                return "No 3D view available"
            view.viewIsometric()
            view.fitAll()
            FreeCADGui.updateGui()
            return True
        except Exception as e:
            FreeCAD.Console.PrintError(f"reset_view error: {e}\n")
            return str(e)

    rpc_request_queue.put(lambda: _engage_viewport_lock() or True)
    rpc_response_queue.get(timeout=self.TIMEOUT)

    rpc_request_queue.put(gui_task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)

    if res is not True:
        rpc_request_queue.put(lambda: _release_viewport_lock() or True)
        try:
            rpc_response_queue.get(timeout=self.TIMEOUT)
        except Exception:
            pass
        return {"success": False, "error": res, "screenshot": None}

    if not take_screenshot:
        rpc_request_queue.put(lambda: _release_viewport_lock() or True)
        try:
            rpc_response_queue.get(timeout=self.TIMEOUT)
        except Exception:
            pass
        return {"success": True, "error": None, "screenshot": None}

    fd, tmp_path = tempfile.mkstemp(suffix=".png")
    os.close(fd)
    rpc_request_queue.put(
        lambda: self._capture_current_view_gui(tmp_path, width, height)
    )
    cap_res = rpc_response_queue.get(timeout=self.TIMEOUT)

    rpc_request_queue.put(lambda: _release_viewport_lock() or True)
    try:
        rpc_response_queue.get(timeout=self.TIMEOUT)
    except Exception:
        pass

    if cap_res is True:
        try:
            encoded = _read_valid_screenshot(tmp_path)
        finally:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)
        return {"success": True, "error": None, "screenshot": encoded}

    if os.path.exists(tmp_path):
        os.remove(tmp_path)
    return {"success": True, "error": None, "screenshot": None}
