from __future__ import annotations

from rpc import *


@rpc_method
def get_world_state(
    self,
    doc_name: str | None = None,
    include_feature_tree: bool = True,
    include_geometry: bool = True,
    max_objects: int = 400,
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

            active = FreeCAD.ActiveDocument
            active_name = active.Name if active is not None else None

            documents: list[dict[str, Any]] = []
            for d in docs:
                objects = list(d.Objects)
                truncated = False
                if max_objects > 0 and len(objects) > max_objects:
                    objects = objects[:max_objects]
                    truncated = True

                object_entries: list[dict[str, Any]] = []
                unhealthy = 0
                category_counts: dict[str, int] = {}
                union_bbox: dict[str, float] | None = None

                for obj in objects:
                    type_id = str(getattr(obj, "TypeId", ""))
                    category = _feature_category(type_id)
                    category_counts[category] = category_counts.get(category, 0) + 1
                    health = _object_health(obj)
                    issues = health.get("issues", []) or []
                    if issues:
                        unhealthy += 1

                    entry: dict[str, Any] = {
                        "name": obj.Name,
                        "label": getattr(obj, "Label", obj.Name),
                        "type_id": type_id,
                        "category": category,
                        "visible": bool(getattr(obj, "Visibility", False)),
                        "issues": issues,
                    }

                    placement = getattr(obj, "Placement", None)
                    if isinstance(placement, FreeCAD.Placement):
                        pos = placement.Base
                        axis = placement.Rotation.Axis
                        entry["placement"] = {
                            "position": [
                                float(pos.x),
                                float(pos.y),
                                float(pos.z),
                            ],
                            "rotation_axis": [
                                float(axis.x),
                                float(axis.y),
                                float(axis.z),
                            ],
                            "rotation_deg": float(placement.Rotation.Angle)
                            * 180.0
                            / math.pi,
                        }

                    if include_geometry:
                        shape_info = health.get("shape")
                        if isinstance(shape_info, dict):
                            bbox = shape_info.get("bbox")
                            entry["geometry"] = {
                                "volume": shape_info.get("volume"),
                                "area": shape_info.get("area"),
                                "solid_count": shape_info.get("solid_count"),
                                "face_count": shape_info.get("face_count"),
                                "edge_count": shape_info.get("edge_count"),
                                "bbox": bbox,
                            }
                            if isinstance(bbox, dict):
                                union_bbox = _merge_bbox(union_bbox, bbox)

                    object_entries.append(entry)

                doc_report: dict[str, Any] = {
                    "name": d.Name,
                    "label": getattr(d, "Label", d.Name),
                    "file_name": getattr(d, "FileName", "") or None,
                    "is_active": d.Name == active_name,
                    "modified": bool(getattr(d, "Modified", False)),
                    "object_count": len(list(d.Objects)),
                    "returned_object_count": len(object_entries),
                    "objects_truncated": truncated,
                    "unhealthy_count": unhealthy,
                    "category_counts": category_counts,
                    "model_bbox": union_bbox,
                    "objects": object_entries,
                }

                if include_feature_tree:
                    tracked: set[str] = set()
                    bodies = [
                        o
                        for o in d.Objects
                        if str(getattr(o, "TypeId", "")).startswith("PartDesign::Body")
                    ]
                    doc_report["bodies"] = [
                        _serialize_partdesign_body(b, tracked) for b in bodies
                    ]

                documents.append(doc_report)

            return {
                "active_document": active_name,
                "document_count": len(documents),
                "documents": documents,
            }
        except Exception as e:
            return f"get_world_state error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
