from __future__ import annotations

from rpc import *


@rpc_method
def make_hollow_box(
    self,
    doc_name: str,
    name: str,
    params: dict[str, Any],
    recompute: bool = True,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"
            shape = _build_hollow_box_shape(
                float(params.get("length")),
                float(params.get("width")),
                float(params.get("height")),
                float(params.get("thickness")),
                bool(params.get("open_top", False)),
                bool(params.get("open_bottom", False)),
            )
            obj = doc.addObject("Part::Feature", name)
            obj.Shape = shape

            placement = _placement_from_dict(params.get("placement"))
            if placement is not None:
                obj.Placement = placement.multiply(obj.Placement)

            if recompute:
                doc.recompute()
            return {
                "name": obj.Name,
                "length": float(params.get("length")),
                "width": float(params.get("width")),
                "height": float(params.get("height")),
                "thickness": float(params.get("thickness")),
                "open_top": bool(params.get("open_top", False)),
                "open_bottom": bool(params.get("open_bottom", False)),
            }
        except Exception as e:
            return f"make_hollow_box error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
