# SPDX-License-Identifier: LGPL-2.1-or-later

"""Shared helpers for embedded FreeCAD init scripts."""

from pathlib import Path
import importlib
import sys
import types


DIR_MOD_NAMESPACE = "freecad._dir_mods"
DIR_MOD_APP_COMPAT_GLOBAL_NAMES = (
    "App",
    "FreeCAD",
    "Log",
    "Msg",
    "Err",
    "Wrn",
    "Crt",
    "Ntf",
    "Tnf",
    "IntEnum",
    "datetime",
    "Path",
    "collections",
    "coll_abc",
    "dataclasses",
    "functools",
    "importlib",
    "inspect",
    "os",
    "pkgutil",
    "platform",
    "re",
    "resources",
    "sys",
    "traceback",
    "types",
)
def _dir_mod_safe_segment(segment: str) -> str:
    value = "".join(char if (char.isalnum() or char == "_") else "_" for char in segment)
    if not value:
        return "_"
    if value[0].isdigit():
        return f"_{value}"
    return value


def dir_mod_package_name(mod_name: str) -> str:
    return f"{DIR_MOD_NAMESPACE}.{_dir_mod_safe_segment(mod_name)}"


def dir_mod_relative_parent(script_path, mod_path):
    script_path = Path(script_path)
    mod_root = Path(mod_path).absolute()

    script_parents = (
        script_path.absolute().parent,
        script_path.resolve().parent,
    )
    mod_roots = (
        mod_root,
        mod_root.resolve(),
    )
    for script_parent in script_parents:
        for candidate_mod_root in mod_roots:
            try:
                return script_parent.relative_to(candidate_mod_root)
            except ValueError:
                continue

    resolved_script_path = script_path.resolve()
    for ancestor in resolved_script_path.parents:
        if ancestor.name != mod_root.name:
            continue
        try:
            relative_parent = resolved_script_path.parent.relative_to(ancestor)
        except ValueError:
            continue

        build_candidate = mod_root / relative_parent / script_path.name
        try:
            if build_candidate.exists() and build_candidate.resolve() == resolved_script_path:
                return relative_parent
        except OSError:
            continue

    raise ValueError(
        f"{resolved_script_path.parent!s} is not within dir mod root {mod_root!s}"
    )


def dir_mod_module_name(mod_name: str, script_path, mod_path) -> str:
    script_path = Path(script_path).absolute()
    relative_parent = dir_mod_relative_parent(script_path, mod_path)
    parts = [dir_mod_package_name(mod_name)]
    if relative_parent != Path():
        parts.extend(_dir_mod_safe_segment(part) for part in relative_parent.parts)
    parts.append(_dir_mod_safe_segment(script_path.stem))
    return ".".join(parts)


def dir_mod_base_path(script_path, mod_path) -> Path:
    script_path = Path(script_path)
    mod_root = Path(mod_path).resolve()
    resolved_script_path = script_path.resolve()
    if resolved_script_path == mod_root or resolved_script_path.is_relative_to(mod_root):
        return mod_root

    relative_parent = dir_mod_relative_parent(script_path, mod_path)
    source_root = resolved_script_path.parent
    for _ in relative_parent.parts:
        source_root = source_root.parent
    return source_root.resolve()


def _bind_module_to_parent(module_name: str, module) -> None:
    if "." not in module_name:
        return

    parent_name, child_name = module_name.rsplit(".", 1)
    parent = sys.modules.get(parent_name)
    if parent is not None:
        setattr(parent, child_name, module)


def _ensure_package_module(package_name: str, path: str | None = None):
    package = sys.modules.get(package_name)
    if package is None:
        package = types.ModuleType(package_name)
        package.__package__ = package_name
        package.__path__ = []
        package.__spec__ = importlib.machinery.ModuleSpec(
            package_name,
            loader=None,
            is_package=True,
        )
        package.__spec__.submodule_search_locations = package.__path__
        sys.modules[package_name] = package
        _bind_module_to_parent(package_name, package)

    if path is not None and path not in package.__path__:
        package.__path__.append(path)

    return package


def _ensure_dir_mod_packages(mod_name: str, script_path, mod_path) -> None:
    mod_path = Path(mod_path).absolute()
    relative_parent = dir_mod_relative_parent(script_path, mod_path)

    freecad = importlib.import_module("freecad")
    if not hasattr(freecad, "__path__"):
        raise ImportError("'freecad' must be an importable package before loading dir mods")
    _ensure_package_module("freecad")
    _ensure_package_module(DIR_MOD_NAMESPACE)

    package_name = dir_mod_package_name(mod_name)
    current_path = mod_path
    _ensure_package_module(package_name, str(current_path))

    for part in relative_parent.parts:
        package_name = f"{package_name}.{_dir_mod_safe_segment(part)}"
        current_path = current_path / part
        _ensure_package_module(package_name, str(current_path))


def dir_mod_compat_globals(source_globals: dict, names: tuple[str, ...]) -> dict:
    compat = {"__builtins__": source_globals.get("__builtins__", __builtins__)}
    for name in names:
        if name in source_globals:
            compat[name] = source_globals[name]
    return compat


def exec_file_as_module(module_name: str, script_path, injected_globals: dict):
    script_path = Path(script_path).absolute()
    previous = sys.modules.get(module_name)
    missing = object()
    parent = None
    child_name = None
    previous_parent_child = missing
    if "." in module_name:
        parent_name, child_name = module_name.rsplit(".", 1)
        parent = sys.modules.get(parent_name)
        if parent is not None:
            previous_parent_child = getattr(parent, child_name, missing)
    spec = importlib.util.spec_from_file_location(module_name, script_path)
    if spec is None:
        raise ImportError(f"Failed to create import spec for {script_path!s}")

    module = importlib.util.module_from_spec(spec)
    module.__dict__.update(injected_globals)
    module.__dict__["__name__"] = module_name
    module.__dict__["__file__"] = str(script_path)
    module.__dict__["__package__"] = module_name.rsplit(".", 1)[0]
    module.__dict__["__loader__"] = spec.loader
    module.__dict__["__spec__"] = spec

    sys.modules[module_name] = module
    _bind_module_to_parent(module_name, module)

    try:
        source = script_path.read_text(encoding="utf-8")
        code = compile(source, script_path, "exec")
        exec(code, module.__dict__)
    except Exception:
        if previous is None:
            sys.modules.pop(module_name, None)
        else:
            sys.modules[module_name] = previous
        if parent is not None and child_name is not None:
            if previous_parent_child is missing:
                try:
                    delattr(parent, child_name)
                except AttributeError:
                    pass
            else:
                setattr(parent, child_name, previous_parent_child)
        raise

    return module


def exec_dir_mod_file(mod_name: str, script_path, mod_path, injected_globals: dict):
    _ensure_dir_mod_packages(mod_name, script_path, mod_path)
    module_name = dir_mod_module_name(mod_name, script_path, mod_path)
    return exec_file_as_module(module_name, script_path, injected_globals)
