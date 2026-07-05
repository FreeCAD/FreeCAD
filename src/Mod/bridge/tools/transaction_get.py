from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def transaction_get(
    ctx: Context,
    transaction_id: str,
) -> list[TextContent]:
    """Get the full record of a single transaction, including its operation log.

    Args:
        transaction_id: Id of the transaction to inspect.
    """
    try:
        res = get_parashell_connection().transaction_get(transaction_id)
        if not res.get("success"):
            return text_response(f"transaction_get failed: {res.get('error')}")
        return json_response(res["transaction"])
    except Exception as e:
        logger.error(f"transaction_get failed: {e}")
        return text_response(f"transaction_get failed: {e}")
