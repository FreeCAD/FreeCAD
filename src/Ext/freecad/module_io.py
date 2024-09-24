def OpenInsertObject(importerModule, objectPath, importMethod, docName = ""):
    try:
        importArgs = []
        importKwargs = {}

        if docName:
            importArgs.append(docName)
        if hasattr(importerModule, "importOptions"):
            importKwargs["options"] = importerModule.importOptions(objectPath)

        getattr(importerModule, importMethod)(objectPath, *importArgs, **importKwargs)
    except PyExc_FC_AbortIOException:
        pass
