from __future__ import annotations

import json
from pathlib import Path

from .proc import die, have_cmd


def load_cmake_presets(source_dir: Path) -> dict[str, dict]:
    presets: dict[str, dict] = {}
    for path in [source_dir / "CMakePresets.json", source_dir / "CMakeUserPresets.json"]:
        if not path.exists():
            continue
        try:
            data = json.loads(path.read_text(encoding="utf-8", errors="replace"))
        except Exception:
            continue
        for p in data.get("configurePresets") or []:
            name = p.get("name")
            if name:
                presets[name] = p
    return presets


def resolve_preset_cache_variables(presets: dict[str, dict], preset_name: str) -> dict[str, object]:
    seen: set[str] = set()

    def rec(name: str) -> dict[str, object]:
        if name in seen:
            return {}
        seen.add(name)
        p = presets.get(name) or {}
        cache: dict[str, object] = {}
        for parent in p.get("inherits") or []:
            cache.update(rec(parent))
        cache.update(p.get("cacheVariables") or {})
        return cache

    return rec(preset_name)


def cache_var_is_set(cache: dict[str, object], key: str) -> bool:
    if key not in cache:
        return False
    val = cache.get(key)
    if isinstance(val, dict):
        return bool(val.get("value"))
    return bool(val)


def resolve_preset_binary_dir(presets: dict[str, dict], preset_name: str, source_dir: Path) -> str:
    seen: set[str] = set()

    def rec(name: str) -> str:
        if name in seen:
            return ""
        seen.add(name)
        p = presets.get(name) or {}
        bd = p.get("binaryDir")
        if bd:
            return bd
        for parent in p.get("inherits") or []:
            got = rec(parent)
            if got:
                return got
        return ""

    bd = rec(preset_name)
    if not bd:
        return ""
    bd = bd.replace("${sourceDir}", str(source_dir))
    if "${" not in bd:
        p = Path(bd)
        if not p.is_absolute():
            bd = str((source_dir / p).resolve())
    return bd


def cmake_user_presets_path(source_dir: Path) -> Path:
    return source_dir / "CMakeUserPresets.json"


def load_or_init_user_presets(source_dir: Path) -> dict:
    path = cmake_user_presets_path(source_dir)
    if path.exists():
        try:
            return json.loads(path.read_text(encoding="utf-8", errors="replace"))
        except Exception:
            return {}
    return {}


def upsert_configure_preset(user: dict, preset: dict) -> None:
    user.setdefault("version", 3)
    user.setdefault("cmakeMinimumRequired", {"major": 3, "minor": 16, "patch": 3})
    presets = user.setdefault("configurePresets", [])
    name = preset.get("name")
    if not name:
        return
    for i, existing in enumerate(presets):
        if existing.get("name") == name:
            presets[i] = preset
            return
    presets.append(preset)


def write_user_presets(source_dir: Path, user: dict) -> None:
    cmake_user_presets_path(source_dir).write_text(json.dumps(user, indent=2) + "\n", encoding="utf-8")


def ensure_clang_mold_presets(source_dir: Path) -> None:
    if not (have_cmd("clang") and have_cmd("clang++") and have_cmd("mold")):
        die("clang/clang++/mold not found; install them or omit --toolchain clang-mold")

    user = load_or_init_user_presets(source_dir)
    cache = {
        "CMAKE_C_COMPILER": {"type": "STRING", "value": "clang"},
        "CMAKE_CXX_COMPILER": {"type": "STRING", "value": "clang++"},
        "CMAKE_EXE_LINKER_FLAGS": {"type": "STRING", "value": "-fuse-ld=mold"},
        "CMAKE_SHARED_LINKER_FLAGS": {"type": "STRING", "value": "-fuse-ld=mold"},
        "CMAKE_MODULE_LINKER_FLAGS": {"type": "STRING", "value": "-fuse-ld=mold"},
    }
    upsert_configure_preset(
        user,
        {
            "name": "clang-mold-debug",
            "displayName": "Clang+Mold Debug (local)",
            "description": "Local toolchain preset: clang/clang++ + mold linker, Debug build.",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/clang-mold-debug",
            "inherits": ["debug"],
            "cacheVariables": cache,
        },
    )
    upsert_configure_preset(
        user,
        {
            "name": "clang-mold-release",
            "displayName": "Clang+Mold Release (local)",
            "description": "Local toolchain preset: clang/clang++ + mold linker, Release build.",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/clang-mold-release",
            "inherits": ["release"],
            "cacheVariables": cache,
        },
    )
    write_user_presets(source_dir, user)
