from __future__ import annotations

from rpc import *


@rpc_method
def make_pyramid(
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

            if "base_points" in params and params["base_points"]:
                if "apex" not in params or params["apex"] is None:
                    return "make_pyramid with base_points requires 'apex'"
                shape = _build_pyramid_shape(params["base_points"], params["apex"])
                base_kind = "polygon"
            else:
                base_radius = float(params.get("base_radius"))
                base_sides = int(params.get("base_sides", 4))
                height = float(params.get("height"))
                shape = _build_regular_pyramid_shape(
                    base_radius,
                    base_sides,
                    height,
                    float(params.get("base_rotation_deg", 0.0)),
                )
                base_kind = f"regular_{base_sides}"

            obj = doc.addObject("Part::Feature", name)
            obj.Shape = shape

            placement = _placement_from_dict(params.get("placement"))
            if placement is not None:
                obj.Placement = placement.multiply(obj.Placement)

            if recompute:
                doc.recompute()
            return {
                "name": obj.Name,
                "base": base_kind,
            }
        except Exception as e:
            return f"make_pyramid error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
