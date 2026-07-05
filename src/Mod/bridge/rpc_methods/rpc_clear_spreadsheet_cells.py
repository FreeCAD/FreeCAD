from __future__ import annotations

from rpc import *


@rpc_method
def clear_spreadsheet_cells(self, doc_name, sheet_name, ranges, recompute=True):
    rpc_request_queue.put(
        lambda: self._clear_spreadsheet_cells_gui(
            doc_name, sheet_name, ranges, recompute
        )
    )
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and res.get("__ok__"):
        return {"success": True, "cleared": res["cleared"]}
    return {"success": False, "error": str(res)}
