from __future__ import annotations

from rpc import *


@rpc_method
def _set_spreadsheet_dimensions_gui(
    self, doc_name, sheet_name, column_widths, row_heights, recompute
):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return f"Document '{doc_name}' not found."
    sheet, error = _resolve_spreadsheet(doc, sheet_name)
    if error:
        return error
    if not column_widths and not row_heights:
        return "Provide column_widths and/or row_heights to apply."
    try:
        columns = {}
        for column, width in (column_widths or {}).items():
            key = str(column).upper()
            sheet.setColumnWidth(key, int(width))
            columns[key] = int(width)
        rows = {}
        for row, height in (row_heights or {}).items():
            key = str(row)
            sheet.setRowHeight(key, int(height))
            rows[key] = int(height)
        if recompute:
            doc.recompute()
        return {"__ok__": True, "columns": columns, "rows": rows}
    except Exception as e:
        return str(e)
