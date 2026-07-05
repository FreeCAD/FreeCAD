import json
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def validate_document(
    ctx: Context,
    doc_name: str,
    deep_geometry: bool = False,
    run_bop_check: bool = False,
) -> list[TextContent]:
    """Run a whole-document introspective validation and return a single verdict.

    Aggregates every other diagnostic into one pass so the model can ask "is the
    document I just built internally consistent?". It reports:
      - valid: overall boolean — true only when no problems are found
      - problem_summary: human-readable list of every problem category hit
      - unhealthy_objects: per-object health (state flags, must_execute, shape
        diagnostics) for objects that fail the health scan
      - dangling_links: link properties that point at objects which no longer
        exist (object, property, target)
      - dependency_cycles: cycles in the recompute dependency graph (each cycle
        as an ordered list of object names) — these break recompute
      - touched_objects: objects still flagged Touched (a recompute is pending)
      - geometry_problems: when deep_geometry is True, per-object OCC geometry
        diagnostics for any object whose BRep is invalid

    Use as a final gate after a build sequence, or whenever a recompute behaves
    unexpectedly, to locate the root cause without running each checker by hand.

    Args:
        doc_name: Document to validate.
        deep_geometry: When True, also run the OCC geometry validity scan
                       (check_geometry) on every shaped object and include any
                       failures. Slower; off by default.
        run_bop_check: Only relevant when deep_geometry is True. Runs the
                       Boolean Operation self-intersection check on each shape.
    """
    try:
        res = get_parashell_connection().validate_document(
            doc_name, deep_geometry, run_bop_check
        )
        if not res.get("success"):
            return text_response(f"Failed to validate document: {res.get('error')}")

        problems = res.get("problem_summary", []) or []
        if res.get("valid"):
            verdict = "Document is valid: no problems detected."
        else:
            verdict = "Problems found: " + "; ".join(problems) + "."
        header = (
            f"Validation of '{res.get('document')}' ({res.get('object_count', 0)} object(s)). "
            f"{verdict}"
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
        logger.error(f"Failed to validate document: {e}")
        return text_response(f"Failed to validate document: {e}")
