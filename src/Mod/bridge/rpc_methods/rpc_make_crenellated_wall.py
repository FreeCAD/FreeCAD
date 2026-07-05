from __future__ import annotations

from rpc import *


@rpc_method
def make_crenellated_wall(
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
            shape = _build_crenellated_wall_shape(
                float(params.get("length")),
                float(params.get("thickness")),
                float(params.get("height")),
                float(params.get("merlon_width")),
                float(params.get("gap_width", params.get("crenel_width", 0.0))),
                float(params.get("merlon_height")),
                bool(params.get("start_with_merlon", True)),
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
                "thickness": float(params.get("thickness")),
                "height": float(params.get("height")),
                "merlon_width": float(params.get("merlon_width")),
                "gap_width": float(
                    params.get("gap_width", params.get("crenel_width", 0.0))
                ),
                "merlon_height": float(params.get("merlon_height")),
            }
        except Exception as e:
            return f"make_crenellated_wall error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
