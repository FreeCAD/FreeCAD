from __future__ import annotations

from rpc import *


@rpc_method
def _capture_current_view_gui(
    self,
    save_path: str,
    width: int | None = None,
    height: int | None = None,
):
    try:
        view = FreeCADGui.ActiveDocument.ActiveView
        if not hasattr(view, "saveImage"):
            return "Current view does not support screenshots"
        FreeCADGui.updateGui()
        if width is not None and height is not None:
            view.saveImage(save_path, width, height)
        else:
            view.saveImage(save_path)
        return True
    except Exception as e:
        return str(e)
