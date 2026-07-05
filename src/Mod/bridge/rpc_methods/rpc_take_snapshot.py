from __future__ import annotations

from rpc import *


@rpc_method
def take_snapshot(
    self,
    doc_name: str,
    include_subelements: bool = False,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            tracked: set[int] = set()
            roots = []
            in_group: set[str] = set()
            for o in doc.Objects:
                for child in getattr(o, "Group", []) or []:
                    in_group.add(child.Name)

            for o in doc.Objects:
                if o.Name in in_group:
                    continue
                if id(o) in tracked:
                    continue
                tracked.add(id(o))
                roots.append(_serialize_snapshot_node(o, doc.Name, include_subelements))

            index: list[dict[str, Any]] = []

            def _walk(node: dict[str, Any]):
                index.append(
                    {
                        "uid": node["uid"],
                        "name": node["name"],
                        "label": node.get("label"),
                        "type_id": node.get("type_id"),
                        "category": node.get("category"),
                        "visible": node.get("visible"),
                        "display_mode": node.get("display_mode"),
                    }
                )
                for face in node.get("faces", []) or []:
                    index.append(
                        {
                            "uid": face["uid"],
                            "ref": face["ref"],
                            "kind": "Face",
                        }
                    )
                for edge in node.get("edges", []) or []:
                    index.append(
                        {
                            "uid": edge["uid"],
                            "ref": edge["ref"],
                            "kind": "Edge",
                        }
                    )
                for vertex in node.get("vertices", []) or []:
                    index.append(
                        {
                            "uid": vertex["uid"],
                            "ref": vertex["ref"],
                            "kind": "Vertex",
                        }
                    )
                for child in node.get("children", []) or []:
                    _walk(child)

            for r in roots:
                _walk(r)

            return {
                "document": doc.Name,
                "object_count": len(list(doc.Objects)),
                "include_subelements": bool(include_subelements),
                "tree": roots,
                "index": index,
            }
        except Exception as e:
            return f"take_snapshot error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
