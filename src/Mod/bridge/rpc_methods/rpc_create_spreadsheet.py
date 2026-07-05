from __future__ import annotations

from rpc import *


@rpc_method
def create_spreadsheet(self, doc_name, name="Spreadsheet", label=None):
    rpc_request_queue.put(lambda: self._create_spreadsheet_gui(doc_name, name, label))
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and res.get("__ok__"):
        return {
            "success": True,
            "sheet_name": res["name"],
            "label": res["label"],
        }
    return {"success": False, "error": str(res)}
