from __future__ import annotations

from rpc import *


@rpc_method
def check_screenshot_support(self) -> dict[str, Any]:
    def task():
        try:
            adoc = getattr(FreeCADGui, "ActiveDocument", None)
            view = getattr(adoc, "ActiveView", None) if adoc else None
            if not adoc or view is None:
                return {"__ok__": {"supported": False}}
            view_type = type(view).__name__
            unsupported = (
                "SpreadsheetGui::SheetView",
                "DrawingGui::DrawingView",
                "TechDrawGui::MDIViewPage",
            )
            supported = view_type not in unsupported and hasattr(view, "saveImage")
            return {"__ok__": {"supported": bool(supported)}}
        except Exception as e:
            return {"__error__": str(e)}

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and "__ok__" in res:
        return {"success": True, **res["__ok__"]}
    if isinstance(res, dict) and "__error__" in res:
        return {"success": False, "error": res["__error__"]}
    return {"success": False, "error": str(res)}
