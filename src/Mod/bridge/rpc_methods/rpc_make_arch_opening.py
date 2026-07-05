from __future__ import annotations

from rpc import *


@rpc_method
def make_arch_opening(
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
            arch_height = params.get("arch_height")
            shape = _build_arch_opening_shape(
                float(params.get("width")),
                float(params.get("height")),
                float(params.get("depth")),
                str(params.get("arch_kind", "round")),
                float(arch_height) if arch_height is not None else None,
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
                "width": float(params.get("width")),
                "height": float(params.get("height")),
                "depth": float(params.get("depth")),
                "arch_kind": str(params.get("arch_kind", "round")),
            }
        except Exception as e:
            return f"make_arch_opening error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
