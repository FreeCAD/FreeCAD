from __future__ import annotations

from rpc import *


@rpc_method
def list_documents(self):
    return list(FreeCAD.listDocuments().keys())
