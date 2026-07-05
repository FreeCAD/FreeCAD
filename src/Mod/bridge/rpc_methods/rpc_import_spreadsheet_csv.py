from __future__ import annotations

from rpc import *


@rpc_method
def import_spreadsheet_csv(
    self, doc_name, sheet_name, file_path, delimiter="\t", recompute=True
):
    rpc_request_queue.put(
        lambda: self._import_spreadsheet_csv_gui(
            doc_name, sheet_name, file_path, delimiter, recompute
        )
    )
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and res.get("__ok__"):
        return {"success": True, "sheet_name": res["name"], "file": res["file"]}
    return {"success": False, "error": str(res)}
