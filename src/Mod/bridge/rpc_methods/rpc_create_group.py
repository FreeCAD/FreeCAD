from __future__ import annotations

from rpc import *


@rpc_method
def create_group(
    self,
    doc_name: str,
    name: str,
    members: list[str] | None = None,
    parent: str | None = None,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            group = doc.addObject("App::DocumentObjectGroup", name)
            added: list[str] = []
            missing: list[str] = []
            if members:
                for ident in members:
                    ref, _ = _resolve_target(doc, ident)
                    if ref is None:
                        missing.append(ident)
                        continue
                    try:
                        group.addObject(ref)
                        added.append(ref.Name)
                    except Exception as e:
                        missing.append(f"{ident} ({e})")

            if parent:
                parent_ref, _ = _resolve_target(doc, parent)
                if parent_ref is None:
                    missing.append(f"parent:{parent}")
                else:
                    try:
                        parent_ref.addObject(group)
                    except Exception as e:
                        missing.append(f"parent:{parent} ({e})")

            doc.recompute()
            return {
                "name": group.Name,
                "added": added,
                "missing": missing,
            }
        except Exception as e:
            return f"create_group error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
