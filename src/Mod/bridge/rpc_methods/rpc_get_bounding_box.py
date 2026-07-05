from __future__ import annotations

from rpc import *


@rpc_method
def get_bounding_box(
    self,
    doc_name: str,
    target: str,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"
            ref, sub = _resolve_target(doc, target)
            if ref is None:
                return f"Object '{target}' not found in '{doc_name}'"
            shape = _resolve_subshape(ref, sub)
            if shape is None:
                return f"Object '{ref.Name}' has no Shape"
            bb = _bbox_dict_full(shape.BoundBox)
            return {
                "object": ref.Name,
                "subelement": sub or "",
                "bbox": bb,
            }
        except Exception as e:
            return f"get_bounding_box error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
