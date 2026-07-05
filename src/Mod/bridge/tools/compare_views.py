from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Literal
from bridge import (
    _compose_diff_image,
    _is_valid_png_b64,
    get_parashell_connection,
    logger,
    make_image_content,
    mcp,
    state,
    text_response,
)


@mcp.tool()
def compare_views(
    ctx: Context,
    baseline_label: str,
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
    title: str | None = None,
) -> list[TextContent | ImageContent]:
    """Capture a current screenshot and diff it against a stored baseline.

    Returns a single composited image with three panels (baseline, current,
    pixel-level diff highlighted in red), plus a summary of how many pixels
    changed and the percentage of the frame that differs. Use this to
    programmatically confirm a stage moved exactly the geometry you expected
    instead of eyeballing two separate screenshots.

    Args:
        baseline_label: Label previously passed to capture_view_baseline.
        view_name: Standard view preset for the current capture.
        width / height: Screenshot pixel dimensions.
        focus_object: Object to focus on. Fits all if omitted.
        display_mode / hide / show_only: Same semantics as get_view.
        title: Optional title bar text drawn at the top of the diff image.
    """
    try:
        if baseline_label not in state.view_baselines:
            available = list(state.view_baselines.keys())
            return text_response(
                f"Baseline '{baseline_label}' not found. Available baselines: {available}"
            )
        before_b64 = state.view_baselines[baseline_label]

        parashell = get_parashell_connection()
        after_b64 = parashell.get_active_screenshot(
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
        if after_b64 is None:
            return text_response("Cannot get screenshot in the current view type")
        if not _is_valid_png_b64(after_b64):
            return text_response(
                "Failed to capture current view for diff: the renderer returned an "
                "empty image. Try a different view or verify the document has visible "
                "geometry."
            )
        if not _is_valid_png_b64(before_b64):
            return text_response(
                f"Stored baseline '{baseline_label}' is invalid (empty image). "
                "Recapture the baseline with capture_view_baseline and try again."
            )

        try:
            diff_b64, stats = _compose_diff_image(
                before_b64,
                after_b64,
                title or f"Diff: {baseline_label} \u2192 current",
                f"baseline:{baseline_label}",
                "current",
            )
        except ImportError:
            return [
                TextContent(
                    type="text",
                    text="Pillow not available for diff rendering — returning baseline and current side by side.",
                ),
                make_image_content(before_b64),
                make_image_content(after_b64),
            ]

        header = (
            f"Diff '{baseline_label}' vs current ({view_name}): "
            f"{stats['changed_pixels']} / {stats['total_pixels']} pixels changed "
            f"({stats['percent_changed']:.2f}%)."
        )
        if not _is_valid_png_b64(diff_b64):
            return text_response(f"{header} (diff rendering produced an empty image.)")
        return [
            TextContent(type="text", text=header),
            make_image_content(diff_b64),
        ]
    except Exception as e:
        logger.error(f"compare_views failed: {e}")
        return text_response(f"compare_views failed: {e}")
