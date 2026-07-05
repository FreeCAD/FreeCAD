from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Literal
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def modify_spreadsheet_structure(
    ctx: Context,
    doc_name: str,
    sheet_name: str,
    operation: Literal[
        "insert_rows", "remove_rows", "insert_columns", "remove_columns"
    ],
    index: str,
    count: int = 1,
    recompute: bool = True,
) -> list[TextContent]:
    """Insert or remove rows and columns, shifting existing cells accordingly.

    Args:
        doc_name: Document containing the spreadsheet.
        sheet_name: Spreadsheet object name.
        operation: One of insert_rows, remove_rows, insert_columns, remove_columns.
        index: For row operations, the row number as a string (e.g. "3"). For
            column operations, the column letter (e.g. "B"). Inserts happen
            before this index.
        count: How many rows or columns to insert or remove. Default 1.
        recompute: Recompute the document afterward. Default True.
    """
    try:
        res = get_parashell_connection().modify_spreadsheet_structure(
            doc_name, sheet_name, operation, index, count, recompute
        )
        if not res.get("success"):
            return text_response(
                f"modify_spreadsheet_structure failed: {res.get('error')}"
            )
        return text_response(
            f"{operation} ({res.get('count')}) at '{res.get('index')}' "
            f"applied to '{sheet_name}'."
        )
    except Exception as e:
        logger.error(f"modify_spreadsheet_structure failed: {e}")
        return text_response(f"modify_spreadsheet_structure failed: {e}")
