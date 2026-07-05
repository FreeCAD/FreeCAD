from __future__ import annotations

from rpc import *


@rpc_method
def transaction_get(self, transaction_id: str) -> dict[str, Any]:
    with _txn_lock:
        record = _transactions.get(transaction_id)
        if record is None:
            return {
                "success": False,
                "error": f"Transaction '{transaction_id}' not found.",
            }
        return {"success": True, "transaction": _txn_public(record)}
