from __future__ import annotations

from rpc import *


@rpc_method
def create_document(self, name="New_Document"):
    rpc_request_queue.put(lambda: self._create_document_gui(name))
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if res is True:
        return {"success": True, "document_name": name}
    return {"success": False, "error": res}
