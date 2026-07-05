from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def transaction_create(
    ctx: Context,
    doc_name: str,
    label: str | None = None,
    reason: str | None = None,
) -> list[TextContent]:
    """Open a new transaction against a document and get its id.

    Transactions work like Terraform: open one here, route every edit
    (create_object, edit_object, delete_object, insert_part_from_library,
    execute_code, and any other model change) through its id while it is open,
    review the pending changes with transaction_plan, then transaction_apply to
    persist and save them or transaction_cancel to roll everything back. Only one
    transaction can be open per document at a time.

    Args:
        doc_name: Document to open the transaction on.
        label: Optional human label for the transaction.
        reason: Optional explanation of what this transaction is for.
    """
    try:
        res = get_parashell_connection().transaction_create(doc_name, label, reason)
        if not res.get("success"):
            return text_response(f"transaction_create failed: {res.get('error')}")
        return json_response(res["transaction"])
    except Exception as e:
        logger.error(f"transaction_create failed: {e}")
        return text_response(f"transaction_create failed: {e}")
