import argparse
import base64
import importlib
import importlib.machinery
import io
import json
import logging
import os
import sys
import urllib.error
import urllib.request
import xmlrpc.client  # nosec B411 - XML-RPC is used only for the local authenticated bridge transport.
from contextlib import asynccontextmanager
from dataclasses import dataclass, field
from typing import Any, AsyncIterator, Dict, Literal, Union

from mcp.server.fastmcp import Context, FastMCP
from mcp.types import ImageContent, TextContent

import validators

import calcinator
import discovery

logger = logging.getLogger("ParashellMCPserver")
logger.addHandler(logging.NullHandler())
logger.propagate = False

for _noisy in (
    "mcp",
    "mcp.server",
    "mcp.server.lowlevel",
    "mcp.server.lowlevel.server",
):
    logging.getLogger(_noisy).setLevel(logging.WARNING)


def _configure_standalone_logging() -> None:
    handler = logging.StreamHandler()
    handler.setFormatter(
        logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    )
    logger.setLevel(logging.INFO)
    logger.addHandler(handler)
    root = logging.getLogger()
    if not root.handlers:
        root.setLevel(logging.WARNING)
        root.addHandler(handler)


_cortex_url: str | None = None


def _get_cortex_url() -> str:
    global _cortex_url
    if _cortex_url is None:
        override = os.environ.get("PARASHELL_CORTEX_URL", "").strip()
        if override:
            _cortex_url = discovery._service_url(override)
        else:
            _cortex_url = discovery.discover_cortex_url()
    return _cortex_url


ASSET_CREATION_STRATEGY = """
Asset Creation Strategy for Parashell MCP

Parashell is a fork of FreeCAD and is fully FreeCAD-compatible — its Python API,
object types (TypeIds like 'Part::Box', 'PartDesign::Body', 'Sketcher::SketchObject'),
and workbenches are identical to FreeCAD. When you call execute_code, the snippet
runs inside Parashell's interpreter, so import the API under its original names:
'import FreeCAD' and 'import FreeCADGui', exactly as in FreeCAD.

When creating content in Parashell, always follow these steps:

0. Before starting any task, always use get_objects() to confirm the current state of the document.

1. Utilize the parts library:
   - Check available parts using get_parts_list().
   - If the required part exists in the library, use insert_part_from_library() to insert it into your document.

2. If the appropriate asset is not available in the parts library:
   - Create basic shapes (e.g., cubes, cylinders, spheres) using create_object().
   - Adjust and define detailed properties of the shapes as necessary using edit_object().

3. Always assign clear and descriptive names to objects when adding them to the document.

4. Explicitly set the position, scale, and rotation properties of created or inserted objects using edit_object() to ensure proper spatial relationships.

5. After editing an object, always verify that the set properties have been correctly applied by using get_object().

6. If detailed customization or specialized operations are necessary, use execute_code() to run custom Python scripts.

Only revert to basic creation methods in the following cases:
- When the required asset is not available in the parts library.
- When a basic shape is explicitly requested.
- When creating complex shapes requires custom scripting.
"""

ToolResponse = list[Union[TextContent, ImageContent]]

_MAX_IMAGE_PIXELS = 1_000_000


def text_response(message: str) -> ToolResponse:
    return [TextContent(type="text", text=message)]


def json_response(data: object) -> ToolResponse:
    return text_response(json.dumps(data, ensure_ascii=False, indent=2, default=str))


def cap_image_megapixels(image_b64: str, max_pixels: int = _MAX_IMAGE_PIXELS) -> str:
    if not image_b64:
        return image_b64
    try:
        from PIL import Image
    except ImportError:
        return image_b64
    try:
        raw = base64.b64decode(image_b64)
    except (ValueError, TypeError):
        return image_b64
    try:
        with Image.open(io.BytesIO(raw)) as img:
            width, height = img.size
            pixels = width * height
            if pixels <= max_pixels or width == 0 or height == 0:
                return image_b64
            scale = (max_pixels / float(pixels)) ** 0.5
            new_width = max(1, int(width * scale))
            new_height = max(1, int(height * scale))
            resized = img.convert("RGB").resize((new_width, new_height), Image.LANCZOS)
            buf = io.BytesIO()
            resized.save(buf, format="PNG", optimize=True)
            return base64.b64encode(buf.getvalue()).decode("utf-8")
    except Exception as e:
        logger.warning(f"Failed to cap image megapixels: {e}")
        return image_b64


_PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
_PNG_IEND = b"IEND\xaeB`\x82"
_MIN_PNG_SIZE = 67


def _is_valid_png_b64(image_b64: str) -> bool:
    if not image_b64 or not isinstance(image_b64, str):
        return False
    try:
        raw = base64.b64decode(image_b64, validate=False)
    except (ValueError, TypeError):
        return False
    if len(raw) < _MIN_PNG_SIZE:
        return False
    if not raw.startswith(_PNG_SIGNATURE):
        return False
    if not raw.rstrip().endswith(_PNG_IEND):
        return False
    return True


def make_image_content(image_b64: str) -> ImageContent:
    return ImageContent(
        type="image", data=cap_image_megapixels(image_b64), mimeType="image/png"
    )


def image_content_or_text(
    image_b64: str | None, fallback_text: str
) -> list[TextContent | ImageContent]:
    if image_b64 and _is_valid_png_b64(image_b64):
        return [make_image_content(image_b64)]
    return [TextContent(type="text", text=fallback_text)]


def append_image_if_valid(
    response: list[TextContent | ImageContent], image_b64: str | None
) -> list[TextContent | ImageContent]:
    if image_b64 and _is_valid_png_b64(image_b64):
        return [*response, make_image_content(image_b64)]
    return response


def add_screenshot_if_available(
    response: ToolResponse, screenshot: str | None, only_text_feedback: bool
) -> ToolResponse:
    if only_text_feedback or screenshot is None:
        return response
    if not _is_valid_png_b64(screenshot):
        logger.warning("Discarding empty or invalid screenshot from response")
        return response
    return [
        *response,
        make_image_content(screenshot),
    ]


def _format_autosave_summary(autosave_res: dict[str, Any]) -> str:
    if not autosave_res.get("success"):
        return f"Autosave error: {autosave_res.get('error', 'unknown error')}"
    results = autosave_res.get("results") or []
    if not results:
        return "Autosave: no open documents."
    saved = [r for r in results if r.get("saved")]
    skipped = [r for r in results if not r.get("saved")]
    parts = [f"Autosave: {len(saved)} saved, {len(skipped)} skipped."]
    for r in saved:
        parts.append(f"  - {r['name']} -> {r['path']}")
    for r in skipped:
        parts.append(f"  - {r['name']} skipped: {r.get('reason', 'unknown')}")
    return "\n".join(parts)


def _compose_diff_image(
    before_b64: str,
    after_b64: str,
    title: str,
    label_before: str,
    label_after: str,
) -> tuple[str, dict[str, Any]]:
    from PIL import Image, ImageDraw, ImageFont, ImageChops

    img_a = Image.open(io.BytesIO(base64.b64decode(before_b64))).convert("RGB")
    img_b = Image.open(io.BytesIO(base64.b64decode(after_b64))).convert("RGB")

    target_w = max(img_a.width, img_b.width)
    target_h = max(img_a.height, img_b.height)
    if (img_a.width, img_a.height) != (target_w, target_h):
        img_a = img_a.resize((target_w, target_h), Image.LANCZOS)
    if (img_b.width, img_b.height) != (target_w, target_h):
        img_b = img_b.resize((target_w, target_h), Image.LANCZOS)

    diff = ImageChops.difference(img_a, img_b)
    diff_l = diff.convert("L")
    diff_pixels = diff_l.getdata()
    total = target_w * target_h
    changed = sum(1 for v in diff_pixels if v > 8)
    pct_changed = changed / total * 100.0 if total else 0.0

    diff_overlay = Image.new("RGB", (target_w, target_h), (15, 15, 25))
    mask = diff_l.point(lambda v: 255 if v > 8 else 0)
    red_layer = Image.new("RGB", (target_w, target_h), (240, 80, 80))
    diff_overlay.paste(red_layer, (0, 0), mask)

    padding = 6
    label_h = 24
    summary_h = 30
    panel_w = target_w * 3 + padding * 4
    panel_h = target_h + padding * 3 + label_h + 30 + summary_h
    canvas = Image.new("RGB", (panel_w, panel_h), (22, 22, 22))
    draw = ImageDraw.Draw(canvas)

    try:
        font = ImageFont.truetype(
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 14
        )
    except Exception:
        try:
            font = ImageFont.truetype("arial.ttf", 14)
        except Exception:
            font = ImageFont.load_default()

    draw.rectangle([0, 0, panel_w, 30], fill=(20, 20, 35))
    if title:
        try:
            bbox_t = draw.textbbox((0, 0), title, font=font)
            tw = bbox_t[2] - bbox_t[0]
            draw.text(((panel_w - tw) // 2, 8), title, fill=(230, 230, 240), font=font)
        except Exception:
            draw.text((10, 8), title, fill=(230, 230, 240), font=font)

    y_top = 30 + padding
    for i, (lbl, im) in enumerate(
        [(label_before, img_a), (label_after, img_b), ("Diff", diff_overlay)]
    ):
        x = padding + i * (target_w + padding)
        draw.rectangle([x, y_top, x + target_w, y_top + label_h], fill=(40, 40, 50))
        try:
            bbox_t = draw.textbbox((0, 0), lbl, font=font)
            tw = bbox_t[2] - bbox_t[0]
            draw.text(
                (x + (target_w - tw) // 2, y_top + 4),
                lbl,
                fill=(220, 230, 240),
                font=font,
            )
        except Exception:
            draw.text((x + 4, y_top + 4), lbl, fill=(220, 230, 240), font=font)
        canvas.paste(im, (x, y_top + label_h))

    summary = f"Changed pixels: {changed} / {total} ({pct_changed:.2f}%)"
    draw.text((padding, panel_h - 24), summary, fill=(220, 220, 230), font=font)

    buf = io.BytesIO()
    canvas.save(buf, format="PNG", optimize=True)
    return base64.b64encode(buf.getvalue()).decode("utf-8"), {
        "width": target_w,
        "height": target_h,
        "changed_pixels": int(changed),
        "total_pixels": int(total),
        "percent_changed": float(pct_changed),
    }


def _draw_overlay_image(
    base_b64: str,
    title: str,
    bbox_lines: list[str],
    measurements: list[dict[str, Any]],
    show_axis_legend: bool,
) -> str:
    from PIL import Image, ImageDraw, ImageFont

    img = Image.open(io.BytesIO(base64.b64decode(base_b64))).convert("RGB")
    draw = ImageDraw.Draw(img, "RGBA")

    try:
        font = ImageFont.truetype(
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 14
        )
        font_small = ImageFont.truetype(
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12
        )
    except Exception:
        try:
            font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 14)
            font_small = font
        except Exception:
            try:
                font = ImageFont.truetype("arial.ttf", 14)
                font_small = font
            except Exception:
                font = ImageFont.load_default()
                font_small = font

    width, height = img.size

    if title:
        bar_height = 30
        draw.rectangle([0, 0, width, bar_height], fill=(20, 20, 30, 200))
        try:
            bbox_t = draw.textbbox((0, 0), title, font=font)
            tw = bbox_t[2] - bbox_t[0]
            draw.text(
                ((width - tw) // 2, (bar_height - (bbox_t[3] - bbox_t[1])) // 2),
                title,
                fill=(240, 240, 240),
                font=font,
            )
        except Exception:
            draw.text((10, 8), title, fill=(240, 240, 240), font=font)

    if bbox_lines:
        line_h = 18
        panel_w = max(
            180,
            max(
                (draw.textlength(line, font=font_small) for line in bbox_lines),
                default=180,
            )
            + 20,
        )
        panel_h = line_h * len(bbox_lines) + 16
        x0 = 10
        y0 = height - panel_h - 10
        draw.rectangle(
            [x0, y0, x0 + panel_w, y0 + panel_h],
            fill=(20, 20, 30, 220),
            outline=(120, 200, 255, 220),
            width=1,
        )
        for i, line in enumerate(bbox_lines):
            draw.text(
                (x0 + 10, y0 + 8 + i * line_h),
                line,
                fill=(220, 230, 240),
                font=font_small,
            )

    if show_axis_legend:
        leg_w = 92
        leg_h = 92
        cx = width - leg_w - 10
        cy = 10 if not title else 40
        draw.rectangle(
            [cx, cy, cx + leg_w, cy + leg_h],
            fill=(15, 15, 25, 210),
            outline=(80, 80, 100, 220),
            width=1,
        )
        origin = (cx + 24, cy + leg_h - 24)
        draw.line(
            [origin, (origin[0] + 50, origin[1])], fill=(220, 60, 60, 255), width=3
        )
        draw.text(
            (origin[0] + 54, origin[1] - 8), "X", fill=(220, 60, 60), font=font_small
        )
        draw.line(
            [origin, (origin[0], origin[1] - 50)], fill=(80, 200, 80, 255), width=3
        )
        draw.text(
            (origin[0] - 4, origin[1] - 70), "Y", fill=(80, 200, 80), font=font_small
        )
        draw.line(
            [origin, (origin[0] - 30, origin[1] - 30)],
            fill=(80, 140, 240, 255),
            width=3,
        )
        draw.text(
            (origin[0] - 44, origin[1] - 44), "Z", fill=(80, 140, 240), font=font_small
        )

    if measurements:
        right_panel_lines: list[str] = []
        for m in measurements:
            label = str(m.get("label") or f"{m.get('a','?')}\u2192{m.get('b','?')}")
            distance = m.get("distance")
            if distance is None:
                continue
            right_panel_lines.append(f"{label}: {distance:.3f} mm")
            for axis_key, axis_label in (
                ("distance_x", "dx"),
                ("distance_y", "dy"),
                ("distance_z", "dz"),
            ):
                if m.get(axis_key) is not None and m.get(axis_key, 0) > 1e-6:
                    right_panel_lines.append(f"  {axis_label}: {m[axis_key]:.3f}")

        if right_panel_lines:
            line_h = 16
            panel_w = max(
                180,
                max(
                    (
                        draw.textlength(line, font=font_small)
                        for line in right_panel_lines
                    ),
                    default=180,
                )
                + 20,
            )
            panel_h = line_h * len(right_panel_lines) + 16
            x0 = width - panel_w - 10
            y0 = height - panel_h - 10
            draw.rectangle(
                [x0, y0, x0 + panel_w, y0 + panel_h],
                fill=(20, 20, 30, 220),
                outline=(255, 200, 80, 220),
                width=1,
            )
            for i, line in enumerate(right_panel_lines):
                draw.text(
                    (x0 + 10, y0 + 8 + i * line_h),
                    line,
                    fill=(245, 230, 200),
                    font=font_small,
                )

    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return base64.b64encode(buf.getvalue()).decode("utf-8")


def _fit_into_cell(img, box_w: int, box_h: int, bg: tuple = (22, 22, 22)):
    from PIL import Image

    box_w = max(1, int(box_w))
    box_h = max(1, int(box_h))
    src_w, src_h = img.size
    cell = Image.new("RGB", (box_w, box_h), bg)
    if src_w <= 0 or src_h <= 0:
        return cell
    scale = min(box_w / src_w, box_h / src_h)
    new_w = max(1, min(box_w, int(round(src_w * scale))))
    new_h = max(1, min(box_h, int(round(src_h * scale))))
    resized = img.resize((new_w, new_h), Image.LANCZOS)
    cell.paste(resized, ((box_w - new_w) // 2, (box_h - new_h) // 2))
    return cell


def _composite_ortho_views(views_data: dict[str, str], tile_w: int, tile_h: int) -> str:
    from PIL import Image, ImageDraw, ImageFont

    standard_order = ["Front", "Right", "Top", "Back", "Left", "Bottom", "Isometric"]
    ortho = [(k, v) for k, v in views_data.items() if k != "Isometric"]
    ortho.sort(
        key=lambda x: standard_order.index(x[0]) if x[0] in standard_order else 99
    )
    iso = [(k, v) for k, v in views_data.items() if k == "Isometric"]

    cols = 3
    padding = 6
    label_h = 28
    border = 2

    rows = (len(ortho) + cols - 1) // cols
    total_rows = rows + (1 if iso else 0)
    total_w = cols * tile_w + (cols + 1) * padding
    total_h = total_rows * (tile_h + label_h) + (total_rows + 1) * padding

    canvas = Image.new("RGB", (total_w, total_h), (22, 22, 22))
    draw = ImageDraw.Draw(canvas)

    try:
        font = ImageFont.truetype(
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 14
        )
        font_small = font
    except Exception:
        try:
            font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 14)
            font_small = font
        except Exception:
            font = ImageFont.load_default()
            font_small = font

    label_bg = (40, 40, 40)
    label_fg = (210, 210, 210)
    border_col = (60, 60, 60)

    for i, (name, b64data) in enumerate(ortho):
        row = i // cols
        col = i % cols
        x = padding + col * (tile_w + padding)
        y = padding + row * (tile_h + label_h + padding)

        draw.rectangle(
            [
                x - border,
                y - border,
                x + tile_w + border,
                y + tile_h + label_h + border,
            ],
            fill=border_col,
        )
        draw.rectangle([x, y, x + tile_w, y + label_h], fill=label_bg)

        try:
            bbox = draw.textbbox((0, 0), name, font=font)
            tw = bbox[2] - bbox[0]
            draw.text(
                (x + (tile_w - tw) // 2, y + (label_h - (bbox[3] - bbox[1])) // 2),
                name,
                fill=label_fg,
                font=font,
            )
        except Exception:
            draw.text((x + 4, y + 6), name, fill=label_fg, font=font)

        try:
            tile_img = _fit_into_cell(
                Image.open(io.BytesIO(base64.b64decode(b64data))).convert("RGB"),
                tile_w,
                tile_h,
            )
            canvas.paste(tile_img, (x, y + label_h))
        except Exception as e:
            draw.rectangle(
                [x, y + label_h, x + tile_w, y + label_h + tile_h], fill=(40, 10, 10)
            )
            draw.text(
                (x + 4, y + label_h + 4),
                f"Error: {e}",
                fill=(200, 80, 80),
                font=font_small,
            )

    if iso:
        name, b64data = iso[0]
        iso_w = total_w - 2 * padding
        iso_h = tile_h
        x = padding
        y = padding + rows * (tile_h + label_h + padding)

        draw.rectangle(
            [x - border, y - border, x + iso_w + border, y + iso_h + label_h + border],
            fill=border_col,
        )
        draw.rectangle([x, y, x + iso_w, y + label_h], fill=(50, 40, 60))

        try:
            bbox = draw.textbbox((0, 0), name, font=font)
            tw = bbox[2] - bbox[0]
            draw.text(
                (x + (iso_w - tw) // 2, y + (label_h - (bbox[3] - bbox[1])) // 2),
                name,
                fill=(200, 180, 230),
                font=font,
            )
        except Exception:
            draw.text((x + 4, y + 6), name, fill=(200, 180, 230), font=font)

        try:
            tile_img = _fit_into_cell(
                Image.open(io.BytesIO(base64.b64decode(b64data))).convert("RGB"),
                iso_w,
                iso_h,
            )
            canvas.paste(tile_img, (x, y + label_h))
        except Exception as e:
            draw.rectangle(
                [x, y + label_h, x + iso_w, y + label_h + iso_h], fill=(40, 10, 10)
            )
            draw.text(
                (x + 4, y + label_h + 4),
                f"Error: {e}",
                fill=(200, 80, 80),
                font=font_small,
            )

    buf = io.BytesIO()
    canvas.save(buf, format="PNG", optimize=True)
    return base64.b64encode(buf.getvalue()).decode("utf-8")


class ParashellConnection:
    def __init__(self, host: str, port: int, token: str):
        self.server = discovery.make_server_proxy(host, port, token)

    def disconnect(self) -> None:
        transport = getattr(self.server, "_ServerProxy__transport", None)
        close = getattr(transport, "close", None)
        if callable(close):
            close()

    def ping(self) -> bool:
        return self.server.ping()

    def create_document(self, name: str) -> dict[str, Any]:
        return self.server.create_document(name)

    def create_object(
        self, doc_name: str, obj_data: dict[str, Any], transaction_id: str
    ) -> dict[str, Any]:
        return self.server.create_object(doc_name, obj_data, transaction_id)

    def edit_object(
        self,
        doc_name: str,
        obj_name: str,
        obj_data: dict[str, Any],
        transaction_id: str,
    ) -> dict[str, Any]:
        return self.server.edit_object(doc_name, obj_name, obj_data, transaction_id)

    def delete_object(
        self, doc_name: str, obj_name: str, transaction_id: str
    ) -> dict[str, Any]:
        return self.server.delete_object(doc_name, obj_name, transaction_id)

    def insert_part_from_library(
        self, relative_path: str, transaction_id: str
    ) -> dict[str, Any]:
        return self.server.insert_part_from_library(relative_path, transaction_id)

    def execute_code(self, code: str, transaction_id: str) -> dict[str, Any]:
        return self.server.execute_code(code, transaction_id)

    def request_code_approval(
        self, code: str, tag: str = "", reason: str = "", expected_action: str = ""
    ) -> dict[str, Any]:
        return self.server.request_code_approval(code, tag, reason, expected_action)

    def autosave(
        self, doc_name: str | None = None, fallback_dir: str | None = None
    ) -> dict[str, Any]:
        return self.server.autosave(doc_name, fallback_dir)

    def get_active_screenshot(
        self,
        view_name: str = "Isometric",
        width: int | None = None,
        height: int | None = None,
        focus_object: str | None = None,
        display_mode: str | None = None,
        hide: list[str] | None = None,
        show_only: list[str] | None = None,
        highlight: list[str] | None = None,
        highlight_color: tuple | None = None,
        camera_mode: str | None = None,
    ) -> str | None:
        try:
            result = self.server.check_screenshot_support()
            if not result.get("success", False) or not result.get("supported", False):
                logger.info("Screenshot unavailable in current view")
                return None
            return self.server.get_active_screenshot(
                view_name,
                width,
                height,
                focus_object,
                display_mode,
                hide,
                show_only,
                highlight,
                highlight_color,
                camera_mode,
            )
        except Exception as e:
            logger.error(f"Error getting screenshot: {e}")
            return None

    def get_ortho_views(
        self,
        views: list[str] | None = None,
        tile_width: int = 800,
        tile_height: int = 600,
        focus_object: str | None = None,
        display_mode: str | None = None,
        hide: list[str] | None = None,
        show_only: list[str] | None = None,
        highlight: list[str] | None = None,
        highlight_color: tuple | None = None,
        camera_mode: str | None = None,
    ) -> dict[str, str]:
        try:
            return self.server.get_ortho_views(
                views or [],
                tile_width,
                tile_height,
                focus_object,
                display_mode,
                hide,
                show_only,
                highlight,
                highlight_color,
                camera_mode,
            )
        except Exception as e:
            logger.error(f"Error getting ortho views: {e}")
            return {}

    def get_sketch_view(
        self,
        doc_name: str,
        sketch_name: str,
        width: int | None = None,
        height: int | None = None,
    ) -> str | None:
        try:
            result = self.server.check_screenshot_support()
            if not result.get("success", False) or not result.get("supported", False):
                logger.info("Screenshot unavailable in current view")
                return None
            return self.server.get_sketch_view(doc_name, sketch_name, width, height)
        except Exception as e:
            logger.error(f"Error getting sketch view: {e}")
            return None

    def take_snapshot(self, doc_name: str, include_subelements: bool) -> dict[str, Any]:
        return self.server.take_snapshot(doc_name, include_subelements)

    def move_viewport(
        self,
        action: str,
        params: dict[str, Any] | None = None,
        take_screenshot: bool = True,
        width: int | None = None,
        height: int | None = None,
    ) -> dict[str, Any]:
        try:
            return self.server.move_viewport(
                action, params or {}, take_screenshot, width, height
            )
        except Exception as e:
            logger.error(f"Error in move_viewport: {e}")
            return {"success": False, "error": str(e), "screenshot": None}

    def reset_view(
        self,
        take_screenshot: bool = True,
        width: int | None = None,
        height: int | None = None,
    ) -> dict[str, Any]:
        try:
            return self.server.reset_view(take_screenshot, width, height)
        except Exception as e:
            logger.error(f"Error in reset_view: {e}")
            return {"success": False, "error": str(e), "screenshot": None}

    def get_objects(self, doc_name: str) -> list[dict[str, Any]]:
        return self.server.get_objects(doc_name)

    def get_object(self, doc_name: str, obj_name: str) -> dict[str, Any]:
        return self.server.get_object(doc_name, obj_name)

    def get_selection(self) -> dict[str, Any]:
        return self.server.get_selection()

    def create_spreadsheet(
        self, doc_name: str, name: str, label: str | None = None
    ) -> dict[str, Any]:
        return self.server.create_spreadsheet(doc_name, name, label)

    def list_spreadsheets(self, doc_name: str) -> dict[str, Any]:
        return self.server.list_spreadsheets(doc_name)

    def get_spreadsheet(
        self, doc_name: str, sheet_name: str, cell_range: list[str] | str | None = None
    ) -> dict[str, Any]:
        return self.server.get_spreadsheet(doc_name, sheet_name, cell_range)

    def set_spreadsheet_cells(
        self,
        doc_name: str,
        sheet_name: str,
        cells: dict[str, Any],
        recompute: bool = True,
    ) -> dict[str, Any]:
        return self.server.set_spreadsheet_cells(doc_name, sheet_name, cells, recompute)

    def clear_spreadsheet_cells(
        self,
        doc_name: str,
        sheet_name: str,
        ranges: list[str],
        recompute: bool = True,
    ) -> dict[str, Any]:
        return self.server.clear_spreadsheet_cells(
            doc_name, sheet_name, ranges, recompute
        )

    def set_spreadsheet_aliases(
        self,
        doc_name: str,
        sheet_name: str,
        aliases: dict[str, str],
        recompute: bool = True,
    ) -> dict[str, Any]:
        return self.server.set_spreadsheet_aliases(
            doc_name, sheet_name, aliases, recompute
        )

    def style_spreadsheet_cells(
        self,
        doc_name: str,
        sheet_name: str,
        targets: list[str],
        style: list[str] | None,
        foreground: list[float] | None,
        background: list[float] | None,
        alignment: list[str] | None,
        display_unit: str | None,
        style_options: str,
        recompute: bool,
    ) -> dict[str, Any]:
        return self.server.style_spreadsheet_cells(
            doc_name,
            sheet_name,
            targets,
            style,
            foreground,
            background,
            alignment,
            display_unit,
            style_options,
            recompute,
        )

    def set_spreadsheet_dimensions(
        self,
        doc_name: str,
        sheet_name: str,
        column_widths: dict[str, int] | None,
        row_heights: dict[str, int] | None,
        recompute: bool,
    ) -> dict[str, Any]:
        return self.server.set_spreadsheet_dimensions(
            doc_name, sheet_name, column_widths, row_heights, recompute
        )

    def merge_spreadsheet_cells(
        self,
        doc_name: str,
        sheet_name: str,
        ranges: list[str],
        action: str,
        recompute: bool,
    ) -> dict[str, Any]:
        return self.server.merge_spreadsheet_cells(
            doc_name, sheet_name, ranges, action, recompute
        )

    def modify_spreadsheet_structure(
        self,
        doc_name: str,
        sheet_name: str,
        operation: str,
        index: str,
        count: int,
        recompute: bool,
    ) -> dict[str, Any]:
        return self.server.modify_spreadsheet_structure(
            doc_name, sheet_name, operation, index, count, recompute
        )

    def import_spreadsheet_csv(
        self,
        doc_name: str,
        sheet_name: str,
        file_path: str,
        delimiter: str,
        recompute: bool,
    ) -> dict[str, Any]:
        return self.server.import_spreadsheet_csv(
            doc_name, sheet_name, file_path, delimiter, recompute
        )

    def export_spreadsheet_csv(
        self, doc_name: str, sheet_name: str, file_path: str, delimiter: str
    ) -> dict[str, Any]:
        return self.server.export_spreadsheet_csv(
            doc_name, sheet_name, file_path, delimiter
        )

    def get_parts_list(self) -> list[str]:
        return self.server.get_parts_list()

    def get_sketch(self, doc_name: str, sketch_name: str) -> dict[str, Any]:
        return self.server.get_sketch(doc_name, sketch_name)

    def edit_sketch_geometry(
        self,
        doc_name: str,
        sketch_name: str,
        operation: str,
        geometry: list[dict[str, Any]] | None,
        delete_indices: list[int] | None,
        recompute: bool,
    ) -> dict[str, Any]:
        return self.server.edit_sketch_geometry(
            doc_name, sketch_name, operation, geometry, delete_indices, recompute
        )

    def add_sketch_constraints(
        self,
        doc_name: str,
        sketch_name: str,
        constraints: list[dict[str, Any]],
        recompute: bool,
    ) -> dict[str, Any]:
        return self.server.add_sketch_constraints(
            doc_name, sketch_name, constraints, recompute
        )

    def clear_sketch_constraints(
        self,
        doc_name: str,
        sketch_name: str,
        indices: list[int] | None,
        recompute: bool,
    ) -> dict[str, Any]:
        return self.server.clear_sketch_constraints(
            doc_name, sketch_name, indices, recompute
        )

    def recompute_document(
        self,
        doc_name: str | None,
        force: bool,
        clear_redo: bool,
    ) -> dict[str, Any]:
        return self.server.recompute_document(doc_name, force, clear_redo)

    def check_objects(
        self,
        doc_name: str,
        obj_names: list[str] | None,
        only_unhealthy: bool,
    ) -> dict[str, Any]:
        return self.server.check_objects(doc_name, obj_names, only_unhealthy)

    def get_shape_info(self, doc_name: str, obj_name: str) -> dict[str, Any]:
        return self.server.get_shape_info(doc_name, obj_name)

    def get_world_state(
        self,
        doc_name: str | None,
        include_feature_tree: bool,
        include_geometry: bool,
        max_objects: int,
    ) -> dict[str, Any]:
        return self.server.get_world_state(
            doc_name, include_feature_tree, include_geometry, max_objects
        )

    def get_feature_tree(
        self,
        doc_name: str,
        body_name: str | None,
        include_orphans: bool,
    ) -> dict[str, Any]:
        return self.server.get_feature_tree(doc_name, body_name, include_orphans)

    def check_geometry(
        self,
        doc_name: str,
        obj_names: list[str] | None,
        run_bop_check: bool,
        min_edge_length: float,
        min_face_area: float,
        only_invalid: bool,
    ) -> dict[str, Any]:
        return self.server.check_geometry(
            doc_name,
            obj_names,
            run_bop_check,
            min_edge_length,
            min_face_area,
            only_invalid,
        )

    def analyze_mass_properties(
        self,
        doc_name: str,
        target: str,
        density: float | None,
    ) -> dict[str, Any]:
        return self.server.analyze_mass_properties(doc_name, target, density)

    def verify_solid(
        self,
        doc_name: str,
        target: str,
        samples: int,
        tolerance: float,
        expected_solids: int,
        min_fill_ratio: float,
        max_volume_discrepancy: float,
    ) -> dict[str, Any]:
        return self.server.verify_solid(
            doc_name,
            target,
            samples,
            tolerance,
            expected_solids,
            min_fill_ratio,
            max_volume_discrepancy,
        )

    def check_interferences(
        self,
        doc_name: str,
        obj_names: list[str] | None,
        clearance: float,
        compute_volume: bool,
    ) -> dict[str, Any]:
        return self.server.check_interferences(
            doc_name, obj_names, clearance, compute_volume
        )

    def validate_document(
        self,
        doc_name: str,
        deep_geometry: bool,
        run_bop_check: bool,
    ) -> dict[str, Any]:
        return self.server.validate_document(doc_name, deep_geometry, run_bop_check)

    def list_documents(self) -> list[str]:
        return self.server.list_documents()

    def list_techdraw_pages(self, doc_name: str | None = None) -> dict[str, Any]:
        return self.server.list_techdraw_pages(doc_name)

    def render_techdraw_page(
        self,
        page_name: str | None = None,
        doc_name: str | None = None,
        width: int | None = None,
        height: int | None = None,
    ) -> dict[str, Any]:
        return self.server.render_techdraw_page(page_name, doc_name, width, height)

    def create_objects(
        self, doc_name: str, items: list[dict[str, Any]], recompute: bool
    ) -> dict[str, Any]:
        return self.server.create_objects(doc_name, items, recompute)

    def create_pattern(
        self,
        doc_name: str,
        source_object: str,
        kind: str,
        count: int,
        params: dict[str, Any] | None,
        copy_mode: str,
        name_prefix: str | None,
        recompute: bool,
    ) -> dict[str, Any]:
        return self.server.create_pattern(
            doc_name,
            source_object,
            kind,
            count,
            params or {},
            copy_mode,
            name_prefix,
            recompute,
        )

    def make_ring(
        self, doc_name: str, name: str, params: dict[str, Any], recompute: bool
    ) -> dict[str, Any]:
        return self.server.make_ring(doc_name, name, params, recompute)

    def make_strut_between_points(
        self, doc_name: str, name: str, params: dict[str, Any], recompute: bool
    ) -> dict[str, Any]:
        return self.server.make_strut_between_points(doc_name, name, params, recompute)

    def make_segment_between_points(
        self, doc_name: str, name: str, params: dict[str, Any], recompute: bool
    ) -> dict[str, Any]:
        return self.server.make_segment_between_points(
            doc_name, name, params, recompute
        )

    def make_gear(
        self, doc_name: str, name: str, params: dict[str, Any], recompute: bool
    ) -> dict[str, Any]:
        return self.server.make_gear(doc_name, name, params, recompute)

    def boolean_op(
        self,
        doc_name: str,
        operation: str,
        operands: list[str],
        result_name: str | None,
        delete_operands: bool,
        recompute: bool,
        verify_growth: bool = True,
    ) -> dict[str, Any]:
        return self.server.boolean_op(
            doc_name,
            operation,
            operands,
            result_name,
            delete_operands,
            recompute,
            verify_growth,
        )

    def make_hollow_box(
        self, doc_name: str, name: str, params: dict[str, Any], recompute: bool
    ) -> dict[str, Any]:
        return self.server.make_hollow_box(doc_name, name, params, recompute)

    def make_hollow_cylinder(
        self, doc_name: str, name: str, params: dict[str, Any], recompute: bool
    ) -> dict[str, Any]:
        return self.server.make_hollow_cylinder(doc_name, name, params, recompute)

    def make_arch_opening(
        self, doc_name: str, name: str, params: dict[str, Any], recompute: bool
    ) -> dict[str, Any]:
        return self.server.make_arch_opening(doc_name, name, params, recompute)

    def make_crenellated_wall(
        self, doc_name: str, name: str, params: dict[str, Any], recompute: bool
    ) -> dict[str, Any]:
        return self.server.make_crenellated_wall(doc_name, name, params, recompute)

    def make_pyramid(
        self, doc_name: str, name: str, params: dict[str, Any], recompute: bool
    ) -> dict[str, Any]:
        return self.server.make_pyramid(doc_name, name, params, recompute)

    def make_sketch_extrude(
        self, doc_name: str, name: str, params: dict[str, Any], recompute: bool
    ) -> dict[str, Any]:
        return self.server.make_sketch_extrude(doc_name, name, params, recompute)

    def set_appearance(
        self, doc_name: str, targets: list[str], properties: dict[str, Any]
    ) -> dict[str, Any]:
        return self.server.set_appearance(doc_name, targets, properties)

    def get_bounding_box(self, doc_name: str, target: str) -> dict[str, Any]:
        return self.server.get_bounding_box(doc_name, target)

    def measure_distance(self, doc_name: str, a, b) -> dict[str, Any]:
        return self.server.measure_distance(doc_name, a, b)

    def create_group(
        self, doc_name: str, name: str, members: list[str] | None, parent: str | None
    ) -> dict[str, Any]:
        return self.server.create_group(doc_name, name, members or [], parent)

    def add_to_group(
        self, doc_name: str, group: str, members: list[str]
    ) -> dict[str, Any]:
        return self.server.add_to_group(doc_name, group, members)

    def remove_from_group(
        self, doc_name: str, group: str, members: list[str]
    ) -> dict[str, Any]:
        return self.server.remove_from_group(doc_name, group, members)

    def transaction_create(
        self, doc_name: str, label: str | None = None, reason: str | None = None
    ) -> dict[str, Any]:
        return self.server.transaction_create(doc_name, label, reason)

    def transaction_plan(self, transaction_id: str) -> dict[str, Any]:
        return self.server.transaction_plan(transaction_id)

    def transaction_apply(self, transaction_id: str) -> dict[str, Any]:
        return self.server.transaction_apply(transaction_id)

    def transaction_cancel(self, transaction_id: str) -> dict[str, Any]:
        return self.server.transaction_cancel(transaction_id)

    def transaction_delete(self, transaction_id: str) -> dict[str, Any]:
        return self.server.transaction_delete(transaction_id)

    def transaction_list(self, doc_name: str | None = None) -> dict[str, Any]:
        return self.server.transaction_list(doc_name)

    def transaction_get(self, transaction_id: str) -> dict[str, Any]:
        return self.server.transaction_get(transaction_id)

    def rollback_to_transaction(self, transaction_id: str) -> dict[str, Any]:
        return self.server.rollback_to_transaction(transaction_id)

    def is_file_saved(self, doc_name: str | None = None) -> dict[str, Any]:
        return self.server.is_file_saved(doc_name)

    def request_file_save(
        self, doc_name: str | None = None, suggested_name: str | None = None
    ) -> dict[str, Any]:
        return self.server.request_file_save(doc_name, suggested_name)

    def check_screenshot_support(self) -> dict[str, Any]:
        return self.server.check_screenshot_support()


@dataclass
class ServerState:
    only_text_feedback: bool = False
    rpc_host: str | None = None
    rpc_port: int | None = None
    rpc_token: str | None = None
    parashell_connection: ParashellConnection | None = None
    view_baselines: dict[str, str] = field(default_factory=dict)


state = ServerState()


@asynccontextmanager
async def server_lifespan(server: FastMCP) -> AsyncIterator[Dict[str, Any]]:
    try:
        logger.info("ParashellMCP server starting up")
        try:
            _ = get_parashell_connection()
            logger.info("Successfully connected to Parashell on startup")
        except Exception as e:
            logger.warning(f"Could not connect to Parashell on startup: {str(e)}")
            logger.warning(
                "Make sure the Parashell addon is running before using Parashell resources or tools"
            )
        yield {}
    finally:
        if state.parashell_connection:
            logger.info("Disconnecting from Parashell on shutdown")
            state.parashell_connection.disconnect()
            state.parashell_connection = None
        logger.info("ParashellMCP server shut down")


mcp = FastMCP(
    "ParashellMCP",
    instructions=(
        "Parashell integration through the Model Context Protocol. "
        "Parashell is a fork of FreeCAD and is fully FreeCAD-compatible: its "
        "Python API, object types, and workbenches are identical to FreeCAD. "
        "Any code passed to execute_code runs inside Parashell's interpreter and "
        "must import the API under its original module names — use 'import FreeCAD' "
        "and 'import FreeCADGui' exactly as you would in FreeCAD. Object TypeIds "
        "such as 'Part::Box', 'PartDesign::Body', and 'Sketcher::SketchObject' are "
        "also identical to FreeCAD."
    ),
    lifespan=server_lifespan,
)


def _resolve_endpoint() -> tuple[str, int, str]:
    if state.rpc_host and state.rpc_port and state.rpc_token:
        return state.rpc_host, state.rpc_port, state.rpc_token
    descriptor = discovery.discover(timeout=10.0)
    if descriptor is None:
        raise Exception(
            "Could not locate the Parashell bridge. Make sure the Parashell addon is running."
        )
    return descriptor["host"], descriptor["port"], descriptor["token"]


def get_parashell_connection() -> ParashellConnection:
    if state.parashell_connection is None:
        host, port, token = _resolve_endpoint()
        connection = ParashellConnection(host, port, token)
        try:
            reachable = connection.ping()
        except Exception as exc:
            logger.error(f"Failed to ping Parashell: {exc}")
            reachable = False
        if not reachable:
            raise Exception(
                "Failed to connect to Parashell. Make sure the Parashell addon is running."
            )
        state.parashell_connection = connection
    return state.parashell_connection


def _validate_word_count(
    text: str, field_name: str, min_words: int, max_words: int
) -> str | None:
    if not isinstance(text, str) or not text.strip():
        return f"The '{field_name}' field is required and cannot be empty."
    count = len(text.split())
    if count < min_words or count > max_words:
        return (
            f"The '{field_name}' field must be between {min_words} and "
            f"{max_words} words, but got {count}. Rewrite it and try again."
        )
    return None


def _run_approved_code(
    code: str, tag: str, transaction_id: str
) -> list[TextContent | ImageContent]:
    try:
        parashell = get_parashell_connection()
        autosave_res = parashell.autosave()
        autosave_summary = _format_autosave_summary(autosave_res)
        res = parashell.execute_code(code, transaction_id)
        screenshot = parashell.get_active_screenshot()
        if res["success"]:
            prefix = (
                f"Cortex flagged this and the user acknowledged with APPROVE, {tag}\n"
                if tag
                else ""
            )
            response = text_response(
                f"{prefix}{autosave_summary}\nCode executed successfully: {res['message']}"
            )
        else:
            response = text_response(
                f"{autosave_summary}\nFailed to execute code: {res['error']}"
            )
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        return text_response(f"Failed to execute code: {str(e)}")


def asset_creation_strategy() -> str:
    return ASSET_CREATION_STRATEGY


def _validate_host(value: str) -> str:
    if validators.ipv4(value) or validators.ipv6(value) or validators.hostname(value):
        return value
    raise argparse.ArgumentTypeError(
        f"Invalid host: '{value}'. Must be a valid IP address or hostname."
    )


_BRIDGE_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_DIR = os.path.join(_BRIDGE_DIR, "tools")
_tools_loaded = False


def _load_tools() -> None:
    global _tools_loaded
    if _tools_loaded:
        return
    if not os.path.isdir(_TOOLS_DIR):
        raise RuntimeError(f"Bridge tools directory missing: {_TOOLS_DIR}")
    if _TOOLS_DIR not in sys.path:
        sys.path.insert(0, _TOOLS_DIR)
    suffixes = tuple(importlib.machinery.SOURCE_SUFFIXES) + tuple(
        importlib.machinery.EXTENSION_SUFFIXES
    )
    seen: set[str] = set()
    names: list[str] = []
    for entry in sorted(os.listdir(_TOOLS_DIR)):
        if entry.startswith("_"):
            continue
        stem = None
        for suffix in suffixes:
            if entry.endswith(suffix):
                stem = entry[: -len(suffix)]
                break
        if stem is None or stem in seen:
            continue
        seen.add(stem)
        names.append(stem)
    if not names:
        raise RuntimeError(f"No Bridge tool modules found in {_TOOLS_DIR}")
    for name in names:
        importlib.import_module(name)
    _tools_loaded = True


def main():
    _configure_standalone_logging()
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--only-text-feedback", action="store_true", help="Only return text feedback"
    )
    parser.add_argument(
        "--host",
        type=_validate_host,
        default=None,
        help="Override the Parashell RPC host (defaults to secure discovery)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=None,
        help="Override the Parashell RPC port (defaults to secure discovery)",
    )
    parser.add_argument(
        "--token",
        default=None,
        help="Override the Parashell RPC auth token (defaults to secure discovery)",
    )
    args = parser.parse_args()
    state.only_text_feedback = args.only_text_feedback
    state.rpc_host = args.host
    state.rpc_port = args.port
    state.rpc_token = args.token
    logger.info(f"Only text feedback: {state.only_text_feedback}")
    if state.rpc_host and state.rpc_port and state.rpc_token:
        logger.info(
            f"Using explicit Parashell RPC endpoint at: {state.rpc_host}:{state.rpc_port}"
        )
    else:
        logger.info("Using secure discovery to locate the Parashell RPC server")
    _load_tools()
    import acs

    acs.register()
    mcp.run(transport="stdio")


_load_tools()


if __name__ == "__main__":
    main()
