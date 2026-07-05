from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def export_spreadsheet_csv(
    ctx: Context,
    doc_name: str,
    sheet_name: str,
    file_path: str,
    delimiter: str = "\t",
) -> list[TextContent]:
    """Export a spreadsheet's cells to a delimited text file.

    Args:
        doc_name: Document containing the spreadsheet.
        sheet_name: Spreadsheet object name.
        file_path: Absolute path of the file to write. Parent directories are created.
        delimiter: Single-character field delimiter. Default is a tab.
    """
    try:
        res = get_parashell_connection().export_spreadsheet_csv(
            doc_name, sheet_name, file_path, delimiter
        )
        if not res.get("success"):
            return text_response(f"export_spreadsheet_csv failed: {res.get('error')}")
        return text_response(
            f"Exported spreadsheet '{sheet_name}' to '{res.get('file')}'."
        )
    except Exception as e:
        logger.error(f"export_spreadsheet_csv failed: {e}")
        return text_response(f"export_spreadsheet_csv failed: {e}")
