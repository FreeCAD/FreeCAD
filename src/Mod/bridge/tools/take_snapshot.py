import json
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def take_snapshot(
    ctx: Context,
    doc_name: str,
    include_subelements: bool = False,
) -> list[TextContent]:
    """Return a structured snapshot of a document with stable uids per object.

    Mirrors the chrome devtools take_snapshot pattern: every object is reported
    with a deterministic 'uid' that round-trips across calls (the uid is a hash
    of doc + object name, so the same object always returns the same uid). Use
    those uids in any other viewport tool that accepts a target — get_view,
    get_ortho, move_viewport, get_object, get_shape_info, edit_object, etc. all
    accept either a uid or a plain object name.

    Response shape:
      - document, object_count, include_subelements
      - tree: nested object tree (children come from Parashell's Group property,
        so PartDesign bodies, App::Part containers, and Document groups are
        rendered as parents). Each node carries:
          uid, name, label, type_id, category, visible, display_mode,
          shape_type, sub_counts {faces, edges, vertices}
      - index: flat list of all uids and a few metadata fields, easy to scan
        for a single match. Subelements appear with kind = "Face" / "Edge" /
        "Vertex" and a 'ref' like "Pad001#Face3" for use in highlight params.

    When include_subelements is True, each shaped node also gets faces / edges /
    vertices arrays containing per-element uids plus a quick metric (face area
    + center, edge length, vertex position). This makes it possible to click /
    highlight individual faces by uid without having to enumerate them through
    execute_code.

    Args:
        doc_name: Document to snapshot.
        include_subelements: When True, list every face/edge/vertex of every
                             shaped object with its own uid. Larger payload —
                             keep False for whole-document overview.
    """
    try:
        res = get_parashell_connection().take_snapshot(doc_name, include_subelements)
        if not res.get("success"):
            return text_response(f"Failed to take snapshot: {res.get('error')}")

        index = res.get("index", []) or []
        tree = res.get("tree", []) or []
        header = (
            f"Snapshot of '{res.get('document')}': {res.get('object_count', 0)} object(s), "
            f"{len(index)} uid entries (subelements={'on' if include_subelements else 'off'})."
        )
        payload = {
            "document": res.get("document"),
            "object_count": res.get("object_count", 0),
            "include_subelements": bool(res.get("include_subelements")),
            "tree": tree,
            "index": index,
        }
        return [
            TextContent(type="text", text=header),
            TextContent(
                type="text",
                text=json.dumps(payload, ensure_ascii=False, indent=2, default=str),
            ),
        ]
    except Exception as e:
        logger.error(f"Failed to take snapshot: {e}")
        return text_response(f"Failed to take snapshot: {e}")
