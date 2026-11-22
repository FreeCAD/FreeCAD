# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

from FreeCAD import Base
import importlib

def OpenInsertObject(importerName, objectPath, importMethod, docName = ""):
    try:
        importArgs = []
        importKwargs = {}

        importerModule = importlib.import_module(importerName)

        if docName:
            importArgs.append(docName)
        if hasattr(importerModule, "importOptions"):
            importKwargs["options"] = importerModule.importOptions(objectPath)

        getattr(importerModule, importMethod)(objectPath, *importArgs, **importKwargs)
    except Base.AbortIOException:
        pass
