from __future__ import annotations

from rpc import *


@rpc_method
def request_code_approval(
    self, code: str, tag: str = "", reason: str = "", expected_action: str = ""
) -> dict[str, Any]:
    if _approval_state["auto_approve_session"]:
        return {"approved": True, "auto": True}

    def task():
        try:
            decision = _show_code_approval_dialog(code, tag, reason, expected_action)
            return {"decision": decision}
        except Exception as e:
            return f"Approval dialog error: {e}"

    rpc_request_queue.put(task)
    try:
        res = rpc_response_queue.get(timeout=600)
    except queue.Empty:
        return {"approved": False, "auto": False, "error": "approval timed out"}
    if isinstance(res, dict):
        decision = res.get("decision", "deny")
        if decision == "auto":
            _approval_state["auto_approve_session"] = True
            return {"approved": True, "auto": True}
        return {"approved": decision == "approve", "auto": False}
    return {"approved": False, "auto": False, "error": str(res)}
