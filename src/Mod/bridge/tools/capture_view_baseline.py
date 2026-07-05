from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Literal
from bridge import (
    _is_valid_png_b64,
    get_parashell_connection,
    logger,
    make_image_content,
    mcp,
    state,
    text_response,
)


@mcp.tool()
def capture_view_baseline(
    ctx: Context,
    label: str,
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
    ] = "Isometric",
    width: int | None = None,
    height: int | None = None,
    focus_object: str | None = None,
    display_mode: (
        Literal["Shaded", "Wireframe", "Flat Lines", "Hidden line", "Points", "As is"]
        | None
    ) = None,
    hide: list[str] | None = None,
    show_only: list[str] | None = None,
) -> list[TextContent | ImageContent]:
    """Capture a screenshot and store it under 'label' as a baseline for compare_views.

    Useful as the "before" snapshot before applying a change so a later
    compare_views(label, view_name=...) can show exactly what moved.

    Args:
        label: Identifier for the stored baseline (e.g. "before_chimney").
        view_name: Standard view preset.
        width / height: Screenshot pixel dimensions.
        focus_object: Object to focus on. Fits all if omitted.
        display_mode / hide / show_only: Same semantics as get_view.
    """
    try:
        parashell = get_parashell_connection()
        screenshot = parashell.get_active_screenshot(
            view_name,
            width,
            height,
            focus_object,
            display_mode,
            hide,
            show_only,
            None,
            None,
        )
        if screenshot is None:
            return text_response("Cannot get screenshot in the current view type")
        if not _is_valid_png_b64(screenshot):
            return text_response(
                "Failed to capture baseline: the renderer returned an empty image. "
                "The active view may not support offscreen rendering or the document "
                "may have no visible geometry."
            )
        state.view_baselines[label] = screenshot
        size_kb = len(screenshot) * 3 / 4 / 1024
        return [
            TextContent(
                type="text",
                text=f"Stored baseline '{label}' ({view_name}). PNG ~{size_kb:.1f} KB. Total baselines: {len(state.view_baselines)}.",
            ),
            make_image_content(screenshot),
        ]
    except Exception as e:
        logger.error(f"capture_view_baseline failed: {e}")
        return text_response(f"capture_view_baseline failed: {e}")
