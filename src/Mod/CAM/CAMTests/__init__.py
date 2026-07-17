# SPDX-License-Identifier: LGPL-2.1-or-later

import importlib
import inspect
import pkgutil
import unittest

_EXCLUDE_MODULES = {
    "TestLinuxCNCLegacyPost",
    "TestGrblLegacyPost",
    "TestPathToolBitBrowserWidget",
    "TestPathToolBitEditorWidget",
    "TestPathToolBitListWidget",
    "TestPathToolBitPropertyEditorWidget",
    "TestPathToolDocumentObjectEditorWidget",
}

_EXCLUDE_MEMBERS = {
    "TestMachine": ["TestOutputOptions"],
    "TestPathToolAssetStore": ["BaseTestPathToolAssetStore"],
    "TestPathToolBitSerializer": ["_BaseToolBitSerializerTestCase"],
    "TestPostToolProcessing": ["TestEmptyMoveSuppression"],
}


__all__ = []
for _, module_name, _ in pkgutil.iter_modules(__path__):
    if module_name in _EXCLUDE_MODULES:
        continue
    module = importlib.import_module(f"{__name__}.{module_name}")
    for name, obj in inspect.getmembers(module, inspect.isclass):
        if name in _EXCLUDE_MEMBERS.get(module_name, []):
            continue
        if issubclass(obj, unittest.TestCase) and obj.__module__ == module.__name__:
            globals()[name] = obj
            __all__.append(name)
