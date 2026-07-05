from __future__ import annotations

from rpc import *


@rpc_method
def edit_object(
    self,
    doc_name: str,
    obj_name: str,
    properties: dict[str, Any],
    transaction_id: str,
) -> dict[str, Any]:
    try:
        with _txn_lock:
            _resolve_open_txn(transaction_id, doc_name)
    except ValueError as e:
        return {"success": False, "error": str(e)}
    obj = _Object(name=obj_name, properties=properties.get("Properties", {}))
    rpc_request_queue.put(lambda: self._edit_object_gui(doc_name, obj))
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if res is True:
        _record_txn_operation(
            transaction_id,
            doc_name,
            "edit_object",
            {"object": obj.name, "properties": sorted(obj.properties.keys())},
        )
        return {"success": True, "object_name": obj.name}
    return {"success": False, "error": res}
