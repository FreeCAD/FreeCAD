from __future__ import annotations

from rpc import *


@rpc_method
def transaction_plan(self, transaction_id: str) -> dict[str, Any]:
    def task():
        try:
            record = _transactions.get(transaction_id)
            if record is None:
                return {"__error__": f"Transaction '{transaction_id}' not found."}
            doc = FreeCAD.getDocument(record["document"])
            if not doc:
                return {"__error__": f"Document '{record['document']}' not found."}
            if record["status"] == "open":
                diff = _diff_snapshots(record["_before"], _snapshot_doc(doc))
                record["plan"] = diff
            else:
                diff = record.get("plan") or {
                    "add": [],
                    "change": [],
                    "destroy": [],
                }
            return {
                "__ok__": {
                    "transaction": _txn_public(record),
                    "plan": diff,
                    "summary": _txn_summary_counts(diff),
                }
            }
        except Exception as e:
            return {"__error__": f"transaction_plan error: {e}"}

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    return _txn_result(res)
