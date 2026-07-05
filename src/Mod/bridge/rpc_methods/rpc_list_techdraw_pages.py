from __future__ import annotations

from rpc import *


@rpc_method
def list_techdraw_pages(self, doc_name: str | None = None) -> dict[str, Any]:
    rpc_request_queue.put(lambda: _list_techdraw_pages_gui(doc_name))
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        return res
    return {"success": False, "error": str(res)}
