from __future__ import annotations

from rpc import *


@rpc_method
def get_selection(self):
    try:
        selections = FreeCADGui.Selection.getSelectionEx("*")
    except Exception:
        try:
            selections = FreeCADGui.Selection.getSelectionEx()
        except Exception as e:
            return {
                "success": False,
                "error": f"get_selection error: {e}",
                "selection": [],
                "count": 0,
            }
    result = []
    for sel in selections or []:
        try:
            obj = getattr(sel, "Object", None)
            name = getattr(sel, "ObjectName", "") or (
                getattr(obj, "Name", "") if obj else ""
            )
            label = getattr(obj, "Label", name) if obj else name
            sub_elements = [s for s in (getattr(sel, "SubElementNames", ()) or ()) if s]
            result.append(
                {
                    "Document": getattr(sel, "DocumentName", "") or "",
                    "Name": name,
                    "Label": label,
                    "TypeId": getattr(obj, "TypeId", "") if obj else "",
                    "SubElements": sub_elements,
                }
            )
        except Exception:
            continue
    return {"success": True, "selection": result, "count": len(result)}
