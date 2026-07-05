from __future__ import annotations

from rpc import *


@rpc_method
def check_geometry(
    self,
    doc_name: str,
    obj_names: list[str] | None = None,
    run_bop_check: bool = True,
    min_edge_length: float = 1e-7,
    min_face_area: float = 1e-9,
    only_invalid: bool = False,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            if obj_names:
                targets = []
                missing: list[str] = []
                for name in obj_names:
                    ref, _ = _resolve_target(doc, name)
                    if ref is None:
                        missing.append(name)
                    else:
                        targets.append(ref)
            else:
                targets = [
                    o for o in doc.Objects if getattr(o, "Shape", None) is not None
                ]
                missing = []

            reports: list[dict[str, Any]] = []
            invalid_total = 0
            no_shape: list[str] = []
            for obj in targets:
                shape = getattr(obj, "Shape", None)
                if shape is None:
                    no_shape.append(obj.Name)
                    continue
                diagnostics = _geometry_diagnostics(
                    shape,
                    run_bop_check,
                    float(min_edge_length),
                    float(min_face_area),
                )
                entry = {
                    "name": obj.Name,
                    "label": getattr(obj, "Label", obj.Name),
                    "type_id": str(getattr(obj, "TypeId", "")),
                    "geometry": diagnostics,
                }
                if not diagnostics.get("valid", True):
                    invalid_total += 1
                reports.append(entry)

            if only_invalid:
                reports = [r for r in reports if not r["geometry"].get("valid", True)]

            return {
                "document": doc.Name,
                "checked_count": len(targets) - len(no_shape),
                "invalid_count": invalid_total,
                "missing_objects": missing,
                "objects_without_shape": no_shape,
                "objects": reports,
            }
        except Exception as e:
            return f"check_geometry error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
