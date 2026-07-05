from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from bridge import (
    _is_valid_png_b64,
    get_parashell_connection,
    make_image_content,
    mcp,
    text_response,
)


@mcp.tool()
def get_techdraw_page(
    ctx: Context,
    page_name: str | None = None,
    doc_name: str | None = None,
    width: int | None = None,
    height: int | None = None,
) -> list[ImageContent | TextContent]:
    """Render a TechDraw drawing page to a PNG image.

    TechDraw pages cannot be captured with the 3D viewport screenshot tools
    (get_view / get_ortho) because the drawing view has no offscreen 3D renderer.
    This tool exports the page to SVG via TechDraw and rasterizes it to a PNG so
    the drawing — its projected views, dimensions, annotations, and title block —
    can be seen directly. The page does not need to be the foreground tab.

    Args:
        page_name: Name or label of the TechDraw page to render. Defaults to the
                   first page in the document. Use list_techdraw_pages to discover names.
        doc_name: Name of the document. Defaults to the active document.
        width: Output width in pixels. If only one of width/height is given, the
               other is derived from the page aspect ratio. If both are omitted,
               the page is rendered at a legible resolution preserving its aspect ratio.
        height: Output height in pixels. See width.
    """
    result = get_parashell_connection().render_techdraw_page(
        page_name, doc_name, width, height
    )
    if result.get("success"):
        image_b64 = result.get("image")
        if image_b64 and _is_valid_png_b64(image_b64):
            size_kb = len(image_b64) * 3 / 4 / 1024
            target = page_name or "first page"
            size_part = f"{width}x{height}" if width and height else "auto-scaled"
            return [
                TextContent(
                    type="text",
                    text=f"Rendered TechDraw page ({target}, {size_part}). PNG ~{size_kb:.1f} KB.",
                ),
                make_image_content(image_b64),
            ]
        return text_response(
            "TechDraw page rendered but the image was empty or invalid."
        )
    return text_response(result.get("error", "Failed to render the TechDraw page."))
