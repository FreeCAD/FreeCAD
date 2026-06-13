# SPDX-License-Identifier: LGPL-2.1-or-later

"""Arch/BIM view-provider resolution for headless-save restore.

The generic "missing view-provider proxy" restore contract is owned by
``draftutils.gui_utils.restore_view_object``. This module only contains the
Arch/BIM-specific convention for finding the matching view-provider
constructor from an object's data-proxy class.
"""

import importlib

import FreeCAD

from draftutils import gui_utils


def _get_view_provider_modules(module_name):
    """Yield the data-proxy module and optional ``*Gui`` companion module."""

    try:
        yield importlib.import_module(module_name)
    except ImportError:
        return

    try:
        yield importlib.import_module(module_name + "Gui")
    except ImportError:
        # Some BIM modules define the view provider next to the data proxy.
        return


def _get_view_provider_constructor(obj):
    """Return the constructor for a BIM object's view provider."""

    proxy = getattr(obj, "Proxy", None)
    if proxy is None:
        return None

    for cls in type(proxy).mro():
        if cls is object:
            continue

        module_name = getattr(cls, "__module__", None)
        class_name = getattr(cls, "__name__", None)
        if not module_name or not class_name:
            continue

        candidates = []
        if class_name.startswith("_"):
            stem = class_name[1:]
            candidates.extend((f"_ViewProvider{stem}", f"ViewProvider{stem}"))
        else:
            candidates.extend((f"ViewProvider{class_name}", f"_ViewProvider{class_name}"))

        if module_name.endswith("ArchReport") and class_name == "_ArchReport":
            candidates.append("ViewProviderReport")

        for module in _get_view_provider_modules(module_name):
            for candidate in candidates:
                constructor = getattr(module, candidate, None)
                if constructor is not None:
                    return constructor

    return None


def get_view_provider_proxy(vobj):
    """Return a real Python view-provider proxy, or ``None`` if it is missing."""

    return gui_utils.get_view_provider_proxy(vobj)


def restore_view_object(obj):
    """Restore a missing Arch/BIM Python view provider after a headless save."""

    if not FreeCAD.GuiUp:
        return

    constructor = _get_view_provider_constructor(obj)
    if constructor is None:
        return

    try:
        restored = gui_utils.restore_view_object(obj, format=False, vp_constructor=constructor)
        if restored and hasattr(obj.ViewObject, "Visibility"):
            # A headless-saved file has no GuiDocument.xml visibility state.
            obj.ViewObject.Visibility = True
    except Exception as exc:
        name = getattr(obj, "Name", "<unnamed>")
        FreeCAD.Console.PrintWarning(f"Arch: failed to restore the view object for {name}: {exc}\n")
