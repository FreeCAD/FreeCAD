from __future__ import annotations

from rpc import *


@rpc_method
def _clear_spreadsheet_cells_gui(self, doc_name, sheet_name, ranges, recompute):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return f"Document '{doc_name}' not found."
    sheet, error = _resolve_spreadsheet(doc, sheet_name)
    if error:
        return error
    targets = ranges if isinstance(ranges, list) else [ranges]
    if not targets:
        return "Provide at least one cell or range to clear."
    try:
        cleared = []
        for span in targets:
            for address in _expand_cell_range(span):
                sheet.clear(address)
                cleared.append(address)
        if recompute:
            doc.recompute()
        return {"__ok__": True, "cleared": cleared}
    except Exception as e:
        return str(e)
