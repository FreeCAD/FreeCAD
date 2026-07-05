from __future__ import annotations

from rpc import *


@rpc_method
def _merge_spreadsheet_cells_gui(self, doc_name, sheet_name, ranges, action, recompute):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return f"Document '{doc_name}' not found."
    sheet, error = _resolve_spreadsheet(doc, sheet_name)
    if error:
        return error
    if action not in ("merge", "split"):
        return "action must be either 'merge' or 'split'."
    spans = ranges if isinstance(ranges, list) else [ranges]
    if not spans:
        return "Provide at least one cell range."
    try:
        applied = []
        for span in spans:
            text = str(span).strip()
            if action == "merge":
                sheet.mergeCells(text)
            else:
                sheet.splitCell(_expand_cell_range(text)[0])
            applied.append(text)
        if recompute:
            doc.recompute()
        return {"__ok__": True, "action": action, "ranges": applied}
    except Exception as e:
        return str(e)
