from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def transaction_delete(
    ctx: Context,
    transaction_id: str,
) -> list[TextContent]:
    """Permanently remove a transaction record.

    Only cancelled transactions can be deleted; cancel an open or applied
    transaction first. This purges the bookkeeping record only — the document is
    not modified.

    Args:
        transaction_id: Id of the cancelled transaction to delete.
    """
    try:
        res = get_parashell_connection().transaction_delete(transaction_id)
        if not res.get("success"):
            return text_response(f"transaction_delete failed: {res.get('error')}")
        return text_response(
            f"Transaction '{res['deleted']}' deleted from '{res['document']}'."
        )
    except Exception as e:
        logger.error(f"transaction_delete failed: {e}")
        return text_response(f"transaction_delete failed: {e}")
