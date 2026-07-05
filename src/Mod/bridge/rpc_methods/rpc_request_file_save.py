from __future__ import annotations

from rpc import *


@rpc_method
def request_file_save(
    self, doc_name: str | None = None, suggested_name: str | None = None
) -> dict[str, Any]:
    def deferred(response_slot):
        doc = (
            FreeCAD.getDocument(doc_name)
            if doc_name
            else getattr(FreeCAD, "ActiveDocument", None)
        )
        if doc is None:
            response_slot.put(
                {
                    "__error__": (
                        f"Document '{doc_name}' not found."
                        if doc_name
                        else "No active document."
                    )
                }
            )
            return
        current = getattr(doc, "FileName", "") or ""
        default_name = suggested_name or (
            os.path.splitext(os.path.basename(current))[0] if current else doc.Name
        )
        default_dir = os.path.dirname(current) if current else _default_autosave_dir()

        def on_done(result):
            if not result or result.get("cancelled"):
                response_slot.put(
                    {
                        "__ok__": {
                            "document": doc.Name,
                            "saved": False,
                            "cancelled": True,
                        }
                    }
                )
                return
            directory = result["directory"]
            filename = result["filename"]
            full_path = os.path.join(directory, f"{filename}.FCStd")
            try:
                os.makedirs(directory, exist_ok=True)
                doc.saveAs(full_path)
            except Exception as e:
                response_slot.put({"__error__": f"Save failed: {e}"})
                return
            response_slot.put(
                {
                    "__ok__": {
                        "document": doc.Name,
                        "saved": True,
                        "cancelled": False,
                        "path": full_path,
                        "directory": directory,
                        "filename": f"{filename}.FCStd",
                    }
                }
            )

        _show_file_save_dialog(doc.Name, default_name, default_dir, on_done)

    rpc_request_queue.put(_DeferredGuiTask(deferred))
    res = rpc_response_queue.get(timeout=600)
    return _txn_result(res)
