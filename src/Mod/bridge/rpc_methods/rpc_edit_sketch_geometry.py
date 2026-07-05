from __future__ import annotations

from rpc import *


@rpc_method
def edit_sketch_geometry(
    self,
    doc_name: str,
    sketch_name: str,
    operation: str = "append",
    geometry: list[dict[str, Any]] | None = None,
    delete_indices: list[int] | None = None,
    recompute: bool = True,
) -> dict[str, Any]:
    op = (operation or "append").strip().lower()
    if op not in ("append", "replace", "delete"):
        return {
            "success": False,
            "error": f"Unknown operation '{operation}'. Use 'append', 'replace', or 'delete'.",
        }

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
            deleted_indices: list[int] = []

            if op in ("replace", "delete"):
                if op == "replace":
                    target_count = len(list(getattr(sketch, "Geometry", []) or []))
                    for idx in sorted(range(target_count), reverse=True):
                        sketch.delGeometry(int(idx))
                        deleted_indices.append(int(idx))
                else:
                    if not delete_indices:
                        return "operation='delete' requires non-empty 'delete_indices'"
                    unique_sorted = sorted(
                        {int(i) for i in delete_indices}, reverse=True
                    )
                    for idx in unique_sorted:
                        sketch.delGeometry(idx)
                        deleted_indices.append(idx)

            if op in ("append", "replace"):
                entries = geometry or []
                for entry in entries:
                    is_construction = bool(entry.get("construction", False))
                    part_geom = _build_part_geometry(entry)
                    new_id = sketch.addGeometry(part_geom, is_construction)
                    if isinstance(new_id, list):
                        added_indices.extend(int(i) for i in new_id)
                    else:
                        added_indices.append(int(new_id))

            if recompute:
                sketch.recompute()
                doc.recompute()

            return {
                "added_indices": added_indices,
                "deleted_indices": sorted(set(deleted_indices)),
                "geometry_count": len(list(getattr(sketch, "Geometry", []) or [])),
            }
        except Exception as e:
            return f"edit_sketch_geometry error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
