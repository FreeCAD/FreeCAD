from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Literal
from bridge import (
    _composite_ortho_views,
    _is_valid_png_b64,
    get_parashell_connection,
    logger,
    make_image_content,
    mcp,
    text_response,
)


@mcp.tool()
def get_ortho(
    ctx: Context,
    views: list[str] | None = None,
    tile_width: int = 480,
    tile_height: int = 360,
    focus_object: str | None = None,
    include_isometric: bool = True,
    display_mode: (
        Literal["Shaded", "Wireframe", "Flat Lines", "Hidden line", "Points", "As is"]
        | None
    ) = None,
    hide: list[str] | None = None,
    show_only: list[str] | None = None,
    highlight: list[str] | None = None,
    highlight_color: list[float] | None = None,
    camera_mode: Literal["Perspective", "Orthographic"] | None = None,
) -> list[ImageContent | TextContent]:
    """Get all orthographic views composited into a single labeled grid image.

    Captures the requested standard views and stitches them into a grid with
    dark-themed labeled tiles. Ortho views are arranged in a 3-column grid
    (Front/Right/Top, Back/Left/Bottom) with the Isometric view spanning the
    full width below. Ideal for a spatial overview of a model in a single call.

    Visual overrides (display_mode, hide, show_only, highlight) follow the same
    rules as get_view — applied per tile, restored after each capture, and
    accept either object names or take_snapshot uids.

    Each tile is fully settled before it is captured, so no tile shows a
    mid-transition camera. Camera animation is disabled during the capture and
    re-enabled afterward.

    Args:
        views: List of view names to include. Defaults to all six orthographic views
               plus Isometric: ["Front", "Right", "Top", "Back", "Left", "Bottom", "Isometric"].
               Any subset of these can be requested.
        tile_width: Width in pixels of each individual view tile. Default 480.
        tile_height: Height in pixels of each individual view tile. Default 360.
        focus_object: Name or uid of an object to focus each view on. Fits all if omitted.
        include_isometric: Whether to append the Isometric view spanning full width
                           at the bottom of the composite. Default True.
        display_mode: Force every visible ViewObject into a specific display mode for
                      every tile (Wireframe / Hidden line is great for verifying
                      internal cuts).
        hide: Objects to hide while capturing.
        show_only: Objects to isolate; everything else is hidden.
        highlight: Objects or subelement refs to outline + select for the capture.
        highlight_color: Optional RGB triple in [0,1].
        camera_mode: Force the camera projection to "Perspective" or "Orthographic"
                     for every tile. The viewport reverts to the user's original
                     projection once the capture finishes. Defaults to the current
                     projection if omitted.
    """
    if views is None:
        views = ["Front", "Right", "Top", "Back", "Left", "Bottom"]
        if include_isometric:
            views.append("Isometric")
    elif include_isometric and "Isometric" not in views:
        views = list(views) + ["Isometric"]

    try:
        parashell = get_parashell_connection()
        color_arg = tuple(highlight_color) if highlight_color else None
        views_data = parashell.get_ortho_views(
            views,
            tile_width,
            tile_height,
            focus_object,
            display_mode,
            hide,
            show_only,
            highlight,
            color_arg,
            camera_mode,
        )

        if not views_data:
            return text_response(
                "No views could be captured. Ensure a 3D view is active in Parashell."
            )

        overrides: list[str] = []
        if display_mode:
            overrides.append(f"display={display_mode}")
        if show_only:
            overrides.append(f"show_only={show_only}")
        if hide:
            overrides.append(f"hide={hide}")
        if highlight:
            overrides.append(f"highlight={highlight}")
        override_part = f" Overrides: {', '.join(overrides)}." if overrides else ""

        try:
            composite_b64 = _composite_ortho_views(views_data, tile_width, tile_height)
            captured = list(views_data.keys())
            size_kb = len(composite_b64) * 3 / 4 / 1024
            focus_part = (
                f", focused on '{focus_object}'"
                if focus_object
                else ", fit-all per view"
            )
            if not _is_valid_png_b64(composite_b64):
                return text_response(
                    "Composited ortho image was empty. The active view may not "
                    "support offscreen rendering or every requested tile failed."
                )
            return [
                TextContent(
                    type="text",
                    text=(
                        f"Composited {len(captured)} views into one image: {captured}. "
                        f"Tile size {tile_width}x{tile_height}{focus_part}. PNG ~{size_kb:.1f} KB.{override_part}"
                    ),
                ),
                make_image_content(composite_b64),
            ]
        except ImportError:
            valid_tiles = [
                (name, b64)
                for name, b64 in views_data.items()
                if _is_valid_png_b64(b64)
            ]
            if not valid_tiles:
                return text_response(
                    "Captured 0 valid ortho tiles: every requested view returned "
                    "an empty image."
                )
            return [
                TextContent(
                    type="text",
                    text=(
                        f"Captured {len(valid_tiles)} views: {[n for n, _ in valid_tiles]}. "
                        f"Pillow not available for compositing — returning individual tiles.{override_part}"
                    ),
                ),
                *[make_image_content(b64) for _, b64 in valid_tiles],
            ]
    except Exception as e:
        logger.error(f"get_ortho failed: {e}")
        return text_response(f"Failed to get orthographic views: {str(e)}")
