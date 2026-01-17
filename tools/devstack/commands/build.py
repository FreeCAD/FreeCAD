from __future__ import annotations

import argparse
import os
import shutil
from pathlib import Path

from tools.devstack.core.adapters import load_adapter
from tools.devstack.core.cmake_presets import (
    cache_var_is_set,
    load_cmake_presets,
    resolve_preset_binary_dir,
    resolve_preset_cache_variables,
)
from tools.devstack.core.env import load_env_from_sh
from tools.devstack.core.git import git, repo_root
from tools.devstack.core.proc import die, have_cmd, note, run


def submodules_need_init(root: Path) -> bool:
    status = git(["submodule", "status", "--recursive"], cwd=root)
    for line in status.splitlines():
        if line.startswith("-"):
            return True
    return False


def ensure_submodules(root: Path, mode: str) -> None:
    if mode not in ("auto", "on", "off"):
        die(f"invalid --submodules mode: {mode} (expected: auto|on|off)")
    if mode == "off":
        return
    if mode == "auto" and not submodules_need_init(root):
        return
    note("running: git submodule update --init --recursive")
    run(["git", "submodule", "update", "--init", "--recursive"], cwd=root)


def cmd_build(args: argparse.Namespace) -> None:
    root = repo_root()

    preset = args.preset
    build_dir = args.build_dir or ""
    jobs = args.jobs
    target = args.target or ""
    toolchain = args.toolchain
    submodules = args.submodules
    build_mode = args.build_mode
    configure_only = args.configure_only
    build_only = args.build_only
    clean = args.clean
    distcc_requested = bool(args.distcc)
    no_distcc = bool(getattr(args, "no_distcc", False))
    distcc_mode = (getattr(args, "distcc_mode", None) or "ccache").strip().lower()
    distcc_hosts = (args.distcc_hosts or "").strip()
    distcc_verbose = bool(args.distcc_verbose)
    use_ccache_launcher_requested = bool(args.ccache_launcher)
    ccache_dir = (getattr(args, "ccache_dir", None) or "").strip()
    no_env_file = bool(args.no_env_file)
    env_file_arg = (args.env_file or "").strip()
    adapter = load_adapter(root, getattr(args, "adapter", "auto"))

    preset, build_mode, toolchain = adapter.resolve_build_preset(
        root=root,
        preset=preset,
        build_mode=build_mode,
        toolchain=toolchain,
        core_flag=bool(args.core),
        clang_mold_flag=bool(args.clang_mold),
    )

    if configure_only and build_only:
        die("cannot use --configure-only and --build-only together")
    if build_dir and not build_only:
        die("--build-dir requires --build-only (configure always uses the preset binaryDir)")

    if distcc_mode not in ("ccache", "direct"):
        die("--distcc-mode must be one of: ccache|direct")

    ensure_submodules(root, submodules)

    env: dict[str, str] = dict(os.environ)
    env_file: str = ""
    if not no_env_file:
        env_file = env_file_arg or os.environ.get("DEVSTACK_ENV_FILE", "").strip()
        if not env_file:
            candidate = root / ".devstack" / "env.sh"
            if candidate.is_file():
                env_file = str(candidate)
            else:
                candidate = Path.home() / ".config" / "devstack" / "env.sh"
                if candidate.is_file():
                    env_file = str(candidate)

    if env_file:
        env_path = Path(env_file)
        if not env_path.is_absolute():
            env_path = (root / env_path).resolve()
        note(f"loading env: {env_path}")
        env = load_env_from_sh(env_path, env)

    if ccache_dir:
        env["CCACHE_DIR"] = ccache_dir

    def env_truthy(key: str) -> bool:
        return env.get(key, "").strip().lower() in ("1", "true", "yes", "on")

    distcc = False
    if not no_distcc:
        distcc = distcc_requested
        if not distcc and env_truthy("DEVSTACK_AUTO_DISTCC") and env.get("DISTCC_HOSTS", "").strip():
            distcc = True

    if distcc:
        if distcc_hosts:
            env["DISTCC_HOSTS"] = distcc_hosts
        if not env.get("DISTCC_HOSTS", "").strip():
            die("distcc enabled but DISTCC_HOSTS is empty (set it in env or pass --distcc-hosts)")

        if not have_cmd("distcc"):
            die("distcc enabled but `distcc` is not on PATH (install distcc client)")
        if distcc_mode == "ccache":
            env.setdefault("CCACHE_PREFIX", "distcc")
            env.setdefault("CCACHE_CPP2", "yes")
            if not have_cmd("ccache"):
                die("distcc enabled (mode=ccache) but `ccache` is not on PATH")
        if distcc_verbose:
            env["DISTCC_VERBOSE"] = "1"

        note(f"distcc: enabled (DISTCC_HOSTS={env.get('DISTCC_HOSTS')})")
        if distcc_mode == "ccache":
            note(
                f"distcc: mode=ccache (CCACHE_PREFIX={env.get('CCACHE_PREFIX')} CCACHE_CPP2={env.get('CCACHE_CPP2')})"
            )
        else:
            note("distcc: mode=direct (compiler launcher=distcc)")
        if build_only:
            note(
                "distcc: build-only mode does not set compiler launchers; ensure this build dir was configured with "
                + (
                    "CMAKE_*_COMPILER_LAUNCHER=distcc"
                    if distcc_mode == "direct"
                    else "CMAKE_*_COMPILER_LAUNCHER=ccache"
                )
            )

    use_ccache_launcher = bool(use_ccache_launcher_requested or (distcc and distcc_mode == "ccache"))
    compiler_launcher = "distcc" if (distcc and distcc_mode == "direct") else ("ccache" if use_ccache_launcher else "")

    presets = load_cmake_presets(root)
    if not build_dir:
        build_dir = resolve_preset_binary_dir(presets, preset, root)
        if not build_dir:
            die(f"unknown preset or missing binaryDir: {preset}")

    def cmake_cache_get(cache_dir: str, key: str) -> str | None:
        cache_root = Path(cache_dir)
        if not cache_root.is_absolute():
            cache_root = (root / cache_root).resolve()
        cache_path = cache_root / "CMakeCache.txt"
        try:
            text = cache_path.read_text(errors="replace")
        except OSError:
            return None
        prefix = f"{key}:"
        for line in text.splitlines():
            if not line.startswith(prefix):
                continue
            _, _, value = line.partition("=")
            return value
        return None

    if clean:
        shutil.rmtree(build_dir, ignore_errors=True)

    if not build_only:
        print(f"cmake configure: preset={preset}")
        cfg_cmd = ["cmake", "--preset", preset]
        preset_cache = resolve_preset_cache_variables(presets, preset)

        def preset_cache_str(key: str) -> str:
            val = preset_cache.get(key)
            if isinstance(val, dict):
                val = val.get("value")
            if val is None:
                return ""
            if isinstance(val, str):
                return val
            return str(val)

        def preset_uses_clang() -> bool:
            c = preset_cache_str("CMAKE_C_COMPILER").strip()
            cxx = preset_cache_str("CMAKE_CXX_COMPILER").strip()
            return ("clang" in c.lower()) or ("clang" in cxx.lower())

        if distcc and distcc_mode == "ccache" and preset_uses_clang():
            flag = "-Wno-gnu-line-marker"
            extra: list[str] = []

            c_flags = cmake_cache_get(build_dir, "CMAKE_C_FLAGS")
            cxx_flags = cmake_cache_get(build_dir, "CMAKE_CXX_FLAGS")
            if c_flags is not None and flag not in c_flags:
                extra += ["-D", f"CMAKE_C_FLAGS:STRING={(c_flags + ' ' + flag).strip()}"]
            if cxx_flags is not None and flag not in cxx_flags:
                extra += ["-D", f"CMAKE_CXX_FLAGS:STRING={(cxx_flags + ' ' + flag).strip()}"]

            if not extra:

                def append_env_flag(k: str) -> None:
                    cur = env.get(k, "").strip()
                    if flag in cur.split():
                        return
                    env[k] = (cur + " " + flag).strip() if cur else flag

                append_env_flag("CFLAGS")
                append_env_flag("CXXFLAGS")
            else:
                cfg_cmd += extra

            note(f"clang: added {flag} (distcc/ccache builds can spam -Wgnu-line-marker under -Wpedantic)")

        if compiler_launcher:
            if compiler_launcher == "distcc":
                cfg_cmd.append("-D")
                cfg_cmd.append("CMAKE_C_COMPILER_LAUNCHER:STRING=distcc")
                cfg_cmd.append("-D")
                cfg_cmd.append("CMAKE_CXX_COMPILER_LAUNCHER:STRING=distcc")
            else:
                if not cache_var_is_set(preset_cache, "CMAKE_C_COMPILER_LAUNCHER"):
                    cfg_cmd.append("-D")
                    cfg_cmd.append("CMAKE_C_COMPILER_LAUNCHER:STRING=ccache")
                if not cache_var_is_set(preset_cache, "CMAKE_CXX_COMPILER_LAUNCHER"):
                    cfg_cmd.append("-D")
                    cfg_cmd.append("CMAKE_CXX_COMPILER_LAUNCHER:STRING=ccache")
        run(cfg_cmd, cwd=root, env=env)

    if not configure_only:
        print(f"cmake build: dir={build_dir} jobs={jobs}{' target='+target if target else ''}")
        cmd = ["cmake", "--build", build_dir, "-j", str(jobs)]
        if target:
            cmd += ["--target", target]
        run(cmd, cwd=root, env=env)

