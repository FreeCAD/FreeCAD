from __future__ import annotations

from rpc import *


@rpc_method
def verify_solid(
    self,
    doc_name: str,
    target: str,
    samples: int = 8,
    tolerance: float = 1e-7,
    expected_solids: int = 1,
    min_fill_ratio: float = 0.0,
    max_volume_discrepancy: float = 0.25,
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

            metrics = _solid_interior_metrics(shape, samples, tolerance)

            failures: list[str] = []
            warnings: list[str] = []

            solid_count = metrics["solid_count"]
            if solid_count == 0:
                failures.append(
                    "shape contains no solids; it is a surface, wire, or empty shape"
                )
            elif expected_solids and solid_count != int(expected_solids):
                warnings.append(
                    f"expected {int(expected_solids)} solid(s) but found {solid_count}"
                )

            volume = metrics["volume"]
            if volume is None:
                failures.append("volume could not be computed")
            elif volume <= tolerance:
                failures.append(f"volume {volume:.6f} is zero or negative")

            inside_fraction = metrics["inside_fraction"]
            if (
                metrics["grid_points"]
                and inside_fraction is not None
                and inside_fraction <= 0.0
            ):
                failures.append(
                    "no interior sample point fell inside the shape; geometry is hollow or degenerate"
                )

            fill_ratio = metrics["fill_ratio"]
            if (
                fill_ratio is not None
                and min_fill_ratio > 0.0
                and fill_ratio < min_fill_ratio
            ):
                warnings.append(
                    f"fill ratio {fill_ratio:.4f} is below the requested minimum {min_fill_ratio:.4f}"
                )

            discrepancy = metrics["volume_discrepancy"]
            if discrepancy is not None and discrepancy > max_volume_discrepancy:
                warnings.append(
                    f"reported volume and interior-sampled volume disagree by {discrepancy * 100:.1f}% "
                    f"(threshold {max_volume_discrepancy * 100:.1f}%); the shell may not match the "
                    f"reported solid"
                )

            if metrics["extra_shells"] > 0:
                warnings.append(
                    f"{metrics['extra_shells']} extra shell(s) detected across solids; the result may "
                    f"contain internal voids"
                )

            com_inside = metrics["center_of_mass_inside"]
            if solid_count == 1 and com_inside is False:
                warnings.append(
                    "center of mass lies outside the solid; the body may be hollow, non-convex, or split"
                )

            for axis_sample in metrics["axis_samples"]:
                if axis_sample["samples"] and axis_sample["inside"] == 0:
                    warnings.append(
                        f"no interior point found along the {axis_sample['axis']}-axis centerline"
                    )

            passed = len(failures) == 0
            return {
                "object": ref.Name,
                "subelement": sub or "",
                "passed": passed,
                "failures": failures,
                "warnings": warnings,
                "metrics": metrics,
            }
        except Exception as e:
            return f"verify_solid error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
