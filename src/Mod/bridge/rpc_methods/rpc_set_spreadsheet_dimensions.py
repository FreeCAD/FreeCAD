from __future__ import annotations

from rpc import *


@rpc_method
def set_spreadsheet_dimensions(
    self,
    doc_name,
    sheet_name,
    column_widths=None,
    row_heights=None,
    recompute=True,
):
    rpc_request_queue.put(
        lambda: self._set_spreadsheet_dimensions_gui(
            doc_name, sheet_name, column_widths, row_heights, recompute
        )
    )
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and res.get("__ok__"):
        return {
            "success": True,
            "columns": res["columns"],
            "rows": res["rows"],
        }
    return {"success": False, "error": str(res)}
