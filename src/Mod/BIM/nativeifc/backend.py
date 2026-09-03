# SPDX-License-Identifier: LGPL-2.1-or-later

"""IfcOpenShell loading and capability detection for FreeCAD BIM.

This module intentionally has no FreeCAD dependency. It is the runtime
boundary between BIM code and the native IfcOpenShell Python package and can
therefore be exercised in isolation, including when IfcOpenShell is absent.
"""

from dataclasses import dataclass
import importlib
from types import ModuleType

IMPORT_MODULES = (
    "ifcopenshell",
    "ifcopenshell.geom",
)

EXPLORER_MODULES = (*IMPORT_MODULES, "ifcopenshell.entity_instance")
READ_MODULES = ("ifcopenshell",)

REQUIRED_MODULES = (
    "ifcopenshell",
    "ifcopenshell.api",
    "ifcopenshell.api.aggregate",
    "ifcopenshell.api.feature",
    "ifcopenshell.api.geometry",
    "ifcopenshell.api.group",
    "ifcopenshell.api.material",
    "ifcopenshell.api.pset",
    "ifcopenshell.api.spatial",
    "ifcopenshell.api.style",
    "ifcopenshell.api.type",
    "ifcopenshell.geom",
    "ifcopenshell.entity_instance",
    "ifcopenshell.util.attribute",
    "ifcopenshell.util.element",
    "ifcopenshell.util.placement",
    "ifcopenshell.util.schema",
    "ifcopenshell.util.unit",
)

READ = "read"
IMPORT = "import"
MULTICORE_IMPORT = "multicore_import"
EXPLORER = "explorer"
EXPORT = "export"
_CAPABILITIES = (READ, IMPORT, MULTICORE_IMPORT, EXPLORER, EXPORT)


class IfcOpenShellUnavailable(ImportError):
    """Raised when FreeCAD's required IfcOpenShell surface cannot be loaded."""


@dataclass(frozen=True)
class BackendStatus:
    """Result of validating the installed IfcOpenShell backend."""

    available: bool
    version: str = ""
    geometry_iterator: bool = False
    serialized_brep: bool = False
    schema_names: bool = False
    error: str = ""


_modules: dict[str, dict[str, ModuleType]] = {}
_statuses: dict[str, BackendStatus] = {}


def _version(module: ModuleType) -> str:
    """Return a normalized display version across IfcOpenShell releases."""

    return str(getattr(module, "version", getattr(module, "__version__", "")))


def _load(capability: str) -> tuple[dict[str, ModuleType], BackendStatus]:
    if capability not in _CAPABILITIES:
        raise ValueError(f"unknown IfcOpenShell capability: {capability}")

    if capability == EXPORT:
        required_modules = REQUIRED_MODULES
    elif capability == EXPLORER:
        required_modules = EXPLORER_MODULES
    elif capability == READ:
        required_modules = READ_MODULES
    else:
        required_modules = IMPORT_MODULES
    modules = {name: importlib.import_module(name) for name in required_modules}
    module = modules["ifcopenshell"]
    geom = modules.get("ifcopenshell.geom")
    wrapper = getattr(module, "ifcopenshell_wrapper", None)
    legacy_settings = getattr(geom, "settings", lambda: None)() if geom else None
    serialized_brep = geom is not None and (
        (wrapper is not None and hasattr(wrapper, "SERIALIZED"))
        or hasattr(legacy_settings, "USE_BREP_DATA")
    )

    required_callables = {
        "ifcopenshell.open": getattr(module, "open", None),
    }
    if capability in (IMPORT, MULTICORE_IMPORT, EXPLORER):
        required_callables.update(
            {
                "ifcopenshell.geom.settings": getattr(geom, "settings", None),
            }
        )
        if capability in (IMPORT, EXPLORER):
            required_callables["ifcopenshell.geom.create_shape"] = getattr(
                geom, "create_shape", None
            )
        if capability == MULTICORE_IMPORT:
            required_callables["ifcopenshell.geom.iterator"] = getattr(geom, "iterator", None)
        if capability == EXPLORER:
            entity_module = modules["ifcopenshell.entity_instance"]
            if not isinstance(getattr(entity_module, "entity_instance", None), type):
                raise IfcOpenShellUnavailable(
                    "missing required API: ifcopenshell.entity_instance.entity_instance"
                )
    elif capability == EXPORT:
        api = modules["ifcopenshell.api"]
        api_aggregate = modules["ifcopenshell.api.aggregate"]
        api_feature = modules["ifcopenshell.api.feature"]
        api_geometry = modules["ifcopenshell.api.geometry"]
        api_group = modules["ifcopenshell.api.group"]
        api_material = modules["ifcopenshell.api.material"]
        api_pset = modules["ifcopenshell.api.pset"]
        api_spatial = modules["ifcopenshell.api.spatial"]
        api_style = modules["ifcopenshell.api.style"]
        api_type = modules["ifcopenshell.api.type"]
        required_callables.update(
            {
                "ifcopenshell.file": getattr(module, "file", None),
                "ifcopenshell.api.run": getattr(api, "run", None),
                "ifcopenshell.api.aggregate.assign_object": getattr(
                    api_aggregate, "assign_object", None
                ),
                "ifcopenshell.api.feature.add_feature": getattr(api_feature, "add_feature", None),
                "ifcopenshell.api.feature.add_filling": getattr(api_feature, "add_filling", None),
                "ifcopenshell.api.geometry.add_wall_representation": getattr(
                    api_geometry, "add_wall_representation", None
                ),
                "ifcopenshell.api.geometry.add_profile_representation": getattr(
                    api_geometry, "add_profile_representation", None
                ),
                "ifcopenshell.api.group.add_group": getattr(api_group, "add_group", None),
                "ifcopenshell.api.group.assign_group": getattr(api_group, "assign_group", None),
                "ifcopenshell.api.material.add_material": getattr(
                    api_material, "add_material", None
                ),
                "ifcopenshell.api.material.add_material_set": getattr(
                    api_material, "add_material_set", None
                ),
                "ifcopenshell.api.material.add_layer": getattr(api_material, "add_layer", None),
                "ifcopenshell.api.material.edit_layer": getattr(api_material, "edit_layer", None),
                "ifcopenshell.api.material.add_profile": getattr(api_material, "add_profile", None),
                "ifcopenshell.api.material.assign_material": getattr(
                    api_material, "assign_material", None
                ),
                "ifcopenshell.api.pset.add_pset": getattr(api_pset, "add_pset", None),
                "ifcopenshell.api.pset.edit_pset": getattr(api_pset, "edit_pset", None),
                "ifcopenshell.api.pset.add_qto": getattr(api_pset, "add_qto", None),
                "ifcopenshell.api.pset.edit_qto": getattr(api_pset, "edit_qto", None),
                "ifcopenshell.api.spatial.assign_container": getattr(
                    api_spatial, "assign_container", None
                ),
                "ifcopenshell.api.style.add_style": getattr(api_style, "add_style", None),
                "ifcopenshell.api.style.add_surface_style": getattr(
                    api_style, "add_surface_style", None
                ),
                "ifcopenshell.api.style.assign_item_style": getattr(
                    api_style, "assign_item_style", None
                ),
                "ifcopenshell.api.type.assign_type": getattr(api_type, "assign_type", None),
                "ifcopenshell.geom.iterator": getattr(geom, "iterator", None),
            }
        )
    missing = [name for name, value in required_callables.items() if not callable(value)]
    if capability in (IMPORT, MULTICORE_IMPORT) and not serialized_brep:
        missing.append("serialized geometry output")
    if missing:
        raise IfcOpenShellUnavailable("missing required API: " + ", ".join(missing))

    status = BackendStatus(
        available=True,
        version=_version(module),
        geometry_iterator=callable(getattr(geom, "iterator", None)) if geom else False,
        serialized_brep=serialized_brep,
        schema_names=wrapper is not None and callable(getattr(wrapper, "schema_names", None)),
    )
    return modules, status


def get_status(refresh: bool = False, capability: str = EXPORT) -> BackendStatus:
    """Validate and describe the backend without raising import errors."""

    if refresh:
        invalidate()
    if capability not in _statuses:
        try:
            _modules[capability], _statuses[capability] = _load(capability)
        except Exception as exc:
            _modules.pop(capability, None)
            _statuses[capability] = BackendStatus(
                available=False,
                error=f"{type(exc).__name__}: {exc}",
            )
    return _statuses[capability]


def get_backend(refresh: bool = False, capability: str = EXPORT) -> ModuleType:
    """Return a validated IfcOpenShell module or raise a stable exception."""

    status = get_status(refresh=refresh, capability=capability)
    modules = _modules.get(capability)
    if not status.available or modules is None:
        detail = f" ({status.error})" if status.error else ""
        raise IfcOpenShellUnavailable(f"IfcOpenShell is unavailable{detail}")
    return modules["ifcopenshell"]


def get_module(name: str, capability: str = EXPORT) -> ModuleType:
    """Return a validated submodule belonging to a capability profile."""

    get_backend(capability=capability)
    try:
        return _modules[capability][name]
    except KeyError as exc:
        raise ValueError(f"module {name!r} is not part of the {capability!r} capability") from exc


def create_geometry_settings(
    capability: str,
    *,
    disable_opening_subtractions: bool = False,
    apply_layersets: bool = False,
):
    """Create serialized BRep settings across old and current IfcOpenShell APIs."""

    module = get_backend(capability=capability)
    geom = get_module("ifcopenshell.geom", capability=capability)
    settings = geom.settings()

    if hasattr(settings, "USE_BREP_DATA"):
        settings.set(settings.USE_BREP_DATA, True)
        if hasattr(settings, "SEW_SHELLS"):
            settings.set(settings.SEW_SHELLS, True)
    else:
        settings.set("iterator-output", module.ifcopenshell_wrapper.SERIALIZED)

    _set_geometry_setting(settings, "use-world-coords", True, "USE_WORLD_COORDS")
    if disable_opening_subtractions:
        _set_geometry_setting(
            settings,
            "disable-opening-subtractions",
            True,
            "DISABLE_OPENING_SUBTRACTIONS",
        )
    if apply_layersets:
        if hasattr(settings, "APPLY_LAYERSETS"):
            settings.set(settings.APPLY_LAYERSETS, True)
        elif "enable-layerset-slicing" in settings.setting_names():
            settings.set("enable-layerset-slicing", True)
    return settings


def create_mesh_settings():
    """Create world-coordinate triangulation settings for IFC previews."""

    geom = get_module("ifcopenshell.geom", capability=EXPLORER)
    settings = geom.settings()
    _set_geometry_setting(settings, "use-world-coords", True, "USE_WORLD_COORDS")
    return settings


def _set_geometry_setting(settings, name, value, legacy_name):
    if hasattr(settings, legacy_name):
        settings.set(getattr(settings, legacy_name), value)
    else:
        settings.set(name, value)


def invalidate() -> None:
    """Forget cached validation after the Python package environment changes."""

    importlib.invalidate_caches()
    _modules.clear()
    _statuses.clear()
