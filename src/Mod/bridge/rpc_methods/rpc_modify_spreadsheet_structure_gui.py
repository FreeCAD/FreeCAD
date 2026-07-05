from __future__ import annotations

from rpc import *


@rpc_method
def _modify_spreadsheet_structure_gui(
    self, doc_name, sheet_name, operation, index, count, recompute
):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return f"Document '{doc_name}' not found."
    sheet, error = _resolve_spreadsheet(doc, sheet_name)
    if error:
        return error
    if operation not in _STRUCTURE_OPERATIONS:
        return f"operation must be one of {', '.join(_STRUCTURE_OPERATIONS)}."
    try:
        amount = int(count)
        if amount < 1:
            return "count must be a positive integer."
        if operation in ("insert_columns", "remove_columns"):
            key = str(index).upper()
            if operation == "insert_columns":
                sheet.insertColumns(key, amount)
            else:
                sheet.removeColumns(key, amount)
        else:
            key = str(index)
            if operation == "insert_rows":
                sheet.insertRows(key, amount)
            else:
                sheet.removeRows(key, amount)
        if recompute:
            doc.recompute()
        return {"__ok__": True, "index": key, "count": amount}
    except Exception as e:
        return str(e)
