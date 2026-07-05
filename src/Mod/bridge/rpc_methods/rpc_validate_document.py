from __future__ import annotations

from rpc import *


@rpc_method
def validate_document(
    self,
    doc_name: str,
    deep_geometry: bool = False,
    run_bop_check: bool = False,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            objects = list(doc.Objects)
            health_reports = [_object_health(o) for o in objects]
            unhealthy = [r for r in health_reports if not r.get("healthy", True)]

            geometry_reports: list[dict[str, Any]] = []
            geometry_invalid = 0
            if deep_geometry:
                for o in objects:
                    shape = getattr(o, "Shape", None)
                    if shape is None:
                        continue
                    diagnostics = _geometry_diagnostics(
                        shape, run_bop_check, 1e-7, 1e-9
                    )
                    if not diagnostics.get("valid", True):
                        geometry_invalid += 1
                        geometry_reports.append(
                            {
                                "name": o.Name,
                                "type_id": str(getattr(o, "TypeId", "")),
                                "geometry": diagnostics,
                            }
                        )

            dangling = _dangling_links(objects)
            cycles = _dependency_cycles(objects)

            touched = [
                o.Name for o in objects if "Touched" in (getattr(o, "State", []) or [])
            ]

            problems: list[str] = []
            if unhealthy:
                problems.append(f"{len(unhealthy)} unhealthy object(s)")
            if geometry_invalid:
                problems.append(f"{geometry_invalid} object(s) with invalid geometry")
            if dangling:
                problems.append(f"{len(dangling)} dangling link(s)")
            if cycles:
                problems.append(f"{len(cycles)} dependency cycle(s)")
            if touched:
                problems.append(f"{len(touched)} object(s) need recompute")

            return {
                "document": doc.Name,
                "object_count": len(objects),
                "valid": len(problems) == 0,
                "problem_summary": problems,
                "unhealthy_count": len(unhealthy),
                "unhealthy_objects": unhealthy,
                "geometry_invalid_count": geometry_invalid,
                "geometry_problems": geometry_reports,
                "dangling_links": dangling,
                "dependency_cycles": cycles,
                "touched_objects": touched,
                "deep_geometry": bool(deep_geometry),
            }
        except Exception as e:
            return f"validate_document error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
