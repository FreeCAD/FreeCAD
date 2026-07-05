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
def transaction_apply(
    ctx: Context,
    transaction_id: str,
) -> list[TextContent | ImageContent]:
    """Apply an open transaction: commit its changes, recompute, and save.

    Finalizes every edit made under the transaction as a single atomic change,
    recomputes the document, and autosaves it (to its existing path, or to your
    home directory as <Name>.FCStd if it has none). The transaction becomes
    'applied'.

    Args:
        transaction_id: Id of the open transaction to apply.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.transaction_apply(transaction_id)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"transaction_apply failed: {res.get('error')}")
        else:
            response = json_response(
                {
                    "transaction": res["transaction"],
                    "applied": res["applied"],
                    "summary": res["summary"],
                    "autosave": res["autosave"],
                }
            )
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"transaction_apply failed: {e}")
        return text_response(f"transaction_apply failed: {e}")
