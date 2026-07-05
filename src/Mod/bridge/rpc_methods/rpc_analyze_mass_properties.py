from __future__ import annotations

from rpc import *


@rpc_method
def analyze_mass_properties(
    self,
    doc_name: str,
    target: str,
    density: float | None = None,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"
            ref, sub = _resolve_target(doc, target)
            if ref is None:
                return f"Object '{target}' not found in '{doc_name}'"
            shape = _resolve_subshape(ref, sub)
            if shape is None:
                return f"Object '{ref.Name}' has no Shape"

            def _safe(fn, default=None):
                try:
                    return fn()
                except Exception:
                    return default

            volume = _safe(lambda: float(shape.Volume))
            area = _safe(lambda: float(shape.Area))
            length = _safe(lambda: float(shape.Length))
            center = _safe(lambda: _vec_to_dict(shape.CenterOfMass))
            inertia = _safe(lambda: _serialize_matrix(shape.MatrixOfInertia))
            principal = _principal_properties(shape)

            result: dict[str, Any] = {
                "object": ref.Name,
                "subelement": sub or "",
                "shape_type": type(shape).__name__,
                "volume": volume,
                "area": area,
                "length": length,
                "center_of_mass": center,
                "matrix_of_inertia": inertia,
                "principal_properties": principal,
                "bbox": _bbox_dict_full(shape.BoundBox),
            }

            if density is not None and volume is not None:
                density_value = float(density)
                mass = volume * density_value
                result["density"] = density_value
                result["mass"] = mass
                moments = None
                if principal:
                    for moment_key in ("Moments", "MomentsOfInertia"):
                        candidate = principal.get(moment_key)
                        if isinstance(candidate, list):
                            moments = candidate
                            break
                if moments is not None:
                    result["principal_moments_mass_scaled"] = [
                        float(m) * density_value
                        for m in moments
                        if isinstance(m, (int, float))
                    ]

            return result
        except Exception as e:
            return f"analyze_mass_properties error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
