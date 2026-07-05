from __future__ import annotations

from rpc import *


@rpc_method
def get_object(self, doc_name, obj_name):
    doc = FreeCAD.getDocument(doc_name)
    if doc:
        obj = doc.getObject(obj_name)
        return serialize_object(obj) if obj else None
    return None
