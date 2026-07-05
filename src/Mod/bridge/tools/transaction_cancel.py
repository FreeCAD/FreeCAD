from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from bridge import (
    add_screenshot_if_available,
    get_parashell_connection,
    json_response,
    logger,
    mcp,
    state,
    text_response,
)


@mcp.tool()
def transaction_cancel(
    ctx: Context,
    transaction_id: str,
) -> list[TextContent | ImageContent]:
    """Cancel a transaction and roll back its changes.

    For an open transaction this aborts and undoes every change made under it.
    For an already-applied transaction it undoes the committed change (only
    possible if no newer transaction sits on top of it). The transaction becomes
    'cancelled' and can then be removed with transaction_delete.

    Args:
        transaction_id: Id of the transaction to cancel.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.transaction_cancel(transaction_id)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"transaction_cancel failed: {res.get('error')}")
        else:
            response = json_response(res["transaction"])
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"transaction_cancel failed: {e}")
        return text_response(f"transaction_cancel failed: {e}")
