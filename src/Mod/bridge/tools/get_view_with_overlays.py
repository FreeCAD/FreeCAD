from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Any, Literal
from bridge import (
    _draw_overlay_image,
    _is_valid_png_b64,
    get_parashell_connection,
    logger,
    make_image_content,
    mcp,
    text_response,
)


@mcp.tool()
def get_view_with_overlays(
    ctx: Context,
    doc_name: str,
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
    bbox_objects: list[str] | None = None,
    measurements: list[dict[str, Any]] | None = None,
    title: str | None = None,
    show_axis_legend: bool = True,
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
) -> list[ImageContent | TextContent]:
    """Get a viewport screenshot with axis legend, bounding-box dimensions, and distance overlays drawn on top.

    The base capture goes through the same pipeline as get_view (so display_mode,
    hide, show_only, highlight all work). Pillow then overlays:
      - An optional title bar at the top (handy for build-stage labels).
      - An XYZ axis legend in the corner (toggleable via show_axis_legend).
      - A bounding-box dimensions panel listing each requested object's
        x_length / y_length / z_length / center.
      - A measurements panel listing each requested distance with its label
        and per-axis components.

    measurements entries take the same form as measure_distance, plus an
    optional 'label':
      [
        {"a": "Tier1", "b": "Tier2", "label": "tier_gap"},
        {"a": [0, 0, 0], "b": "Sun#Vertex1", "label": "core_radius"}
      ]

    Args:
        doc_name: Document containing referenced objects.
        view_name: Standard view preset.
        bbox_objects: Object names / uids whose bbox dimensions should be drawn.
        measurements: List of distance entries (see above).
        title: Optional title shown across the top of the image.
        show_axis_legend: Whether to draw the XYZ axis legend. Default True.
        width / height: Screenshot pixel dimensions.
        focus_object: Object to focus on. Fits all if omitted.
        display_mode / hide / show_only / highlight: Same semantics as get_view.
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
            highlight,
            None,
        )
        if screenshot is None:
            return text_response("Cannot get screenshot in the current view type")
        if not _is_valid_png_b64(screenshot):
            return text_response(
                f"Failed to capture {view_name} view with overlays: the renderer "
                "returned an empty image. Try a different view or verify the "
                "document has visible geometry."
            )

        bbox_lines: list[str] = []
        for ident in bbox_objects or []:
            r = parashell.get_bounding_box(doc_name, ident)
            if r.get("success"):
                bb = r.get("bbox") or {}
                bbox_lines.append(
                    f"{r.get('object')}: {bb.get('x_length', 0):.2f} x "
                    f"{bb.get('y_length', 0):.2f} x {bb.get('z_length', 0):.2f} mm"
                )
            else:
                bbox_lines.append(f"{ident}: <error: {r.get('error')}>")

        meas_results: list[dict[str, Any]] = []
        for entry in measurements or []:
            if not isinstance(entry, dict):
                continue
            a = entry.get("a")
            b = entry.get("b")
            if a is None or b is None:
                continue
            r = parashell.measure_distance(doc_name, a, b)
            if r.get("success"):
                m = {k: v for k, v in r.items() if k != "success"}
                if entry.get("label"):
                    m["label"] = entry["label"]
                meas_results.append(m)

        try:
            overlay_b64 = _draw_overlay_image(
                screenshot,
                title or "",
                bbox_lines,
                meas_results,
                bool(show_axis_legend),
            )
        except ImportError:
            return [
                TextContent(
                    type="text",
                    text="Pillow not available for overlay rendering — returning plain screenshot.",
                ),
                make_image_content(screenshot),
            ]

        size_kb = len(overlay_b64) * 3 / 4 / 1024
        header = (
            f"Captured {view_name} view with overlays "
            f"(bbox={len(bbox_lines)}, measurements={len(meas_results)}). "
            f"PNG ~{size_kb:.1f} KB."
        )
        if not _is_valid_png_b64(overlay_b64):
            return [
                TextContent(
                    type="text",
                    text=f"{header} (overlay rendering produced an empty image; returning plain screenshot.)",
                ),
                make_image_content(screenshot),
            ]
        return [
            TextContent(type="text", text=header),
            make_image_content(overlay_b64),
        ]
    except Exception as e:
        logger.error(f"get_view_with_overlays failed: {e}")
        return text_response(f"get_view_with_overlays failed: {e}")
