from __future__ import annotations

from rpc import *


@rpc_method
def add_sketch_constraints(
    self,
    doc_name: str,
    sketch_name: str,
    constraints: list[dict[str, Any]],
    recompute: bool = True,
) -> dict[str, Any]:
    if not isinstance(constraints, list) or not constraints:
        return {"success": False, "error": "'constraints' must be a non-empty list"}

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

            added_indices: list[int] = []
            for entry in constraints:
                constraint = _build_sketch_constraint(entry)
                new_id = sketch.addConstraint(constraint)
                if isinstance(new_id, list):
                    added_indices.extend(int(i) for i in new_id)
                else:
                    added_indices.append(int(new_id))

                if entry.get("driving") is False and added_indices:
                    try:
                        sketch.setDriving(added_indices[-1], False)
                    except Exception:
                        pass

            if recompute:
                sketch.recompute()
                doc.recompute()

            return {
                "added_indices": added_indices,
                "constraint_count": len(list(getattr(sketch, "Constraints", []) or [])),
            }
        except Exception as e:
            return f"add_sketch_constraints error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
