from __future__ import annotations

from rpc import *


@rpc_method
def merge_spreadsheet_cells(
    self, doc_name, sheet_name, ranges, action="merge", recompute=True
):
    rpc_request_queue.put(
        lambda: self._merge_spreadsheet_cells_gui(
            doc_name, sheet_name, ranges, action, recompute
        )
    )
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and res.get("__ok__"):
        return {
            "success": True,
            "action": res["action"],
            "ranges": res["ranges"],
        }
    return {"success": False, "error": str(res)}
