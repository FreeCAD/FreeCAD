from __future__ import annotations

from rpc import *


@rpc_method
def transaction_list(self, doc_name: str | None = None) -> dict[str, Any]:
    with _txn_lock:
        records = [
            _txn_public(r)
            for r in _transactions.values()
            if doc_name is None or r["document"] == doc_name
        ]
    records.sort(key=lambda r: r["created_at"])
    return {"success": True, "transactions": records}
