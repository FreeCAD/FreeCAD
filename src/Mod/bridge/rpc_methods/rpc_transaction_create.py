from __future__ import annotations

from rpc import *


@rpc_method
def transaction_create(
    self, doc_name: str, label: str | None = None, reason: str | None = None
) -> dict[str, Any]:
    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return {"__error__": f"Document '{doc_name}' not found."}
            with _txn_lock:
                active_id = _active_txn_by_doc.get(doc_name)
                if (
                    active_id
                    and _transactions.get(active_id, {}).get("status") == "open"
                ):
                    return {
                        "__error__": (
                            f"Document '{doc_name}' already has an open "
                            f"transaction '{active_id}'. Apply or cancel it "
                            "before creating another."
                        )
                    }
                txn_id = _uuid7()
                try:
                    doc.UndoMode = 1
                except Exception:
                    pass
                doc.openTransaction(txn_id)
                record = {
                    "id": txn_id,
                    "document": doc.Name,
                    "label": label or "Transaction",
                    "reason": reason or "",
                    "status": "open",
                    "created_at": _now_iso(),
                    "applied_at": None,
                    "cancelled_at": None,
                    "operations": [],
                    "plan": None,
                    "_before": _snapshot_doc(doc),
                }
                _transactions[txn_id] = record
                _active_txn_by_doc[doc_name] = txn_id
                return {"__ok__": {"transaction": _txn_public(record)}}
        except Exception as e:
            return {"__error__": f"transaction_create error: {e}"}

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    return _txn_result(res)
