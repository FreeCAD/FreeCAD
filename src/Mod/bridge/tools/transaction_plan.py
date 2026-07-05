from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def transaction_plan(
    ctx: Context,
    transaction_id: str,
) -> list[TextContent]:
    """Preview the pending changes of a transaction without persisting them.

    Returns a Terraform-style plan of what would change on apply: objects to add,
    change, and destroy, plus the recorded operation log. Safe to call repeatedly.

    Args:
        transaction_id: Id of the transaction returned by transaction_create.
    """
    try:
        res = get_parashell_connection().transaction_plan(transaction_id)
        if not res.get("success"):
            return text_response(f"transaction_plan failed: {res.get('error')}")
        return json_response(
            {
                "transaction": res["transaction"],
                "plan": res["plan"],
                "summary": res["summary"],
            }
        )
    except Exception as e:
        logger.error(f"transaction_plan failed: {e}")
        return text_response(f"transaction_plan failed: {e}")
