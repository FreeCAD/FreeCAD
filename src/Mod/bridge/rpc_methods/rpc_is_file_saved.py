from __future__ import annotations

from rpc import *


@rpc_method
def is_file_saved(self, doc_name: str | None = None) -> dict[str, Any]:
    def task():
        doc = (
            FreeCAD.getDocument(doc_name)
            if doc_name
            else getattr(FreeCAD, "ActiveDocument", None)
        )
        if doc is None:
            return {
                "__error__": (
                    f"Document '{doc_name}' not found."
                    if doc_name
                    else "No active document."
                )
            }
        path = getattr(doc, "FileName", "") or ""
        saved = bool(path) and os.path.isfile(path)
        return {
            "__ok__": {
                "document": doc.Name,
                "saved": saved,
                "path": path or None,
            }
        }

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    return _txn_result(res)
