from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import (
    _format_autosave_summary,
    get_parashell_connection,
    logger,
    mcp,
    text_response,
)


@mcp.tool()
def autosave(
    ctx: Context,
    doc_name: str | None = None,
    fallback_dir: str | None = None,
) -> list[TextContent]:
    """Save Parashell documents to disk.

    Saves the active document, a named document, or all open documents. Documents
    that already have a file path are saved in place. Documents without a stored
    path are written as <Name>.FCStd into fallback_dir, or into your home
    directory (e.g. C:\\Users\\<you> on Windows) when fallback_dir is omitted.
    Runs automatically before every execute_code call.

    Args:
        doc_name: Name of a specific document to save. If omitted, all open
            documents are saved.
        fallback_dir: Directory used to save documents that have no FileName
            yet. If omitted, the user's home directory is used.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.autosave(doc_name, fallback_dir)
        if res.get("success"):
            return text_response(_format_autosave_summary(res))
        return text_response(f"Autosave failed: {res.get('error', 'unknown error')}")
    except Exception as e:
        logger.error(f"Autosave failed: {str(e)}")
        return text_response(f"Autosave failed: {str(e)}")
