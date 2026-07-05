from __future__ import annotations

from rpc import *


@rpc_method
def create_object(self, doc_name, obj_data: dict[str, Any], transaction_id: str):
    try:
        with _txn_lock:
            _resolve_open_txn(transaction_id, doc_name)
    except ValueError as e:
        return {"success": False, "error": str(e)}
    obj = _Object(
        name=obj_data.get("Name", "New_Object"),
        type=obj_data["Type"],
        analysis=obj_data.get("Analysis", None),
        properties=obj_data.get("Properties", {}),
    )
    rpc_request_queue.put(lambda: self._create_object_gui(doc_name, obj))
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if res is True:
        _record_txn_operation(
            transaction_id,
            doc_name,
            "create_object",
            {"object": obj.name, "type": obj.type},
        )
        return {"success": True, "object_name": obj.name}
    return {"success": False, "error": res}
