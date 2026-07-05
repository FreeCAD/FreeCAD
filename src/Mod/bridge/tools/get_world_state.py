import json
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def get_world_state(
    ctx: Context,
    doc_name: str | None = None,
    include_feature_tree: bool = True,
    include_geometry: bool = True,
    max_objects: int = 400,
) -> list[TextContent]:
    """Capture a complete structured digest of the current Parashell scene as text.

    This is the authoritative, image-free description of the model. Use it to refresh
    your understanding of spatial state without spending a screenshot, and to record
    durable facts (positions, sizes, volumes, feature order, health) that survive once
    older screenshots scroll out of the conversation. Call it after a batch of edits,
    whenever you are unsure what currently exists, or before reasoning about geometry
    you last saw several turns ago.

    For each open document (or only doc_name) the digest reports:
      - document metadata: name, label, file_name, is_active, modified, object_count
      - category_counts: how many Bodies, Sketches, PartDesign features, Part
        features, Datums, and Other objects exist
      - model_bbox: the axis-aligned bounding box enclosing every object's geometry,
        with min/max corners and x/y/z side lengths — the overall extent of the model
      - unhealthy_count plus, per object: name, label, type_id, category, visibility,
        placement (position, rotation axis, rotation angle in degrees), and the
        'issues' tokens from the health scan (state:Invalid, must_execute, shape:null,
        shape:invalid, shape:empty, shape:zero_volume)
      - geometry per object (when include_geometry): volume, area, solid/face/edge
        counts, and per-object bounding box
      - bodies (when include_feature_tree): each PartDesign::Body's tip and ordered
        feature_chain with sketch consumption links

    Prefer this over repeatedly capturing screenshots when you only need to confirm
    what exists, where it is, and whether it is valid. Reserve get_view / get_ortho for
    when you genuinely need to see rendered geometry.

    Args:
        doc_name: Limit the digest to a single document. Omit to digest every open document.
        include_feature_tree: Include PartDesign body feature chains. Default True.
        include_geometry: Include per-object volume/area/counts/bbox and the model bbox. Default True.
        max_objects: Cap the number of objects detailed per document (newest beyond the
                     cap are summarized in counts only). Default 400.
    """
    try:
        res = get_parashell_connection().get_world_state(
            doc_name, include_feature_tree, include_geometry, max_objects
        )
        if not res.get("success"):
            return text_response(f"Failed to get world state: {res.get('error')}")

        documents = res.get("documents", []) or []
        if not documents:
            return text_response(
                "No open documents. Create or open a document before inspecting world state."
            )

        active = res.get("active_document")
        summary_lines = [
            f"World state: {res.get('document_count', len(documents))} open document(s)."
            + (f" Active: '{active}'." if active else " No active document.")
        ]
        for d in documents:
            bbox = d.get("model_bbox")
            if isinstance(bbox, dict):
                extent = (
                    f"{bbox['x_length']:.2f} x {bbox['y_length']:.2f} x "
                    f"{bbox['z_length']:.2f}"
                )
            else:
                extent = "no geometry"
            summary_lines.append(
                f"  - '{d.get('name')}': {d.get('object_count', 0)} object(s), "
                f"{d.get('unhealthy_count', 0)} unhealthy, extent {extent}"
                + (" [object list truncated]" if d.get("objects_truncated") else "")
            )

        payload = {
            "active_document": active,
            "document_count": res.get("document_count", len(documents)),
            "documents": documents,
        }
        return [
            TextContent(type="text", text="\n".join(summary_lines)),
            TextContent(
                type="text",
                text=json.dumps(payload, ensure_ascii=False, indent=2, default=str),
            ),
        ]
    except Exception as e:
        logger.error(f"Failed to get world state: {e}")
        return text_response(f"Failed to get world state: {e}")
