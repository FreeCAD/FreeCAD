from __future__ import annotations

from rpc import *


@rpc_method
def transaction_delete(self, transaction_id: str) -> dict[str, Any]:
    with _txn_lock:
        record = _transactions.get(transaction_id)
        if record is None:
            return {
                "success": False,
                "error": f"Transaction '{transaction_id}' not found.",
            }
        if record["status"] != "cancelled":
            return {
                "success": False,
                "error": (
                    f"Transaction '{transaction_id}' is '{record['status']}'; "
                    "only cancelled transactions can be deleted. Cancel it first."
                ),
            }
        _transactions.pop(transaction_id, None)
        if _active_txn_by_doc.get(record["document"]) == transaction_id:
            _active_txn_by_doc.pop(record["document"], None)
        return {
            "success": True,
            "deleted": transaction_id,
            "document": record["document"],
        }
