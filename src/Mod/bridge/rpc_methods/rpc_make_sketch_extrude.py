from __future__ import annotations

from rpc import *


@rpc_method
def make_sketch_extrude(
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
            profile = params.get("profile") or []
            depth = float(params.get("depth"))
            direction = params.get("direction")
            shape = _build_sketch_extrude_shape(profile, depth, direction)
            obj = doc.addObject("Part::Feature", name)
            obj.Shape = shape

            placement = _placement_from_dict(params.get("placement"))
            if placement is not None:
                obj.Placement = placement.multiply(obj.Placement)

            if recompute:
                doc.recompute()
            return {
                "name": obj.Name,
                "profile_segments": len(profile),
                "depth": depth,
            }
        except Exception as e:
            return f"make_sketch_extrude error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
