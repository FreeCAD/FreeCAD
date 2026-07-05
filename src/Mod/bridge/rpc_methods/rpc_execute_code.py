from __future__ import annotations

from rpc import *


@rpc_method
def execute_code(self, code: str, transaction_id: str) -> dict[str, Any]:
    try:
        with _txn_lock:
            _resolve_open_txn(transaction_id)
    except ValueError as e:
        return {"success": False, "error": str(e)}
    output_buffer = io.StringIO()

    def task():
        _engage_viewport_lock()
        try:
            with contextlib.redirect_stdout(output_buffer):
                exec(code, globals())
            FreeCAD.Console.PrintMessage("Python code executed successfully.\n")
            return True
        except Exception:
            captured = output_buffer.getvalue()
            detail = "Error executing Python code:\n" + traceback.format_exc()
            if captured:
                detail += "\nCaptured output before error:\n" + captured
            return detail
        finally:
            _release_viewport_lock()

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if res is True:
        record = _transactions.get(transaction_id)
        doc_name = record["document"] if record else None
        _record_txn_operation(
            transaction_id,
            doc_name,
            "execute_code",
            {"bytes": len(code.encode("utf-8"))},
        )
        return {
            "success": True,
            "message": "Python code executed.\nOutput:\n" + output_buffer.getvalue(),
        }
    return {"success": False, "error": res}
