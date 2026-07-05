from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def is_file_saved(
    ctx: Context,
    doc_name: str | None = None,
) -> list[TextContent]:
    """Report whether a document is saved to a file on disk.

    Args:
        doc_name: Document to check. If omitted, the active document is used.
    """
    try:
        res = get_parashell_connection().is_file_saved(doc_name)
        if not res.get("success"):
            return text_response(f"is_file_saved failed: {res.get('error')}")
        return json_response(
            {
                "document": res["document"],
                "saved": res["saved"],
                "path": res["path"],
            }
        )
    except Exception as e:
        logger.error(f"is_file_saved failed: {e}")
        return text_response(f"is_file_saved failed: {e}")
