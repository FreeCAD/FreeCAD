from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def create_spreadsheet(
    ctx: Context,
    doc_name: str,
    name: str = "Spreadsheet",
    label: str | None = None,
) -> list[TextContent]:
    """Create a new Spreadsheet::Sheet object in a Parashell document.

    Spreadsheets hold tabular data and parametric values. Cells can carry
    literal text, numbers, or formulas (prefix with '='), and any cell can be
    given an alias so model expressions reference it by a readable name.

    Args:
        doc_name: Document to create the spreadsheet in.
        name: Internal object name. Parashell may adjust it to keep it unique.
        label: Optional human-readable label shown in the tree.
    """
    try:
        res = get_parashell_connection().create_spreadsheet(doc_name, name, label)
        if res.get("success"):
            return text_response(
                f"Spreadsheet '{res['sheet_name']}' (label '{res['label']}') "
                f"created in '{doc_name}'."
            )
        return text_response(f"Failed to create spreadsheet: {res.get('error')}")
    except Exception as e:
        logger.error(f"create_spreadsheet failed: {e}")
        return text_response(f"create_spreadsheet failed: {e}")
