from __future__ import annotations

from rpc import *


@rpc_method
def remove_from_group(
    self,
    doc_name: str,
    group: str,
    members: list[str],
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"
            group_ref, _ = _resolve_target(doc, group)
            if group_ref is None:
                return f"Group '{group}' not found"
            if not hasattr(group_ref, "removeObject"):
                return f"'{group_ref.Name}' is not a group container"

            removed: list[str] = []
            missing: list[str] = []
            for ident in members:
                ref, _ = _resolve_target(doc, ident)
                if ref is None:
                    missing.append(ident)
                    continue
                try:
                    group_ref.removeObject(ref)
                    removed.append(ref.Name)
                except Exception as e:
                    missing.append(f"{ident} ({e})")

            doc.recompute()
            return {"group": group_ref.Name, "removed": removed, "missing": missing}
        except Exception as e:
            return f"remove_from_group error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
