from __future__ import annotations

from rpc import *


@rpc_method
def _create_spreadsheet_gui(self, doc_name, name, label):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return f"Document '{doc_name}' not found."
    try:
        sheet = doc.addObject(_SPREADSHEET_TYPE_ID, name or "Spreadsheet")
        if label:
            sheet.Label = str(label)
        doc.recompute()
        FreeCAD.Console.PrintMessage(
            f"Spreadsheet '{sheet.Name}' created in '{doc_name}'.\n"
        )
        return {"__ok__": True, "name": sheet.Name, "label": sheet.Label}
    except Exception as e:
        return str(e)
