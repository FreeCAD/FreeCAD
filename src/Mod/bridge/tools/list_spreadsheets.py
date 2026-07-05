from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def list_spreadsheets(ctx: Context, doc_name: str) -> list[TextContent]:
    """List every Spreadsheet::Sheet in a document with its used-cell count.

    Args:
        doc_name: Document to inspect.
    """
    try:
        res = get_parashell_connection().list_spreadsheets(doc_name)
        if not res.get("success"):
            return text_response(f"list_spreadsheets failed: {res.get('error')}")
        return json_response(res.get("spreadsheets", []))
    except Exception as e:
        logger.error(f"list_spreadsheets failed: {e}")
        return text_response(f"list_spreadsheets failed: {e}")
