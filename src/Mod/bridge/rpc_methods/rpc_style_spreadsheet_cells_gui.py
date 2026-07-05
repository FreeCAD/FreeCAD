from __future__ import annotations

from rpc import *


@rpc_method
def _style_spreadsheet_cells_gui(
    self,
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
):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return f"Document '{doc_name}' not found."
    sheet, error = _resolve_spreadsheet(doc, sheet_name)
    if error:
        return error
    spans = targets if isinstance(targets, list) else [targets]
    if not spans:
        return "Provide at least one cell or range to style."
    if (
        style is None
        and foreground is None
        and background is None
        and alignment is None
        and display_unit is None
    ):
        return (
            "Provide at least one of style, foreground, background, "
            "alignment, or display_unit."
        )
    options = style_options if style_options in _STYLE_OPTIONS else "replace"
    try:
        addresses: list[str] = []
        for span in spans:
            addresses.extend(_expand_cell_range(span))
        fg = _list_to_color(foreground) if foreground is not None else None
        bg = _list_to_color(background) if background is not None else None
        style_list = None
        if style is not None:
            raw_style = style if isinstance(style, list) else [style]
            style_list = [str(s) for s in raw_style]
        align_list = None
        if alignment is not None:
            raw_align = alignment if isinstance(alignment, list) else [alignment]
            align_list = [str(a) for a in raw_align]
        for address in addresses:
            if style_list is not None:
                sheet.setStyle(address, style_list, options)
            if fg is not None:
                sheet.setForeground(address, fg)
            if bg is not None:
                sheet.setBackground(address, bg)
            if align_list is not None:
                sheet.setAlignment(address, align_list, options)
            if display_unit is not None:
                sheet.setDisplayUnit(address, str(display_unit))
        if recompute:
            doc.recompute()
        return {"__ok__": True, "styled": addresses}
    except Exception as e:
        return str(e)
