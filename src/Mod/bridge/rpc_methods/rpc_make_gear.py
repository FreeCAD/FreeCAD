from __future__ import annotations

from rpc import *


@rpc_method
def make_gear(
    self,
    doc_name: str,
    name: str,
    params: dict[str, Any],
    recompute: bool = True,
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            module = float(params.get("module"))
            tooth_count = int(params.get("tooth_count"))
            thickness = float(params.get("thickness"))
            pressure_angle_deg = float(params.get("pressure_angle_deg", 20.0))
            hub_radius = float(params.get("hub_radius", 0.0))
            bore_radius = float(params.get("bore_radius", 0.0))
            helix_angle_deg = float(params.get("helix_angle_deg", 0.0))

            shape = _build_gear_shape(
                module,
                tooth_count,
                thickness,
                pressure_angle_deg,
                hub_radius,
                bore_radius,
                helix_angle_deg,
            )
            obj = doc.addObject("Part::Feature", name)
            obj.Shape = shape

            placement = _placement_from_dict(params.get("placement"))
            if placement is not None:
                obj.Placement = placement.multiply(obj.Placement)

            if recompute:
                doc.recompute()

            pitch_radius = module * tooth_count / 2.0
            outer_radius = pitch_radius + module
            root_radius = max(pitch_radius - 1.25 * module, 0.0)
            return {
                "name": obj.Name,
                "module": module,
                "tooth_count": tooth_count,
                "thickness": thickness,
                "pressure_angle_deg": pressure_angle_deg,
                "pitch_radius": pitch_radius,
                "outer_radius": outer_radius,
                "root_radius": root_radius,
                "hub_radius": hub_radius,
                "bore_radius": bore_radius,
                "helix_angle_deg": helix_angle_deg,
            }
        except Exception as e:
            return f"make_gear error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
