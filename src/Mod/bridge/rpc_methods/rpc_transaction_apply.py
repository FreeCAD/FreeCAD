from __future__ import annotations

from rpc import *


@rpc_method
def transaction_apply(self, transaction_id: str) -> dict[str, Any]:
    def task():
        try:
            record = _transactions.get(transaction_id)
            if record is None:
                return {"__error__": f"Transaction '{transaction_id}' not found."}
            if record["status"] != "open":
                return {
                    "__error__": (
                        f"Transaction '{transaction_id}' is "
                        f"'{record['status']}'; only open transactions can be "
                        "applied."
                    )
                }
            doc = FreeCAD.getDocument(record["document"])
            if not doc:
                return {"__error__": f"Document '{record['document']}' not found."}
            try:
                doc.recompute()
            except Exception as e:
                try:
                    doc.abortTransaction()
                    doc.recompute()
                except Exception:
                    pass
                with _txn_lock:
                    record["status"] = "cancelled"
                    record["cancelled_at"] = _now_iso()
                    if _active_txn_by_doc.get(record["document"]) == transaction_id:
                        _active_txn_by_doc.pop(record["document"], None)
                return {
                    "__error__": (
                        f"Apply aborted: recompute failed ({e}). The change was "
                        "rolled back and the saved file was left untouched."
                    )
                }
            diff = _diff_snapshots(record["_before"], _snapshot_doc(doc))
            doc.commitTransaction()
            backup = _write_doc_backup(doc, transaction_id)
            autosave_entry = _autosave_document(doc)
            with _txn_lock:
                record["status"] = "applied"
                record["applied_at"] = _now_iso()
                record["plan"] = diff
                record["_backup"] = backup
                if _active_txn_by_doc.get(record["document"]) == transaction_id:
                    _active_txn_by_doc.pop(record["document"], None)
            return {
                "__ok__": {
                    "transaction": _txn_public(record),
                    "applied": diff,
                    "summary": _txn_summary_counts(diff),
                    "autosave": autosave_entry,
                    "checkpoint": bool(backup),
                }
            }
        except Exception as e:
            try:
                doc = FreeCAD.getDocument(record["document"]) if record else None
                if doc is not None:
                    doc.abortTransaction()
            except Exception:
                pass
            return {
                "__error__": (
                    f"transaction_apply error: {e}. The saved file was left "
                    "untouched."
                )
            }

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    return _txn_result(res)
