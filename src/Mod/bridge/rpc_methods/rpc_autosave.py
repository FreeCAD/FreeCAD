from __future__ import annotations

from rpc import *


@rpc_method
def autosave(
    self, doc_name: str | None = None, fallback_dir: str | None = None
) -> dict[str, Any]:
    def task():
        try:
            if doc_name is not None:
                target = FreeCAD.getDocument(doc_name)
                if target is None:
                    return {"__error__": f"Document '{doc_name}' not found."}
                docs = [target]
            else:
                docs = list(FreeCAD.listDocuments().values())

            results = [_autosave_document(d, fallback_dir) for d in docs]
            FreeCAD.Console.PrintMessage(
                f"Autosave processed {len(results)} document(s).\n"
            )
            return {"__results__": results}
        except Exception as e:
            FreeCAD.Console.PrintError(f"Error during autosave: {e}\n")
            return {"__error__": f"Error during autosave: {e}"}

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict) and "__results__" in res:
        return {"success": True, "results": res["__results__"]}
    if isinstance(res, dict) and "__error__" in res:
        return {"success": False, "error": res["__error__"]}
    return {"success": False, "error": str(res)}
