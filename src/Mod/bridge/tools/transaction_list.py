from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def transaction_list(
    ctx: Context,
    doc_name: str | None = None,
) -> list[TextContent]:
    """List tracked transactions and their status.

    Args:
        doc_name: Optional document to filter by. If omitted, lists all
            transactions across every document.
    """
    try:
        res = get_parashell_connection().transaction_list(doc_name)
        if not res.get("success"):
            return text_response(f"transaction_list failed: {res.get('error')}")
        return json_response(res["transactions"])
    except Exception as e:
        logger.error(f"transaction_list failed: {e}")
        return text_response(f"transaction_list failed: {e}")
