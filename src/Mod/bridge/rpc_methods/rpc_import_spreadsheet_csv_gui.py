from __future__ import annotations

from rpc import *


@rpc_method
def _import_spreadsheet_csv_gui(
    self, doc_name, sheet_name, file_path, delimiter, recompute
):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return f"Document '{doc_name}' not found."
    if not os.path.isfile(file_path):
        return f"File '{file_path}' does not exist."
    try:
        sheet = doc.getObject(sheet_name)
        if sheet is None:
            sheet = doc.addObject(_SPREADSHEET_TYPE_ID, sheet_name or "Spreadsheet")
        elif str(getattr(sheet, "TypeId", "")) != _SPREADSHEET_TYPE_ID:
            return f"Object '{sheet_name}' is not a {_SPREADSHEET_TYPE_ID}."
        sep = (delimiter or "\t")[:1]
        sheet.importFile(file_path, sep)
        if recompute:
            doc.recompute()
        FreeCAD.Console.PrintMessage(
            f"Imported '{file_path}' into spreadsheet '{sheet.Name}'.\n"
        )
        return {"__ok__": True, "name": sheet.Name, "file": file_path}
    except Exception as e:
        return str(e)
