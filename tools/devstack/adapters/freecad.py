from __future__ import annotations

ADAPTER_NAME = "freecad"

def is_repo(root: Path) -> bool:
    # Heuristic: this is intentionally lightweight and filesystem-based.
    return (root / "src" / "Gui").is_dir() and (root / "src" / "App").is_dir() and (root / "CMakeLists.txt").is_file()

from pathlib import Path

from tools.devstack.core.cmake_presets import (
    ensure_clang_mold_presets,
    load_or_init_user_presets,
    upsert_configure_preset,
    write_user_presets,
)
from tools.devstack.core.cache import devstack_cache_dir
from tools.devstack.core.proc import die, have_cmd


def ensure_core_presets(source_dir: Path) -> None:
    user = load_or_init_user_presets(source_dir)
    core_modules = {
        "BUILD_GUI": {"type": "BOOL", "value": "ON"},
        "BUILD_MATERIAL": {"type": "BOOL", "value": "ON"},
        "BUILD_PART": {"type": "BOOL", "value": "ON"},
        "BUILD_ADDONMGR": {"type": "BOOL", "value": "OFF"},
        "BUILD_ASSEMBLY": {"type": "BOOL", "value": "OFF"},
        "BUILD_BIM": {"type": "BOOL", "value": "OFF"},
        "BUILD_CAM": {"type": "BOOL", "value": "OFF"},
        "BUILD_CLOUD": {"type": "BOOL", "value": "OFF"},
        "BUILD_DRAFT": {"type": "BOOL", "value": "OFF"},
        "BUILD_DRAWING": {"type": "BOOL", "value": "OFF"},
        "BUILD_FEM": {"type": "BOOL", "value": "OFF"},
        "BUILD_HELP": {"type": "BOOL", "value": "OFF"},
        "BUILD_IDF": {"type": "BOOL", "value": "OFF"},
        "BUILD_IMPORT": {"type": "BOOL", "value": "OFF"},
        "BUILD_INSPECTION": {"type": "BOOL", "value": "OFF"},
        "BUILD_JTREADER": {"type": "BOOL", "value": "OFF"},
        "BUILD_MATERIAL_EXTERNAL": {"type": "BOOL", "value": "OFF"},
        "BUILD_MEASURE": {"type": "BOOL", "value": "OFF"},
        "BUILD_MESH": {"type": "BOOL", "value": "OFF"},
        "BUILD_MESH_PART": {"type": "BOOL", "value": "OFF"},
        "BUILD_FLAT_MESH": {"type": "BOOL", "value": "OFF"},
        "BUILD_OPENSCAD": {"type": "BOOL", "value": "OFF"},
        "BUILD_PART_DESIGN": {"type": "BOOL", "value": "OFF"},
        "BUILD_PLOT": {"type": "BOOL", "value": "OFF"},
        "BUILD_POINTS": {"type": "BOOL", "value": "OFF"},
        "BUILD_REVERSEENGINEERING": {"type": "BOOL", "value": "OFF"},
        "BUILD_ROBOT": {"type": "BOOL", "value": "OFF"},
        "BUILD_SANDBOX": {"type": "BOOL", "value": "OFF"},
        "BUILD_SHOW": {"type": "BOOL", "value": "OFF"},
        "BUILD_SKETCHER": {"type": "BOOL", "value": "OFF"},
        "BUILD_SPREADSHEET": {"type": "BOOL", "value": "OFF"},
        "BUILD_START": {"type": "BOOL", "value": "OFF"},
        "BUILD_SURFACE": {"type": "BOOL", "value": "OFF"},
        "BUILD_TECHDRAW": {"type": "BOOL", "value": "OFF"},
        "BUILD_TEST": {"type": "BOOL", "value": "OFF"},
        "BUILD_TEMPLATE": {"type": "BOOL", "value": "OFF"},
        "BUILD_TUX": {"type": "BOOL", "value": "OFF"},
        "BUILD_VR": {"type": "BOOL", "value": "OFF"},
        "BUILD_WEB": {"type": "BOOL", "value": "OFF"},
        "ENABLE_DEVELOPER_TESTS": {"type": "BOOL", "value": "OFF"},
        "FREECAD_USE_PCL": {"type": "BOOL", "value": "OFF"},
    }
    upsert_configure_preset(
        user,
        {
            "name": "core-debug",
            "displayName": "Core Debug (Part-only, local)",
            "description": "Reduced build: GUI + Material + Part (most modules OFF), Debug.",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/core-debug",
            "inherits": ["debug"],
            "cacheVariables": core_modules,
        },
    )
    upsert_configure_preset(
        user,
        {
            "name": "core-release",
            "displayName": "Core Release (Part-only, local)",
            "description": "Reduced build: GUI + Material + Part (most modules OFF), Release.",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/core-release",
            "inherits": ["release"],
            "cacheVariables": core_modules,
        },
    )
    write_user_presets(source_dir, user)


def _merge_cache_variables(preset_a: dict, preset_b: dict) -> dict:
    out: dict = {}
    out.update(preset_a.get("cacheVariables") or {})
    out.update(preset_b.get("cacheVariables") or {})
    return out


def ensure_core_clang_mold_combined(source_dir: Path, variant: str) -> str:
    # variant: debug|release
    ensure_core_presets(source_dir)
    ensure_clang_mold_presets(source_dir)
    user = load_or_init_user_presets(source_dir)
    presets = {p.get("name"): p for p in (user.get("configurePresets") or []) if p.get("name")}
    core_name = f"core-{variant}"
    tool_name = f"clang-mold-{variant}"
    combined_name = f"core-clang-mold-{variant}"
    cache = _merge_cache_variables(presets.get(core_name, {}), presets.get(tool_name, {}))
    upsert_configure_preset(
        user,
        {
            "name": combined_name,
            "displayName": f"Core Clang+Mold {variant.capitalize()} (local)",
            "description": f"Reduced build (core modules) + clang/mold toolchain, {variant.capitalize()}.",
            "generator": "Ninja",
            "binaryDir": f"${{sourceDir}}/build/{combined_name}",
            "inherits": [variant],
            "cacheVariables": cache,
        },
    )
    write_user_presets(source_dir, user)
    return combined_name


def resolve_build_preset(
    *,
    root: Path,
    preset: str,
    build_mode: str,
    toolchain: str,
    core_flag: bool,
    clang_mold_flag: bool,
) -> tuple[str, str, str]:
    if clang_mold_flag:
        toolchain = "clang-mold"
    if core_flag:
        build_mode = "core"

    if build_mode == "core":
        ensure_core_presets(root)
        if preset in ("debug", "release"):
            preset = f"core-{preset}"
    elif build_mode != "full":
        die(f"invalid --build-mode: {build_mode} (expected: full|core)")

    if toolchain == "auto":
        toolchain = "clang-mold" if (have_cmd("clang") and have_cmd("clang++") and have_cmd("mold")) else "default"

    if toolchain == "default":
        return preset, build_mode, toolchain

    if toolchain != "clang-mold":
        die(f"invalid --toolchain: {toolchain} (expected: default|clang-mold|auto)")

    # If user already picked a toolchain preset explicitly, leave it as-is.
    if preset.startswith("clang-mold-") or preset.startswith("core-clang-mold-"):
        return preset, build_mode, toolchain

    ensure_clang_mold_presets(root)

    if preset in ("debug", "release"):
        preset = f"clang-mold-{preset}"

    if build_mode == "core":
        variant = "debug" if preset.endswith("debug") else ("release" if preset.endswith("release") else "")
        if variant:
            preset = ensure_core_clang_mold_combined(root, variant)

    return preset, build_mode, toolchain


def lint_default_base_ref(root: Path) -> str:
    import os

    remote = os.environ.get("DEVSTACK_STACK_REMOTE", "origin")
    main = os.environ.get("DEVSTACK_STACK_MAIN_BRANCH", "main")
    return f"{remote}/{main}"


def lint_venv_bin(root: Path) -> Path:
    return devstack_cache_dir("python-lint", "venv", "bin")
