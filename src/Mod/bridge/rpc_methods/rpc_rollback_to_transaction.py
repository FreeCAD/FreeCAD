from __future__ import annotations

from rpc import *


@rpc_method
def rollback_to_transaction(self, transaction_id: str) -> dict[str, Any]:
    def task():
        record = _transactions.get(transaction_id)
        if record is None:
            return {"__error__": f"Transaction '{transaction_id}' not found."}
        backup = record.get("_backup")
        if not backup:
            return {
                "__error__": (
                    f"Transaction '{transaction_id}' has no checkpoint; only "
                    "applied transactions can be rolled back to."
                )
            }
        doc_name = record["document"]
        try:
            active_id = _active_txn_by_doc.get(doc_name)
            if active_id:
                open_doc = FreeCAD.getDocument(doc_name)
                if open_doc is not None:
                    try:
                        open_doc.abortTransaction()
                    except Exception:
                        pass
            restored = _restore_doc_from_backup(doc_name, backup)
            target_created = record["created_at"]
            superseded = []
            with _txn_lock:
                for tid, r in _transactions.items():
                    if r["document"] != doc_name or tid == transaction_id:
                        continue
                    if r["status"] == "cancelled":
                        continue
                    if r["created_at"] > target_created or tid == active_id:
                        r["status"] = "cancelled"
                        r["cancelled_at"] = _now_iso()
                        superseded.append(tid)
                record["status"] = "applied"
                _active_txn_by_doc.pop(doc_name, None)
            autosave_entry = _autosave_document(restored)
            return {
                "__ok__": {
                    "restored_to": transaction_id,
                    "document": doc_name,
                    "superseded": superseded,
                    "transaction": _txn_public(record),
                    "autosave": autosave_entry,
                }
            }
        except Exception as e:
            return {
                "__error__": (
                    f"rollback_to_transaction error: {e}. The saved file was "
                    "left untouched."
                )
            }

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    return _txn_result(res)
