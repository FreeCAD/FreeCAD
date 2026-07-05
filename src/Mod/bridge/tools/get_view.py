from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Literal
from bridge import (
    _is_valid_png_b64,
    get_parashell_connection,
    make_image_content,
    mcp,
    text_response,
)


@mcp.tool()
def get_view(
    ctx: Context,
    view_name: Literal[
        "Isometric",
        "Front",
        "Top",
        "Right",
        "Back",
        "Left",
        "Bottom",
        "Dimetric",
        "Trimetric",
    ],
    width: int | None = None,
    height: int | None = None,
    focus_object: str | None = None,
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
    """Get a screenshot of the Parashell viewport from a named standard view.

    Visual overrides are applied for the duration of the screenshot, then every
    affected ViewObject is restored to its original Visibility / DisplayMode /
    LineColor / LineWidth. Targets accept either a stable uid from take_snapshot
    (e.g. "obj_a1b2c3...") or a plain object name. Subelements use either a
    sub_ uid or "ObjectName#Face3" notation.

    The viewport is fully settled before the image is captured, so the screenshot
    never shows a mid-transition camera. Any animation is disabled during the
    capture and re-enabled afterward.

    Args:
        view_name: Standard view name — Isometric, Front, Top, Right, Back, Left, Bottom, Dimetric, or Trimetric.
        width: Screenshot width in pixels. Defaults to viewport width.
        height: Screenshot height in pixels. Defaults to viewport height.
        focus_object: Object uid or name to focus on. Fits all objects if omitted.
        display_mode: Force every visible ViewObject into a specific display mode for
                      the screenshot. Useful for verifying internal cuts (Wireframe or
                      Hidden line) or thin features (Wireframe).
        hide: List of object uids or names to hide for the screenshot.
        show_only: List of object uids or names — every other object is hidden,
                   isolating the listed targets. Useful for verifying a specific
                   pad / pocket / boolean.
        highlight: List of object uids, "ObjectName#Face3" refs, or sub_ uids to
                   draw with a thick outline color and select for the screenshot.
                   Useful for tracing feature outlines on top of the model.
        highlight_color: Optional RGB triple in [0,1] for the highlight outline.
                         Defaults to bright orange (1.0, 0.4, 0.0).
        camera_mode: Force the camera projection to "Perspective" or "Orthographic"
                     for this screenshot. The viewport reverts to whatever projection
                     the user had set once the capture finishes. Defaults to the
                     current projection if omitted.
    """
    color_arg = tuple(highlight_color) if highlight_color else None
    screenshot = get_parashell_connection().get_active_screenshot(
        view_name,
        width,
        height,
        focus_object,
        display_mode,
        hide,
        show_only,
        highlight,
        color_arg,
        camera_mode,
    )
    if screenshot is not None and _is_valid_png_b64(screenshot):
        size_kb = len(screenshot) * 3 / 4 / 1024
        focus_part = f", focused on '{focus_object}'" if focus_object else ", fit-all"
        size_part = f"{width}x{height}" if width and height else "viewport size"
        overrides: list[str] = []
        if display_mode:
            overrides.append(f"display={display_mode}")
        if show_only:
            overrides.append(f"show_only={show_only}")
        if hide:
            overrides.append(f"hide={hide}")
        if highlight:
            overrides.append(f"highlight={highlight}")
        override_part = f". Overrides: {', '.join(overrides)}" if overrides else ""
        return [
            TextContent(
                type="text",
                text=f"Captured {view_name} view ({size_part}{focus_part}). PNG ~{size_kb:.1f} KB{override_part}.",
            ),
            make_image_content(screenshot),
        ]
    if screenshot is not None:
        return text_response(
            f"Failed to capture {view_name} view: the renderer returned an empty image. "
            "This usually means the active view does not support offscreen rendering, the "
            "document has no visible geometry, or the requested view preset is not "
            "available. Try a different view or verify that geometry exists."
        )
    return text_response(
        "Cannot get screenshot in the current view type (such as TechDraw or "
        "Spreadsheet). For TechDraw drawing pages, use get_techdraw_page to render "
        "the page to an image, or list_techdraw_pages to discover available pages."
    )
