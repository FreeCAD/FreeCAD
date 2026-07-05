from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def set_spreadsheet_cells(
    ctx: Context,
    doc_name: str,
    sheet_name: str,
    cells: dict[str, Any],
    recompute: bool = True,
) -> list[TextContent]:
    """Write content to one or more spreadsheet cells (create or update).

    Each value is stored as cell content: plain text stays text, a numeric
    string becomes a number, and a string starting with '=' becomes a formula
    (e.g. "=A1*B1" or "=Box.Length"). Setting a value to an empty string clears
    that cell's content.

    Args:
        doc_name: Document containing the spreadsheet.
        sheet_name: Spreadsheet object name.
        cells: Mapping of cell address to content, e.g.
            {"A1": "Width", "B1": "100", "B2": "=B1*2"}.
        recompute: Recompute the document after writing. Default True.
    """
    try:
        res = get_parashell_connection().set_spreadsheet_cells(
            doc_name, sheet_name, cells, recompute
        )
        if not res.get("success"):
            return text_response(f"set_spreadsheet_cells failed: {res.get('error')}")
        updated = res.get("updated", [])
        return text_response(
            f"Updated {len(updated)} cell(s) in '{sheet_name}': {', '.join(updated)}"
        )
    except Exception as e:
        logger.error(f"set_spreadsheet_cells failed: {e}")
        return text_response(f"set_spreadsheet_cells failed: {e}")
