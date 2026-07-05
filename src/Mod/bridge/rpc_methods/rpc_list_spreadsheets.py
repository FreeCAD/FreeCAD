from __future__ import annotations

from rpc import *


@rpc_method
def list_spreadsheets(self, doc_name):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return {"success": False, "error": f"Document '{doc_name}' not found."}
    sheets = []
    for obj in doc.Objects:
        if str(getattr(obj, "TypeId", "")) == _SPREADSHEET_TYPE_ID:
            used = _safe_sheet_call(obj.getUsedCells) or ()
            sheets.append(
                {
                    "name": obj.Name,
                    "label": obj.Label,
                    "used_cell_count": len(used),
                }
            )
    return {"success": True, "spreadsheets": sheets}
