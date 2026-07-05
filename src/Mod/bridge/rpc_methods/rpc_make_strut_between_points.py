from __future__ import annotations

from rpc import *


@rpc_method
def make_strut_between_points(
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

            p1 = params.get("p1") or params.get("point1") or params.get("start")
            p2 = params.get("p2") or params.get("point2") or params.get("end")
            radius = float(params.get("radius"))
            end_caps = str(params.get("end_caps", "flat")).lower()

            shape = _build_strut_shape(p1, p2, radius, end_caps)
            obj = doc.addObject("Part::Feature", name)
            obj.Shape = shape

            if recompute:
                doc.recompute()

            v1 = _vec_from(p1)
            v2 = _vec_from(p2)
            length = (v2 - v1).Length
            return {
                "name": obj.Name,
                "p1": _vec_to_dict(v1),
                "p2": _vec_to_dict(v2),
                "radius": radius,
                "length": length,
                "end_caps": end_caps,
            }
        except Exception as e:
            return f"make_strut_between_points error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
