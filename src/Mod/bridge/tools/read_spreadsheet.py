from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def read_spreadsheet(
    ctx: Context,
    doc_name: str,
    sheet_name: str,
    cell_range: list[str] | str | None = None,
) -> list[TextContent]:
    """Read cells from a spreadsheet, returning content, computed value, alias, and styling.

    For each cell this returns the stored content (the literal or formula text),
    the evaluated value, and any alias, display unit, style, alignment, and
    foreground/background colors that are set.

    Args:
        doc_name: Document containing the spreadsheet.
        sheet_name: Spreadsheet object name.
        cell_range: A single cell ("B3"), a range ("A1:C10"), or a list of cells
            and ranges. If omitted, every used cell in the sheet is returned.
    """
    try:
        res = get_parashell_connection().get_spreadsheet(
            doc_name, sheet_name, cell_range
        )
        if not res.get("success"):
            return text_response(f"read_spreadsheet failed: {res.get('error')}")
        return json_response(
            {
                "sheet_name": res.get("sheet_name"),
                "label": res.get("label"),
                "cell_count": res.get("cell_count"),
                "cells": res.get("cells", []),
            }
        )
    except Exception as e:
        logger.error(f"read_spreadsheet failed: {e}")
        return text_response(f"read_spreadsheet failed: {e}")
