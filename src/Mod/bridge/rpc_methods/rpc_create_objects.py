from __future__ import annotations

from rpc import *


@rpc_method
def create_objects(
    self,
    doc_name: str,
    items: list[dict[str, Any]],
    recompute: bool = True,
) -> dict[str, Any]:
    if not isinstance(items, list) or not items:
        return {"success": False, "error": "'items' must be a non-empty list"}

    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            created: list[dict[str, Any]] = []
            errors: list[dict[str, Any]] = []

            for index, raw in enumerate(items):
                if not isinstance(raw, dict):
                    errors.append({"index": index, "error": "item must be a dict"})
                    continue
                obj_spec = _Object(
                    name=raw.get("Name") or raw.get("name") or f"BatchObject_{index}",
                    type=raw.get("Type") or raw.get("type"),
                    analysis=raw.get("Analysis") or raw.get("analysis"),
                    properties=raw.get("Properties") or raw.get("properties") or {},
                )
                if not obj_spec.type:
                    errors.append(
                        {
                            "index": index,
                            "name": obj_spec.name,
                            "error": "missing 'Type'",
                        }
                    )
                    continue
                res = self._create_object_gui(doc_name, obj_spec)
                if res is True:
                    created.append(
                        {
                            "index": index,
                            "name": obj_spec.name,
                            "type": obj_spec.type,
                        }
                    )
                else:
                    errors.append(
                        {"index": index, "name": obj_spec.name, "error": str(res)}
                    )

            if recompute:
                doc.recompute()

            return {
                "created": created,
                "errors": errors,
                "created_count": len(created),
                "error_count": len(errors),
            }
        except Exception as e:
            return f"create_objects error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
