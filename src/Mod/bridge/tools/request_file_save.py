from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def request_file_save(
    ctx: Context,
    doc_name: str | None = None,
    suggested_name: str | None = None,
) -> list[TextContent]:
    """Ask the user where to save a model via a save-location prompt.

    Opens a dialog letting the user pick a folder and a file name (the .FCStd
    extension is fixed), then saves the document there. Use this when a model has
    no save location yet so applied changes persist where the user wants. Returns
    where the model was saved, or that the user cancelled.

    Args:
        doc_name: Document to save. If omitted, the active document is used.
        suggested_name: Optional default file name to pre-fill (without extension).
    """
    try:
        res = get_parashell_connection().request_file_save(doc_name, suggested_name)
        if not res.get("success"):
            return text_response(f"request_file_save failed: {res.get('error')}")
        if res.get("cancelled"):
            return text_response(
                f"Save cancelled by user; '{res['document']}' is not yet saved to a "
                "chosen location."
            )
        return json_response(
            {
                "document": res["document"],
                "saved": res["saved"],
                "path": res["path"],
                "directory": res["directory"],
                "filename": res["filename"],
            }
        )
    except Exception as e:
        logger.error(f"request_file_save failed: {e}")
        return text_response(f"request_file_save failed: {e}")
