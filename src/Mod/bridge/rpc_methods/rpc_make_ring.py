from __future__ import annotations

from rpc import *


@rpc_method
def make_ring(
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

            outer_radius = float(params.get("outer_radius"))
            inner_radius = float(params.get("inner_radius", 0.0))
            thickness = float(params.get("thickness"))
            tilt_deg = float(params.get("tilt_deg", 0.0))

            shape = _build_ring_shape(outer_radius, inner_radius, thickness, tilt_deg)
            obj = doc.addObject("Part::Feature", name)
            obj.Shape = shape

            placement = _placement_from_dict(params.get("placement"))
            if placement is not None:
                obj.Placement = placement.multiply(obj.Placement)

            if recompute:
                doc.recompute()

            return {
                "name": obj.Name,
                "outer_radius": outer_radius,
                "inner_radius": inner_radius,
                "thickness": thickness,
                "tilt_deg": tilt_deg,
            }
        except Exception as e:
            return f"make_ring error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
