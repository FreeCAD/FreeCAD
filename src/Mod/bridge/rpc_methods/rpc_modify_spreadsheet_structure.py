from __future__ import annotations

from rpc import *


@rpc_method
def modify_spreadsheet_structure(
    self, doc_name, sheet_name, operation, index, count=1, recompute=True
):
    rpc_request_queue.put(
        lambda: self._modify_spreadsheet_structure_gui(
            doc_name, sheet_name, operation, index, count, recompute
        )
    )
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and res.get("__ok__"):
        return {
            "success": True,
            "operation": operation,
            "index": res["index"],
            "count": res["count"],
        }
    return {"success": False, "error": str(res)}
