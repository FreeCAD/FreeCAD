from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Literal
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def style_spreadsheet_cells(
    ctx: Context,
    doc_name: str,
    sheet_name: str,
    targets: list[str] | str,
    style: list[str] | None = None,
    foreground: list[float] | None = None,
    background: list[float] | None = None,
    alignment: list[str] | None = None,
    display_unit: str | None = None,
    style_options: Literal["replace", "add", "remove"] = "replace",
    recompute: bool = True,
) -> list[TextContent]:
    """Apply text styling, colors, alignment, and display units to cells.

    Args:
        doc_name: Document containing the spreadsheet.
        sheet_name: Spreadsheet object name.
        targets: A cell, a range ("A1:C1"), or a list of cells/ranges to style.
        style: Font styles to apply, any of "bold", "italic", "underline".
        foreground: Text color as [r, g, b] or [r, g, b, a] floats in 0..1.
        background: Cell fill color as [r, g, b] or [r, g, b, a] floats in 0..1.
        alignment: Alignment keywords, e.g. ["left"], ["center"], ["right"],
            ["top"], ["vcenter"], ["bottom"].
        display_unit: Display unit string applied to the cells, e.g. "mm".
        style_options: How to apply style/alignment — "replace" (default),
            "add", or "remove".
        recompute: Recompute the document afterward. Default True.
    """
    try:
        res = get_parashell_connection().style_spreadsheet_cells(
            doc_name,
            sheet_name,
            targets,
            style,
            foreground,
            background,
            alignment,
            display_unit,
            style_options,
            recompute,
        )
        if not res.get("success"):
            return text_response(f"style_spreadsheet_cells failed: {res.get('error')}")
        styled = res.get("styled", [])
        return text_response(f"Styled {len(styled)} cell(s) in '{sheet_name}'.")
    except Exception as e:
        logger.error(f"style_spreadsheet_cells failed: {e}")
        return text_response(f"style_spreadsheet_cells failed: {e}")
