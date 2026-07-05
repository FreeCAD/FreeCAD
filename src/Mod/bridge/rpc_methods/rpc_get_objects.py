from __future__ import annotations

from rpc import *


@rpc_method
def get_objects(self, doc_name):
    doc = FreeCAD.getDocument(doc_name)
    if doc:
        return [serialize_object(obj) for obj in doc.Objects]
    return []
