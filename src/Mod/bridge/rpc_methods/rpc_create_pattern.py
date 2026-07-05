from __future__ import annotations

from rpc import *


@rpc_method
def create_pattern(
    self,
    doc_name: str,
    source_object: str,
    kind: str,
    count: int,
    params: dict[str, Any] | None = None,
    copy_mode: str = "duplicate",
    name_prefix: str | None = None,
    recompute: bool = True,
) -> dict[str, Any]:
    params = params or {}

    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"
            source, _ = _resolve_target(doc, source_object)
            if source is None:
                return f"Source object '{source_object}' not found in '{doc_name}'"

            placements = _compute_pattern_placements(kind, int(count), params)
            if not placements:
                return "Pattern produced no placements"

            base_placement = getattr(source, "Placement", FreeCAD.Placement())
            created_names: list[str] = []
            prefix = name_prefix or f"{source.Name}_{kind.lower()}_"

            if copy_mode == "duplicate":
                import Part

                if not hasattr(source, "Shape") or source.Shape is None:
                    return f"Source '{source.Name}' has no Shape — cannot duplicate"
                base_shape = source.Shape
                base_inv = base_placement.inverse()
                local_shape = base_shape.copy()
                local_shape.Placement = base_inv.multiply(base_shape.Placement)

                for i, p in enumerate(placements):
                    new_obj = doc.addObject("Part::Feature", f"{prefix}{i + 1}")
                    copy = local_shape.copy()
                    new_obj.Shape = copy
                    new_obj.Placement = p.multiply(base_placement)
                    created_names.append(new_obj.Name)
            elif copy_mode == "link":
                for i, p in enumerate(placements):
                    link = doc.addObject("App::Link", f"{prefix}{i + 1}")
                    link.LinkedObject = source
                    link.Placement = p.multiply(base_placement)
                    created_names.append(link.Name)
            else:
                return f"Unknown copy_mode '{copy_mode}'. Use 'duplicate' or 'link'."

            if recompute:
                doc.recompute()

            return {
                "source": source.Name,
                "kind": kind,
                "count": len(created_names),
                "created": created_names,
            }
        except Exception as e:
            return f"create_pattern error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
