from __future__ import annotations

from rpc import *


@rpc_method
def recompute_document(
    self,
    doc_name: str | None = None,
    force: bool = False,
    clear_redo: bool = False,
) -> dict[str, Any]:
    def task():
        try:
            if doc_name:
                doc = FreeCAD.getDocument(doc_name)
                if not doc:
                    return f"Document '{doc_name}' not found"
                docs = [doc]
            else:
                docs = list(FreeCAD.listDocuments().values())
                if not docs:
                    return "No open documents to recompute"

            results: list[dict[str, Any]] = []
            for d in docs:
                try:
                    if clear_redo:
                        try:
                            d.clearRedos()
                        except Exception:
                            pass
                    try:
                        objects_recomputed = int(d.recompute(None, bool(force)))
                    except TypeError:
                        objects_recomputed = int(d.recompute())

                    unhealthy: list[dict[str, Any]] = []
                    for o in d.Objects:
                        health = _object_health(o)
                        if not health.get("healthy", True):
                            unhealthy.append(health)

                    results.append(
                        {
                            "document": d.Name,
                            "objects_recomputed": objects_recomputed,
                            "object_count": len(list(d.Objects)),
                            "unhealthy_count": len(unhealthy),
                            "unhealthy_objects": unhealthy,
                        }
                    )
                except Exception as e:
                    results.append(
                        {
                            "document": d.Name,
                            "error": str(e),
                        }
                    )

            return {"results": results}
        except Exception as e:
            return f"recompute_document error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
