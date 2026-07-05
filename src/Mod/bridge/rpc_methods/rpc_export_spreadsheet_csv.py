from __future__ import annotations

from rpc import *


@rpc_method
def export_spreadsheet_csv(self, doc_name, sheet_name, file_path, delimiter="\t"):
    rpc_request_queue.put(
        lambda: self._export_spreadsheet_csv_gui(
            doc_name, sheet_name, file_path, delimiter
        )
    )
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and res.get("__ok__"):
        return {"success": True, "file": res["file"]}
    return {"success": False, "error": str(res)}
