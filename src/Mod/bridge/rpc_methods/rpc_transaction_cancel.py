from __future__ import annotations

from rpc import *


@rpc_method
def transaction_cancel(self, transaction_id: str) -> dict[str, Any]:
    def task():
        try:
            record = _transactions.get(transaction_id)
            if record is None:
                return {"__error__": f"Transaction '{transaction_id}' not found."}
            if record["status"] == "cancelled":
                return {
                    "__error__": (
                        f"Transaction '{transaction_id}' is already cancelled."
                    )
                }
            doc = FreeCAD.getDocument(record["document"])
            if not doc:
                return {"__error__": f"Document '{record['document']}' not found."}
            if record["status"] == "open":
                doc.abortTransaction()
                doc.recompute()
            elif record["status"] == "applied":
                plan = record.get("plan") or {}
                has_changes = any(plan.get(k) for k in ("add", "change", "destroy"))
                if has_changes:
                    undo_names = list(getattr(doc, "UndoNames", []) or [])
                    if not undo_names or undo_names[0] != transaction_id:
                        return {
                            "__error__": (
                                f"Transaction '{transaction_id}' cannot be "
                                "rolled back because newer changes sit on top "
                                "of it in the undo stack. Cancel the newer "
                                "transactions first."
                            )
                        }
                    doc.undo()
                    doc.recompute()
            with _txn_lock:
                record["status"] = "cancelled"
                record["cancelled_at"] = _now_iso()
                if _active_txn_by_doc.get(record["document"]) == transaction_id:
                    _active_txn_by_doc.pop(record["document"], None)
            return {"__ok__": {"transaction": _txn_public(record)}}
        except Exception as e:
            return {"__error__": f"transaction_cancel error: {e}"}

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    return _txn_result(res)
