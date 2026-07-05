from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Any, Literal
from bridge import (
    _is_valid_png_b64,
    get_parashell_connection,
    logger,
    make_image_content,
    mcp,
    text_response,
)


@mcp.tool()
def move_viewport(
    ctx: Context,
    action: Literal[
        "fit_all",
        "zoom_in",
        "zoom_out",
        "zoom_to_object",
        "navicube",
        "pan",
        "rotate",
        "set_camera",
    ],
    params: dict[str, Any] | None = None,
    get_screenshot: bool = True,
    width: int | None = None,
    height: int | None = None,
) -> list[ImageContent | TextContent]:
    """Interact with the Parashell 3D viewport — navigate, zoom, orient the camera, and optionally get a screenshot.

    Supported actions and their params:

    - "fit_all": Zoom to fit all objects. No params needed.

    - "zoom_in": Zoom in. Params: {"factor": 1.25}

    - "zoom_out": Zoom out. Params: {"factor": 1.25}

    - "zoom_to_object": Zoom to a specific object. Params: {"object_name": "MyBox"}

    - "navicube": Set a named camera orientation (like clicking the NaviCube).
      Params: {"preset": "Front"} — preset can be any of: Isometric, Front, Back,
      Top, Bottom, Right, Left, Dimetric, Trimetric.

    - "pan": Pan the camera. Params: {"dx": 50, "dy": 0} — dx/dy in screen units.

    - "rotate": Rotate camera around an axis by degrees.
      Params: {"axis": [0, 0, 1], "angle_deg": 45}

    - "set_camera": Set camera orientation directly.
      Params: {"orientation": {"axis_x": 0, "axis_y": 0, "axis_z": 1, "angle_deg": 45},
               "type": "Orthographic", "fit": true}

    Args:
        action: The viewport action to perform.
        params: Action-specific parameters dict.
        get_screenshot: Whether to return a screenshot after the action. Default True.
        width: Screenshot width in pixels.
        height: Screenshot height in pixels.
    """
    try:
        parashell = get_parashell_connection()
        result = parashell.move_viewport(
            action, params or {}, get_screenshot, width, height
        )

        if not result.get("success"):
            return text_response(
                f"Viewport action '{action}' failed: {result.get('error')}"
            )

        screenshot = result.get("screenshot")
        if screenshot and _is_valid_png_b64(screenshot):
            return [
                TextContent(
                    type="text",
                    text=f"Viewport action '{action}' applied successfully.",
                ),
                make_image_content(screenshot),
            ]
        return text_response(f"Viewport action '{action}' applied successfully.")
    except Exception as e:
        logger.error(f"move_viewport failed: {e}")
        return text_response(f"Failed to perform viewport action '{action}': {str(e)}")
