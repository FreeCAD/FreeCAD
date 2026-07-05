import json
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def verify_solid(
    ctx: Context,
    doc_name: str,
    target: str,
    samples: int = 8,
    tolerance: float = 1e-7,
    expected_solids: int = 1,
    min_fill_ratio: float = 0.0,
    max_volume_discrepancy: float = 0.25,
) -> list[TextContent]:
    """Introspectively verify that a shape is a real, filled solid.

    Detects the silent "valid but hollow" failure mode where the kernel reports
    isValid=True and one solid, yet the body is an empty shell (for example
    after a boolean that dropped an operand). Instead of trusting the kernel's
    self-report, this samples the interior directly: it builds a 3D grid inside
    the bounding box, tests each point with Solid.isInside, walks the three
    axis centerlines, checks whether the center of mass lies inside the body,
    and compares the kernel's reported volume against the volume implied by the
    interior sampling.

    Returns passed (bool), a list of hard failures, a list of warnings, and a
    full metrics block (solid_count, shells_per_solid, volume, area, bbox,
    fill_ratio, grid sampling counts, grid_volume_estimate, volume_discrepancy,
    per-axis interior fractions, and center_of_mass_inside).

    Hard failures (passed=False): no solids, zero/negative volume, the volume
    could not be computed, or not a single interior sample point fell inside the
    shape (hollow). Warnings flag suspicious-but-not-fatal conditions: unexpected
    solid count, fill ratio below min_fill_ratio, volume discrepancy above
    max_volume_discrepancy, extra shells (internal voids), a center of mass
    outside the body, or an axis centerline with no interior hits.

    Args:
        doc_name: Document containing the object.
        target: Object name, 'Object#Solid1' subelement reference, or a
                snapshot uid.
        samples: Grid resolution per axis for interior sampling (clamped to
                 2..40). Higher values are more thorough but slower. Default 8.
        tolerance: isInside tolerance in mm. Default 1e-7.
        expected_solids: Solid count to expect; a mismatch is reported as a
                         warning. Default 1.
        min_fill_ratio: Minimum acceptable volume/bbox_volume ratio. 0 disables
                        the check (default).
        max_volume_discrepancy: Maximum allowed relative gap between the reported
                                volume and the interior-sampled volume before a
                                warning is raised. Default 0.25 (25%).
    """
    try:
        res = get_parashell_connection().verify_solid(
            doc_name,
            target,
            samples,
            tolerance,
            expected_solids,
            min_fill_ratio,
            max_volume_discrepancy,
        )
        if not res.get("success"):
            return text_response(f"Failed to verify solid: {res.get('error')}")

        metrics = res.get("metrics") or {}
        passed = res.get("passed")
        failures = res.get("failures") or []
        warnings = res.get("warnings") or []
        volume = metrics.get("volume")
        vol_text = f"{volume:.6g}" if isinstance(volume, (int, float)) else "n/a"
        fill = metrics.get("fill_ratio")
        fill_text = f"{fill * 100:.1f}%" if isinstance(fill, (int, float)) else "n/a"
        inside = metrics.get("inside_points")
        grid = metrics.get("grid_points")
        status = "PASS" if passed else "FAIL"
        header = (
            f"verify_solid {status} for '{res.get('object')}': "
            f"solids={metrics.get('solid_count')}, volume={vol_text} mm^3, "
            f"fill={fill_text}, interior samples {inside}/{grid}, "
            f"{len(failures)} failure(s), {len(warnings)} warning(s)."
        )
        payload = {k: v for k, v in res.items() if k != "success"}
        return [
            TextContent(type="text", text=header),
            TextContent(
                type="text",
                text=json.dumps(payload, ensure_ascii=False, indent=2, default=str),
            ),
        ]
    except Exception as e:
        logger.error(f"Failed to verify solid: {e}")
        return text_response(f"Failed to verify solid: {e}")
