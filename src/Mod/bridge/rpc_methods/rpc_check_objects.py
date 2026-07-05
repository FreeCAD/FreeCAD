from __future__ import annotations

from rpc import *


@rpc_method
def check_objects(
    self,
    doc_name: str,
    obj_names: list[str] | None = None,
    only_unhealthy: bool = False,
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
                    ref = doc.getObject(name)
                    if ref is None:
                        missing.append(name)
                    else:
                        targets.append(ref)
            else:
                targets = list(doc.Objects)
                missing = []

            all_reports = [_object_health(o) for o in targets]
            unhealthy_total = sum(1 for r in all_reports if not r.get("healthy", True))
            reports = (
                [r for r in all_reports if not r.get("healthy", True)]
                if only_unhealthy
                else all_reports
            )

            return {
                "document": doc.Name,
                "object_count": len(list(doc.Objects)),
                "checked_count": len(targets),
                "unhealthy_count": unhealthy_total,
                "missing_objects": missing,
                "objects": reports,
            }
        except Exception as e:
            return f"check_objects error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
