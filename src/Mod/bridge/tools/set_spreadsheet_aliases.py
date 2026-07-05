from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def set_spreadsheet_aliases(
    ctx: Context,
    doc_name: str,
    sheet_name: str,
    aliases: dict[str, str],
    recompute: bool = True,
) -> list[TextContent]:
    """Assign or remove cell aliases so model expressions can reference them by name.

    Once cell B1 has the alias "width", expressions elsewhere can use
    "Spreadsheet.width" and object properties can bind to it. Pass an empty
    string as the alias to remove an existing alias from a cell.

    Args:
        doc_name: Document containing the spreadsheet.
        sheet_name: Spreadsheet object name.
        aliases: Mapping of cell address to alias, e.g. {"B1": "width", "B2": "height"}.
        recompute: Recompute the document afterward. Default True.
    """
    try:
        res = get_parashell_connection().set_spreadsheet_aliases(
            doc_name, sheet_name, aliases, recompute
        )
        if not res.get("success"):
            return text_response(f"set_spreadsheet_aliases failed: {res.get('error')}")
        return json_response({"aliases": res.get("aliases", {})})
    except Exception as e:
        logger.error(f"set_spreadsheet_aliases failed: {e}")
        return text_response(f"set_spreadsheet_aliases failed: {e}")
