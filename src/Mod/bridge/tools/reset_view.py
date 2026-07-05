from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from bridge import (
    _is_valid_png_b64,
    get_parashell_connection,
    logger,
    make_image_content,
    mcp,
    text_response,
)


@mcp.tool()
def reset_view(
    ctx: Context,
    get_screenshot: bool = True,
    width: int | None = None,
    height: int | None = None,
) -> list[ImageContent | TextContent]:
    """Reset the Parashell viewport to a standard isometric view with all objects fitted in frame.

    Equivalent to pressing Home or clicking the Isometric preset on the NaviCube and
    then fitting all objects. Useful after panning, zooming, or orbiting the model to
    get back to a clean reference view.

    Args:
        get_screenshot: Whether to return a screenshot of the reset view. Default True.
        width: Screenshot width in pixels.
        height: Screenshot height in pixels.
    """
    try:
        parashell = get_parashell_connection()
        result = parashell.reset_view(get_screenshot, width, height)

        if not result.get("success"):
            return text_response(f"reset_view failed: {result.get('error')}")

        screenshot = result.get("screenshot")
        if screenshot and _is_valid_png_b64(screenshot):
            return [
                TextContent(type="text", text="View reset to isometric with fit-all."),
                make_image_content(screenshot),
            ]
        return text_response("View reset to isometric with fit-all.")
    except Exception as e:
        logger.error(f"reset_view failed: {e}")
        return text_response(f"Failed to reset view: {str(e)}")
