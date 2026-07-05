from __future__ import annotations

from rpc import *


@rpc_method
def check_interferences(
    self,
    doc_name: str,
    obj_names: list[str] | None = None,
    clearance: float = 0.0,
    compute_volume: bool = True,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            if obj_names:
                candidates = []
                missing: list[str] = []
                for name in obj_names:
                    ref, _ = _resolve_target(doc, name)
                    if ref is None:
                        missing.append(name)
                    else:
                        candidates.append(ref)
            else:
                candidates = []
                missing = []
                for o in doc.Objects:
                    shape = getattr(o, "Shape", None)
                    if shape is None:
                        continue
                    try:
                        if len(shape.Solids) == 0:
                            continue
                    except Exception:
                        continue
                    if not bool(getattr(o, "Visibility", True)):
                        continue
                    candidates.append(o)

            solids = []
            for o in candidates:
                shape = getattr(o, "Shape", None)
                if shape is None:
                    continue
                try:
                    if shape.isNull() or len(shape.Solids) == 0:
                        continue
                except Exception:
                    continue
                solids.append((o.Name, shape))

            gap = float(clearance)
            pairs: list[dict[str, Any]] = []
            for i in range(len(solids)):
                name_a, shape_a = solids[i]
                bb_a = shape_a.BoundBox
                for j in range(i + 1, len(solids)):
                    name_b, shape_b = solids[j]
                    if not _bbox_overlap(bb_a, shape_b.BoundBox, gap):
                        continue

                    try:
                        dist_info = shape_a.distToShape(shape_b)
                        distance = float(dist_info[0])
                    except Exception as e:
                        pairs.append(
                            {
                                "a": name_a,
                                "b": name_b,
                                "status": "error",
                                "error": str(e),
                            }
                        )
                        continue

                    tolerance = 1e-7
                    penetration_points = _points_inside_shape(
                        shape_b, _shape_sample_points(shape_a), tolerance
                    )
                    if not penetration_points:
                        penetration_points = _points_inside_shape(
                            shape_a, _shape_sample_points(shape_b), tolerance
                        )

                    if penetration_points:
                        entry: dict[str, Any] = {
                            "a": name_a,
                            "b": name_b,
                            "status": "interference",
                            "distance": distance,
                        }
                        if compute_volume:
                            entry["interior_sample_count"] = len(penetration_points)
                            xs = [p.x for p in penetration_points]
                            ys = [p.y for p in penetration_points]
                            zs = [p.z for p in penetration_points]
                            entry["overlap_region"] = {
                                "x_min": min(xs),
                                "x_max": max(xs),
                                "y_min": min(ys),
                                "y_max": max(ys),
                                "z_min": min(zs),
                                "z_max": max(zs),
                            }
                        pairs.append(entry)
                    elif distance <= 1e-7:
                        pairs.append(
                            {
                                "a": name_a,
                                "b": name_b,
                                "status": "contact",
                                "distance": distance,
                            }
                        )
                    elif gap > 0.0 and distance <= gap:
                        pairs.append(
                            {
                                "a": name_a,
                                "b": name_b,
                                "status": "clearance_violation",
                                "distance": distance,
                                "clearance": gap,
                            }
                        )

            interference_count = sum(
                1 for p in pairs if p.get("status") == "interference"
            )
            contact_count = sum(1 for p in pairs if p.get("status") == "contact")
            clearance_count = sum(
                1 for p in pairs if p.get("status") == "clearance_violation"
            )
            return {
                "document": doc.Name,
                "solid_count": len(solids),
                "pair_count": len(solids) * (len(solids) - 1) // 2,
                "interference_count": interference_count,
                "contact_count": contact_count,
                "clearance_violation_count": clearance_count,
                "missing_objects": missing,
                "pairs": pairs,
            }
        except Exception as e:
            return f"check_interferences error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
