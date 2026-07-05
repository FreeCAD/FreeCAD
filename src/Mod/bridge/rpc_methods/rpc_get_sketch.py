from __future__ import annotations

from rpc import *


@rpc_method
def get_sketch(self, doc_name: str, sketch_name: str) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"
            sketch = doc.getObject(sketch_name)
            if not sketch:
                return f"Object '{sketch_name}' not found in '{doc_name}'"
            if not str(getattr(sketch, "TypeId", "")).startswith(
                "Sketcher::SketchObject"
            ):
                return f"Object '{sketch_name}' is not a Sketcher::SketchObject (TypeId={sketch.TypeId})"
            return _serialize_full_sketch(sketch)
        except Exception as e:
            return f"get_sketch error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, "sketch": res}
    return {"success": False, "error": str(res)}
