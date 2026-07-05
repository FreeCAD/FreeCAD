from __future__ import annotations

from rpc import *


@rpc_method
def set_spreadsheet_aliases(self, doc_name, sheet_name, aliases, recompute=True):
    rpc_request_queue.put(
        lambda: self._set_spreadsheet_aliases_gui(
            doc_name, sheet_name, aliases, recompute
        )
    )
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and res.get("__ok__"):
        return {"success": True, "aliases": res["aliases"]}
    return {"success": False, "error": str(res)}
