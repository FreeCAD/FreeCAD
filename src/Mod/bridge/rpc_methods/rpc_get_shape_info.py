from __future__ import annotations

from rpc import *


@rpc_method
def get_shape_info(
    self,
    doc_name: str,
    obj_name: str,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"
            obj = doc.getObject(obj_name)
            if obj is None:
                return f"Object '{obj_name}' not found in '{doc_name}'"
            return _object_health(obj)
        except Exception as e:
            return f"get_shape_info error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, "object": res}
    return {"success": False, "error": str(res)}
