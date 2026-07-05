from __future__ import annotations

from rpc import *


@rpc_method
def clear_sketch_constraints(
    self,
    doc_name: str,
    sketch_name: str,
    indices: list[int] | None = None,
    recompute: bool = True,
) -> dict[str, Any]:
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
                return f"Object '{sketch_name}' is not a Sketcher::SketchObject"

            deleted: list[int] = []
            if indices is None:
                total = len(list(getattr(sketch, "Constraints", []) or []))
                for idx in sorted(range(total), reverse=True):
                    sketch.delConstraint(int(idx))
                    deleted.append(int(idx))
            else:
                if not isinstance(indices, list):
                    return "'indices' must be a list of integers"
                unique_sorted = sorted({int(i) for i in indices}, reverse=True)
                for idx in unique_sorted:
                    sketch.delConstraint(idx)
                    deleted.append(idx)

            if recompute:
                sketch.recompute()
                doc.recompute()

            return {
                "deleted_indices": sorted(set(deleted)),
                "constraint_count": len(list(getattr(sketch, "Constraints", []) or [])),
            }
        except Exception as e:
            return f"clear_sketch_constraints error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
