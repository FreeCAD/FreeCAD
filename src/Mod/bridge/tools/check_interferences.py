import json
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def check_interferences(
    ctx: Context,
    doc_name: str,
    obj_names: list[str] | None = None,
    clearance: float = 0.0,
    compute_volume: bool = True,
) -> list[TextContent]:
    """Detect solid-on-solid clashes and clearance violations in an assembly.

    Performs read-only pairwise interference checking across the solids in the
    document using Parashell's measurement APIs only — no boolean geometry is
    constructed and the document is left completely unchanged. Bounding boxes
    skip non-overlapping pairs first; remaining pairs are tested with
    Shape.distToShape (gap measurement) and Solid.isInside point-containment on
    sampled vertices/edge/face centers. For each relevant pair it reports:
      - status: 'interference' (one solid's sampled points fall inside the other,
        i.e. they physically penetrate), 'contact' (touching, distance ~0 with no
        interior penetration), 'clearance_violation' (separated but closer than
        the requested clearance), or 'error' (the measurement failed)
      - distance: the measured minimum gap between the two solids
      - interior_sample_count and overlap_region (sampled-point bounds) for
        interferences when compute_volume is True

    The summary reports solid_count, pairs evaluated, interference_count,
    contact_count, and clearance_violation_count.

    Use after positioning multiple parts (patterns, struts, inserted library
    parts) to confirm nothing collides or sits too close before exporting or
    booleaning them together.

    Args:
        doc_name: Document to inspect.
        obj_names: Specific objects (names or snapshot uids) to test against each
                   other. If omitted, every visible solid in the document is used.
        clearance: Minimum allowed gap (mm) between non-touching solids. When > 0,
                   pairs closer than this are reported as clearance violations.
                   Default 0.0 reports only true interferences and contacts.
        compute_volume: When True, report the sampled interior point count and the
                        bounds of the penetrating region for each interfering pair.
                        Set False for a faster status-only scan.
    """
    try:
        res = get_parashell_connection().check_interferences(
            doc_name, obj_names, clearance, compute_volume
        )
        if not res.get("success"):
            return text_response(f"Failed to check interferences: {res.get('error')}")

        interference_count = int(res.get("interference_count", 0))
        contact_count = int(res.get("contact_count", 0))
        clearance_count = int(res.get("clearance_violation_count", 0))
        solid_count = int(res.get("solid_count", 0))
        missing = res.get("missing_objects", []) or []
        missing_part = f" Missing: {missing}." if missing else ""
        if interference_count == 0 and contact_count == 0 and clearance_count == 0:
            verdict = "No interferences, contacts, or clearance violations detected."
        else:
            verdict = (
                f"{interference_count} interference(s), {contact_count} contact(s), "
                f"{clearance_count} clearance violation(s)."
            )
        header = (
            f"Interference scan of '{res.get('document')}': {solid_count} solid(s) tested. "
            f"{verdict}{missing_part}"
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
        logger.error(f"Failed to check interferences: {e}")
        return text_response(f"Failed to check interferences: {e}")
