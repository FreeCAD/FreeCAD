# SPDX-License-Identifier: LGPL-2.1-or-later
from __future__ import annotations

import importlib
import importlib.util
import os
import pkgutil
import sys
import traceback
from pathlib import Path


ENTRYPOINT_WRAPPERS = {"Init", "InitGui", "__init__"}
ENVIRONMENT_DEPENDENCIES = ("FreeCAD", "FreeCADGui")
SUBDIRECTORIES = ("tools", "rpc_methods")


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def mod_root() -> Path:
    override = os.environ.get("PARASHELL_MOD_ROOT", "").strip()
    if override:
        return Path(override).resolve()
    return repo_root() / "src" / "Mod"


def is_parashell_module(module_dir: Path) -> bool:
    if not module_dir.is_dir():
        return False
    if not (module_dir / "Init.py").is_file():
        return False
    for entry in module_dir.iterdir():
        if entry.is_file() and entry.name.split(".", 1)[0] == "hostcontrol":
            return True
    return False


def discover_parashell_modules(root: Path) -> list[Path]:
    return sorted(
        (child for child in root.iterdir() if is_parashell_module(child)),
        key=lambda path: path.name,
    )


def importable_names(directory: Path) -> list[str]:
    if not directory.is_dir():
        return []
    names = []
    for info in pkgutil.iter_modules([str(directory)]):
        name = info.name
        if "." in name:
            continue
        if name in ENTRYPOINT_WRAPPERS:
            continue
        names.append(name)
    return sorted(names)


def can_import(name: str) -> bool:
    if name in sys.modules:
        return True
    try:
        return importlib.util.find_spec(name) is not None
    except Exception:
        return False


def is_environment_skip(exc: BaseException) -> bool:
    name = getattr(exc, "name", None)
    if name in ENVIRONMENT_DEPENDENCIES and not can_import(name):
        return True
    return False


def register_paths(module_dir: Path) -> None:
    candidates = [module_dir] + [module_dir / sub for sub in SUBDIRECTORIES]
    for candidate in candidates:
        text = str(candidate)
        if candidate.is_dir() and text not in sys.path:
            sys.path.insert(0, text)


def import_targets(module_dir: Path) -> list[str]:
    ordered: list[str] = []
    seen: set[str] = set()
    directories = [module_dir] + [module_dir / sub for sub in SUBDIRECTORIES]
    for directory in directories:
        for name in importable_names(directory):
            if name not in seen:
                seen.add(name)
                ordered.append(name)
    return ordered


def load_module(module_dir: Path) -> tuple[list[str], list[str], list[tuple[str, str]]]:
    register_paths(module_dir)
    loaded: list[str] = []
    skipped: list[str] = []
    failed: list[tuple[str, str]] = []
    for name in import_targets(module_dir):
        try:
            importlib.import_module(name)
            loaded.append(name)
        except Exception as exc:
            if is_environment_skip(exc):
                skipped.append(name)
                continue
            failed.append((name, traceback.format_exc()))
    return loaded, skipped, failed


def run() -> int:
    root = mod_root()
    if not root.is_dir():
        print(f"Mod root not found: {root}", file=sys.stderr)
        return 2

    modules = discover_parashell_modules(root)
    if not modules:
        print(f"No Parashell modules found under {root}", file=sys.stderr)
        return 2

    freecad_available = can_import("FreeCAD")
    gui_available = can_import("FreeCADGui")
    print(f"Mod root: {root}")
    print(f"FreeCAD available: {freecad_available}")
    print(f"FreeCADGui available: {gui_available}")
    print(f"Modules under test: {', '.join(path.name for path in modules)}")

    failures: list[tuple[str, str]] = []
    for module_dir in modules:
        loaded, skipped, failed = load_module(module_dir)
        print(
            f"[{module_dir.name}] loaded={len(loaded)} "
            f"skipped={len(skipped)} failed={len(failed)}"
        )
        if skipped:
            print(f"  skipped (host dependency unavailable): {', '.join(skipped)}")
        for name, tb in failed:
            print(f"\n=== {module_dir.name}: FAILED to import '{name}' ===")
            print(tb)
        failures.extend((module_dir.name, name) for name, _ in failed)

    if failures:
        listing = ", ".join(f"{module}/{name}" for module, name in failures)
        print(f"\nModule load test FAILED for: {listing}", file=sys.stderr)
        return 1

    print("\nModule load test PASSED: every module imported without errors.")
    return 0


def main() -> int:
    try:
        return run()
    except Exception:
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    exit_code = main()
    sys.stdout.flush()
    sys.stderr.flush()
    os._exit(exit_code)
