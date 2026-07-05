from __future__ import annotations

from rpc import *


@rpc_method
def insert_part_from_library(self, relative_path, transaction_id: str):
    try:
        with _txn_lock:
            _resolve_open_txn(transaction_id)
    except ValueError as e:
        return {"success": False, "error": str(e)}
    rpc_request_queue.put(lambda: self._insert_part_from_library_gui(relative_path))
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if res is True:
        record = _transactions.get(transaction_id)
        doc_name = record["document"] if record else None
        _record_txn_operation(
            transaction_id,
            doc_name,
            "insert_part_from_library",
            {"part": relative_path},
        )
        return {"success": True, "message": "Part inserted from library."}
    return {"success": False, "error": res}
