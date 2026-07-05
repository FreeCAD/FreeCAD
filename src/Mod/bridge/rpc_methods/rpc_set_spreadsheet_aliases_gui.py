from __future__ import annotations

from rpc import *


@rpc_method
def _set_spreadsheet_aliases_gui(self, doc_name, sheet_name, aliases, recompute):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return f"Document '{doc_name}' not found."
    sheet, error = _resolve_spreadsheet(doc, sheet_name)
    if error:
        return error
    if not isinstance(aliases, dict) or not aliases:
        return "aliases must be a non-empty mapping of cell address to alias."
    try:
        applied = {}
        for address, alias in aliases.items():
            target = _normalize_cell(address)
            resolved = "" if alias is None else str(alias)
            sheet.setAlias(target, resolved)
            applied[target] = resolved
        if recompute:
            doc.recompute()
        return {"__ok__": True, "aliases": applied}
    except Exception as e:
        return str(e)
