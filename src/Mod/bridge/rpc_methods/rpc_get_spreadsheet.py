from __future__ import annotations

from rpc import *


@rpc_method
def get_spreadsheet(self, doc_name, sheet_name, cell_range=None):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return {"success": False, "error": f"Document '{doc_name}' not found."}
    sheet, error = _resolve_spreadsheet(doc, sheet_name)
    if error:
        return {"success": False, "error": error}
    try:
        if cell_range:
            spans = cell_range if isinstance(cell_range, list) else [cell_range]
            addresses: list[str] = []
            for span in spans:
                addresses.extend(_expand_cell_range(span))
        else:
            used = _safe_sheet_call(sheet.getUsedCells) or ()
            addresses = sorted(
                (_normalize_cell(a) for a in used),
                key=lambda a: (
                    _split_cell(a)[1],
                    _column_to_index(_split_cell(a)[0]),
                ),
            )
    except ValueError as exc:
        return {"success": False, "error": str(exc)}
    cells = [_serialize_spreadsheet_cell(sheet, addr) for addr in addresses]
    return {
        "success": True,
        "sheet_name": sheet.Name,
        "label": sheet.Label,
        "cell_count": len(cells),
        "cells": cells,
    }
