from __future__ import annotations

from rpc import *


@rpc_method
def get_feature_tree(
    self,
    doc_name: str,
    body_name: str | None = None,
    include_orphans: bool = True,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            bodies = []
            if body_name:
                body = doc.getObject(body_name)
                if body is None:
                    return f"Object '{body_name}' not found in '{doc_name}'"
                if not str(getattr(body, "TypeId", "")).startswith("PartDesign::Body"):
                    return f"Object '{body_name}' is not a PartDesign::Body (TypeId={body.TypeId})"
                bodies.append(body)
            else:
                for o in doc.Objects:
                    if str(getattr(o, "TypeId", "")).startswith("PartDesign::Body"):
                        bodies.append(o)

            tracked_in_body: set[str] = set()
            body_reports: list[dict[str, Any]] = []
            for body in bodies:
                body_reports.append(_serialize_partdesign_body(body, tracked_in_body))

            orphans = []
            if include_orphans:
                for o in doc.Objects:
                    if o.Name in tracked_in_body:
                        continue
                    type_id = str(getattr(o, "TypeId", ""))
                    if not (
                        type_id.startswith("PartDesign::")
                        or type_id.startswith("Part::")
                        or type_id.startswith("Sketcher::")
                    ):
                        continue
                    if type_id.startswith("PartDesign::Body"):
                        continue
                    orphans.append(_serialize_feature_node(o))

            return {
                "document": doc.Name,
                "body_count": len(body_reports),
                "bodies": body_reports,
                "orphan_features": orphans,
            }
        except Exception as e:
            return f"get_feature_tree error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
