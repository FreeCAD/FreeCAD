from __future__ import annotations

from rpc import *


@rpc_method
def style_spreadsheet_cells(
    self,
    doc_name,
    sheet_name,
    targets,
    style=None,
    foreground=None,
    background=None,
    alignment=None,
    display_unit=None,
    style_options="replace",
    recompute=True,
):
    rpc_request_queue.put(
        lambda: self._style_spreadsheet_cells_gui(
            doc_name,
            sheet_name,
            targets,
            style,
            foreground,
            background,
            alignment,
            display_unit,
            style_options,
            recompute,
        )
    )
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and res.get("__ok__"):
        return {"success": True, "styled": res["styled"]}
    return {"success": False, "error": str(res)}
