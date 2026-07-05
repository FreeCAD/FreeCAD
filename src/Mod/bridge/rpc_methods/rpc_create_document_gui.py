from __future__ import annotations

from rpc import *


@rpc_method
def _create_document_gui(self, name):
    doc = FreeCAD.newDocument(name)
    doc.recompute()
    FreeCAD.Console.PrintMessage(f"Document '{name}' created via RPC.\n")
    return True
