from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def import_spreadsheet_csv(
    ctx: Context,
    doc_name: str,
    sheet_name: str,
    file_path: str,
    delimiter: str = "\t",
    recompute: bool = True,
) -> list[TextContent]:
    """Import a delimited text file into a spreadsheet.

    If the named spreadsheet does not exist it is created. Existing cell data is
    overwritten by the imported file.

    Args:
        doc_name: Document to import into.
        sheet_name: Spreadsheet object name. Created if absent.
        file_path: Absolute path to the file to import.
        delimiter: Single-character field delimiter. Default is a tab.
        recompute: Recompute the document afterward. Default True.
    """
    try:
        res = get_parashell_connection().import_spreadsheet_csv(
            doc_name, sheet_name, file_path, delimiter, recompute
        )
        if not res.get("success"):
            return text_response(f"import_spreadsheet_csv failed: {res.get('error')}")
        return text_response(
            f"Imported '{res.get('file')}' into spreadsheet '{res.get('sheet_name')}'."
        )
    except Exception as e:
        logger.error(f"import_spreadsheet_csv failed: {e}")
        return text_response(f"import_spreadsheet_csv failed: {e}")
