from __future__ import annotations

from rpc import *


@rpc_method
def measure_distance(
    self,
    doc_name: str,
    a,
    b,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            point_a, label_a = _point_for_distance(doc, a)
            point_b, label_b = _point_for_distance(doc, b)
            if point_a is None or point_b is None:
                return f"Could not resolve point for {label_a if point_a is None else label_b}"

            delta = point_b - point_a
            distance = float(delta.Length)
            return {
                "a": label_a,
                "b": label_b,
                "point_a": _vec_to_dict(point_a),
                "point_b": _vec_to_dict(point_b),
                "delta": _vec_to_dict(delta),
                "distance": distance,
                "distance_x": float(abs(delta.x)),
                "distance_y": float(abs(delta.y)),
                "distance_z": float(abs(delta.z)),
            }
        except Exception as e:
            return f"measure_distance error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
