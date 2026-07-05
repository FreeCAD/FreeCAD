import json
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def get_feature_tree(
    ctx: Context,
    doc_name: str,
    body_name: str | None = None,
    include_orphans: bool = True,
) -> list[TextContent]:
    """Return PartDesign body feature dependency graphs for a document.

    For each PartDesign::Body in the document (or only the named one), returns:
      - name, label, type_id, tip (the body's current tip feature)
      - feature_chain: ordered list of PartDesign features (Pad, Pocket, Boolean,
        Fillet, Chamfer, Mirrored, ...). Each entry has:
          name, label, type_id, category, state, must_execute, is_tip,
          base_features (the BaseFeature predecessor names — the linear chain),
          links (every link property and target plus subelement when present, e.g.
            Profile -> Sketch001, ReferenceAxis -> Origin#X_Axis, UpToFace -> Pad.Face3),
          shape_summary (type, is_null, is_valid, volume).
      - sketches_and_datums: every sketch / datum / shape binder that lives inside
        the body's Group. Sketches include 'consumed_by' listing every PartDesign
        feature whose Profile/Sketch references them, so you can see which Pocket
        owns which sketch without piecing it together yourself.
      - feature_count, sketch_count, datum_count

    Outside the bodies, when include_orphans is True the response also lists any
    PartDesign / Part / Sketcher objects that are not inside a body, with the same
    node format. This catches Part::Box/Cut/Fuse trees and dangling sketches.

    Args:
        doc_name: Document to inspect.
        body_name: Optional PartDesign::Body name. If omitted, every body in the
                   document is returned.
        include_orphans: When True, also report PartDesign/Part/Sketcher objects
                         that are not inside a body. Default True.
    """
    try:
        res = get_parashell_connection().get_feature_tree(
            doc_name, body_name, include_orphans
        )
        if not res.get("success"):
            return text_response(f"Failed to get feature tree: {res.get('error')}")

        bodies = res.get("bodies", []) or []
        orphans = res.get("orphan_features", []) or []
        body_summary = (
            ", ".join(
                f"{b.get('name')}: {b.get('feature_count', 0)} features, "
                f"{b.get('sketch_count', 0)} sketches"
                for b in bodies
            )
            or "no PartDesign bodies"
        )

        header = (
            f"Feature tree for '{res.get('document')}': {len(bodies)} body(ies) "
            f"[{body_summary}], {len(orphans)} orphan feature(s)."
        )
        payload = {
            "document": res.get("document"),
            "body_count": len(bodies),
            "bodies": bodies,
            "orphan_features": orphans,
        }
        return [
            TextContent(type="text", text=header),
            TextContent(
                type="text",
                text=json.dumps(payload, ensure_ascii=False, indent=2, default=str),
            ),
        ]
    except Exception as e:
        logger.error(f"Failed to get feature tree: {e}")
        return text_response(f"Failed to get feature tree: {e}")
