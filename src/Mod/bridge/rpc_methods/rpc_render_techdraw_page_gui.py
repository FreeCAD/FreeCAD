from __future__ import annotations

from rpc import *


@rpc_method
def _render_techdraw_page_gui(
    self,
    page_name: str | None,
    doc_name: str | None,
    svg_path: str,
    png_path: str,
    width: int | None,
    height: int | None,
):
    try:
        doc = FreeCAD.getDocument(doc_name) if doc_name else FreeCAD.ActiveDocument
        if doc is None:
            return "No document is open or the named document was not found."
        page = _find_techdraw_page(doc, page_name)
        if page is None:
            if page_name:
                return f"TechDraw page '{page_name}' was not found in the document."
            return "The document contains no TechDraw pages."
        try:
            doc.recompute()
        except Exception:
            pass
        export_result = _export_techdraw_svg(page, svg_path)
        if export_result is not True:
            return export_result
        return _rasterize_svg_to_png(svg_path, png_path, width, height)
    except Exception as e:
        return str(e)
