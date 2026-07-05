from __future__ import annotations

from rpc import *


@rpc_method
def _set_spreadsheet_cells_gui(self, doc_name, sheet_name, cells, recompute):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return f"Document '{doc_name}' not found."
    sheet, error = _resolve_spreadsheet(doc, sheet_name)
    if error:
        return error
    if not isinstance(cells, dict) or not cells:
        return "cells must be a non-empty mapping of cell address to content."
    try:
        updated = []
        for address, content in cells.items():
            target = _normalize_cell(address)
            sheet.set(target, "" if content is None else str(content))
            updated.append(target)
        if recompute:
            doc.recompute()
        return {"__ok__": True, "updated": updated}
    except Exception as e:
        return str(e)
