from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, mcp, text_response


@mcp.tool()
def list_techdraw_pages(ctx: Context, doc_name: str | None = None) -> list[TextContent]:
    """List the TechDraw drawing pages in a Parashell document.

    Returns each TechDraw::DrawPage with its label, template size (Width/Height in
    mm and orientation), page scale, and the child views placed on it (name, type,
    scale, projection direction, and source object). Use this to discover which
    page or view to render with get_techdraw_page.

    Args:
        doc_name: Name of the document to inspect. Defaults to the active document.
    """
    result = get_parashell_connection().list_techdraw_pages(doc_name)
    if not result.get("success"):
        return text_response(result.get("error", "Could not list TechDraw pages."))
    pages = result.get("pages") or []
    if not pages:
        return text_response(
            f"Document '{result.get('document')}' has no TechDraw pages. "
            "Create one with execute_code (App.activeDocument().addObject"
            "('TechDraw::DrawPage', 'Page') plus a template and views)."
        )
    return json_response(result)
