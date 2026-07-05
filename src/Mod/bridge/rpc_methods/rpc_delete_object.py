from __future__ import annotations

from rpc import *


@rpc_method
def delete_object(self, doc_name: str, obj_name: str, transaction_id: str):
    try:
        with _txn_lock:
            _resolve_open_txn(transaction_id, doc_name)
    except ValueError as e:
        return {"success": False, "error": str(e)}
    rpc_request_queue.put(lambda: self._delete_object_gui(doc_name, obj_name))
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if res is True:
        _record_txn_operation(
            transaction_id, doc_name, "delete_object", {"object": obj_name}
        )
        return {"success": True, "object_name": obj_name}
    return {"success": False, "error": res}
