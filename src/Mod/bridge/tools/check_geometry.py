import json
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def check_geometry(
    ctx: Context,
    doc_name: str,
    obj_names: list[str] | None = None,
    run_bop_check: bool = True,
    min_edge_length: float = 1e-7,
    min_face_area: float = 1e-9,
    only_invalid: bool = False,
) -> list[TextContent]:
    """Run a deep OCC geometry validity scan on the shapes you just built.

    Where check_objects reports the parametric/document health (state flags,
    must_execute, null/empty shape), this tool drills into the actual BRep solid
    to answer "is this part geometrically and topologically sound?". For each
    object with a Shape it reports:
      - is_null, is_valid, is_closed (top-level OCC flags)
      - counts: solids, shells, faces, wires, edges, vertices
      - tolerance: average / max / min modelling tolerance of the shape
      - invalid_subshapes: every Face/Edge/Wire/Solid that fails isValid(),
        named like 'Face3' / 'Edge12' so you can target a repair
      - short_edges: edges shorter than min_edge_length (sliver edges that break
        downstream fillets/booleans), each with its length
      - degenerate_faces: faces with area below min_face_area
      - open_shells: shells that are not closed (a solid that should be watertight
        but is not, e.g. after a failed boolean)
      - bop_check: result of Shape.check() — the Boolean Operation checker that
        finds self-intersections and bad geometry; 'ok', 'invalid', or the OCC
        error text
      - issues: short tokens (shape:null, shape:invalid, subshapes:invalid,
        edges:short, faces:degenerate, shells:open, bop:problems)
      - valid: convenience boolean, true when issues is empty

    Use immediately after create_object, boolean_op, make_* helpers, or
    make_sketch_extrude to confirm the produced solid is manufacturable before
    building further features on top of it.

    Args:
        doc_name: Document to inspect.
        obj_names: Specific objects (names or snapshot uids). If omitted, scans
                   every object that has a Shape.
        run_bop_check: Run the heavier Boolean Operation self-intersection check
                       (Shape.check(True)). Default True. Set False for a faster
                       structural-only pass on large assemblies.
        min_edge_length: Edges shorter than this (mm) are flagged as slivers.
        min_face_area: Faces with area below this (mm^2) are flagged as degenerate.
        only_invalid: When True, only objects with at least one issue are returned.
    """
    try:
        res = get_parashell_connection().check_geometry(
            doc_name,
            obj_names,
            run_bop_check,
            min_edge_length,
            min_face_area,
            only_invalid,
        )
        if not res.get("success"):
            return text_response(f"Failed to check geometry: {res.get('error')}")

        invalid_count = int(res.get("invalid_count", 0))
        checked_count = int(res.get("checked_count", 0))
        missing = res.get("missing_objects", []) or []
        no_shape = res.get("objects_without_shape", []) or []
        missing_part = f" Missing: {missing}." if missing else ""
        no_shape_part = f" No shape: {no_shape}." if no_shape else ""
        verdict = (
            "All checked shapes are geometrically valid."
            if invalid_count == 0
            else f"{invalid_count} object(s) have geometry problems."
        )
        header = (
            f"Geometry scan of '{res.get('document')}': checked {checked_count} "
            f"shape(s). {verdict}{missing_part}{no_shape_part}"
        )
        payload = {
            "document": res.get("document"),
            "checked_count": checked_count,
            "invalid_count": invalid_count,
            "missing_objects": missing,
            "objects_without_shape": no_shape,
            "objects": res.get("objects", []),
        }
        return [
            TextContent(type="text", text=header),
            TextContent(
                type="text",
                text=json.dumps(payload, ensure_ascii=False, indent=2, default=str),
            ),
        ]
    except Exception as e:
        logger.error(f"Failed to check geometry: {e}")
        return text_response(f"Failed to check geometry: {e}")
