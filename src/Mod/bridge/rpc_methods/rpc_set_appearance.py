from __future__ import annotations

from rpc import *


@rpc_method
def set_appearance(
    self,
    doc_name: str,
    targets: list[str],
    properties: dict[str, Any],
) -> dict[str, Any]:
    if not isinstance(targets, list) or not targets:
        return {"success": False, "error": "'targets' must be a non-empty list"}
    if not isinstance(properties, dict) or not properties:
        return {"success": False, "error": "'properties' must be a non-empty dict"}

    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            applied: list[str] = []
            missing: list[str] = []
            errors: list[dict[str, Any]] = []

            for ident in targets:
                ref, _ = _resolve_target(doc, ident)
                if ref is None:
                    missing.append(ident)
                    continue
                view = getattr(ref, "ViewObject", None)
                if view is None:
                    errors.append({"object": ref.Name, "error": "no ViewObject"})
                    continue

                failed_props: list[str] = []
                for prop, value in properties.items():
                    try:
                        if prop in ("ShapeColor", "LineColor", "PointColor"):
                            if isinstance(value, (list, tuple)) and len(value) >= 3:
                                rgba = (
                                    float(value[0]),
                                    float(value[1]),
                                    float(value[2]),
                                    float(value[3]) if len(value) >= 4 else 1.0,
                                )
                                setattr(view, prop, rgba)
                            else:
                                setattr(view, prop, value)
                        elif prop in ("Transparency", "LineWidth", "PointSize"):
                            setattr(view, prop, value)
                        elif prop == "Visibility":
                            view.Visibility = bool(value)
                        elif prop == "DisplayMode":
                            _apply_display_mode(view, str(value))
                        else:
                            if hasattr(view, prop):
                                setattr(view, prop, value)
                            else:
                                failed_props.append(prop)
                    except Exception as e:
                        failed_props.append(f"{prop}({e})")

                if failed_props:
                    errors.append({"object": ref.Name, "failed": failed_props})
                applied.append(ref.Name)

            return {
                "applied": applied,
                "missing": missing,
                "errors": errors,
            }
        except Exception as e:
            return f"set_appearance error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
