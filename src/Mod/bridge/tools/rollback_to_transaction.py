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
def rollback_to_transaction(
    ctx: Context,
    transaction_id: str,
) -> list[TextContent | ImageContent]:
    """Hard-reset a document to the exact state of an applied transaction.

    Like 'git reset --hard <commit>': restores the document to the checkpoint
    captured when the given transaction was applied and discards every change
    made after it, no matter how many transactions sit on top. Transactions
    newer than the target are marked cancelled. Only applied transactions have a
    checkpoint to roll back to. This destroys later work — confirm with the user
    before using it when applied changes would be lost.

    Args:
        transaction_id: Id of the applied transaction to roll back to.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.rollback_to_transaction(transaction_id)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(
                f"rollback_to_transaction failed: {res.get('error')}"
            )
        else:
            response = json_response(
                {
                    "restored_to": res["restored_to"],
                    "document": res["document"],
                    "superseded": res["superseded"],
                    "autosave": res["autosave"],
                }
            )
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"rollback_to_transaction failed: {e}")
        return text_response(f"rollback_to_transaction failed: {e}")
