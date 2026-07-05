from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def set_spreadsheet_dimensions(
    ctx: Context,
    doc_name: str,
    sheet_name: str,
    column_widths: dict[str, int] | None = None,
    row_heights: dict[str, int] | None = None,
    recompute: bool = True,
) -> list[TextContent]:
    """Set column widths and row heights in pixels.

    Args:
        doc_name: Document containing the spreadsheet.
        sheet_name: Spreadsheet object name.
        column_widths: Mapping of column letter to width in pixels, e.g. {"A": 120}.
        row_heights: Mapping of row number to height in pixels, e.g. {"1": 30}.
        recompute: Recompute the document afterward. Default True.
    """
    try:
        res = get_parashell_connection().set_spreadsheet_dimensions(
            doc_name, sheet_name, column_widths, row_heights, recompute
        )
        if not res.get("success"):
            return text_response(
                f"set_spreadsheet_dimensions failed: {res.get('error')}"
            )
        return json_response(
            {"columns": res.get("columns", {}), "rows": res.get("rows", {})}
        )
    except Exception as e:
        logger.error(f"set_spreadsheet_dimensions failed: {e}")
        return text_response(f"set_spreadsheet_dimensions failed: {e}")
