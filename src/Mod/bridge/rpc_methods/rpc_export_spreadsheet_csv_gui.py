from __future__ import annotations

from rpc import *


@rpc_method
def _export_spreadsheet_csv_gui(self, doc_name, sheet_name, file_path, delimiter):
    doc = FreeCAD.getDocument(doc_name)
    if not doc:
        return f"Document '{doc_name}' not found."
    sheet, error = _resolve_spreadsheet(doc, sheet_name)
    if error:
        return error
    try:
        directory = os.path.dirname(os.path.abspath(file_path))
        if directory:
            os.makedirs(directory, exist_ok=True)
        sep = (delimiter or "\t")[:1]
        sheet.exportFile(file_path, sep)
        FreeCAD.Console.PrintMessage(
            f"Exported spreadsheet '{sheet.Name}' to '{file_path}'.\n"
        )
        return {"__ok__": True, "file": file_path}
    except Exception as e:
        return str(e)
