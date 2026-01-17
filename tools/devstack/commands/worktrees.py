from __future__ import annotations

import argparse
import os
import shutil
import subprocess
from pathlib import Path
from typing import Optional

from tools.devstack.core.git import (
    current_branch,
    git,
    repo_root,
    sanitize_branch_to_conf_name,
)
from tools.devstack.core.proc import die, note, run
from tools.devstack.core.stackconf import default_body_dir, read_conf, stack_name_from_conf

def default_pr_prefix_for_branch(root: Path, conf_path: Path) -> str:
    b = current_branch(root)
    if not b:
        return f"pr/{stack_name_from_conf(conf_path)}/"
    leaf = sanitize_branch_to_conf_name(b.split("/")[-1])
    return f"pr/{leaf}/"

def cmd_init(args: argparse.Namespace) -> None:
    root = repo_root()
    stack_dir = root / ".devstack"

    mode = args.mode
    force_script = args.force or args.force_script
    force_conf = args.force or args.force_conf

    devstack_dir = Path(__file__).resolve().parents[1]
    here_py = (devstack_dir / "devstack.py").resolve()
    here_sh = (devstack_dir / "devstack.sh").resolve()

    dst_py = stack_dir / "devstack.py"
    dst_sh = stack_dir / "devstack.sh"

    if mode == "symlink":
        if force_script or not dst_py.exists():
            dst_py.parent.mkdir(parents=True, exist_ok=True)
            if dst_py.exists() or dst_py.is_symlink():
                dst_py.unlink()
            dst_py.symlink_to(here_py)
            print(f"installed {dst_py} (symlink -> {here_py})")
        if force_script or not dst_sh.exists():
            dst_sh.parent.mkdir(parents=True, exist_ok=True)
            if dst_sh.exists() or dst_sh.is_symlink():
                dst_sh.unlink()
            dst_sh.symlink_to(here_sh)
            print(f"installed {dst_sh} (symlink -> {here_sh})")
    else:
        if force_script or not dst_py.exists():
            dst_py.parent.mkdir(parents=True, exist_ok=True)
            if dst_py.exists() and dst_py.resolve() == here_py:
                note(f"{dst_py} already up to date")
            else:
                shutil.copy2(here_py, dst_py)
                dst_py.chmod(0o755)
                print(f"installed {dst_py}")
        else:
            note(f"{dst_py} already exists (use --force-script to overwrite)")
        if force_script or not dst_sh.exists():
            dst_sh.parent.mkdir(parents=True, exist_ok=True)
            if dst_sh.exists() and dst_sh.resolve() == here_sh:
                note(f"{dst_sh} already up to date")
            else:
                shutil.copy2(here_sh, dst_sh)
                dst_sh.chmod(0o755)
                print(f"installed {dst_sh}")
        else:
            note(f"{dst_sh} already exists (use --force-script to overwrite)")


    conf_path = stack_dir / "stack.conf"
    if conf_path.exists() and not force_conf:
        note(f"{conf_path} already exists (use --force-conf to overwrite)")
    else:
        conf_path.parent.mkdir(parents=True, exist_ok=True)
        remote = os.environ.get("DEVSTACK_STACK_REMOTE", "origin")
        main = os.environ.get("DEVSTACK_STACK_MAIN_BRANCH", "main")
        pr_prefix = default_pr_prefix_for_branch(root, conf_path)
        body_dir = default_body_dir(root, conf_path, pr_prefix)
        conf_path.write_text(
            "\n".join(
                [
                    "# Cut-point branches for stacked PRs (GitHub).",
                    "#",
                    "# This file is local-only (gitignored). Fill in cut points as you decide how to split your branch.",
                    "#",
                    "# Config directives:",
                    "#   base <remote>/<branch>         Base branch for the first PR (default: origin/main)",
                    "#   pr_prefix <prefix/>            Prefix to apply to branch keys (optional)",
                    "#   body_dir <path>                PR body directory for this stack (optional)",
                    "#   cut_prefix <prefix/>           Source cut-point branch prefix (optional; used when ignore is enabled)",
                    "#   ignore <commit-ish|ref>        Exclude commit from generated PR branches (optional; enables filtered mode)",
                    "#",
                    "# Stack entries:",
                    "#   <branch-key-or-name> <commit-ish> [body-file]",
                    "",
                    f"base {remote}/{main}",
                    "",
                    f"pr_prefix {pr_prefix}",
                    "",
                    f"body_dir {body_dir}",
                    "",
                    "# Example:",
                    "# 001-my-layer <sha>",
                    "",
                ]
            ),
            encoding="utf-8",
        )
        print(f"created {conf_path}")

    conf = read_conf(root)
    body_dir_path = Path(conf.body_dir)
    if not body_dir_path.is_absolute():
        body_dir_path = (root / body_dir_path).resolve()
    body_dir_path.mkdir(parents=True, exist_ok=True)
    print(f"ensured {body_dir_path}")

    print()
    print("next:")
    print(f"  edit: {conf_path}")
    print(f"  then: {dst_sh} list")


def worktree_list_paths(root: Path) -> list[Path]:
    proc = run(["git", "worktree", "list", "--porcelain"], cwd=root, capture=True)
    paths: list[Path] = []
    for line in proc.stdout.splitlines():
        if line.startswith("worktree "):
            paths.append(Path(line.split(" ", 1)[1]).resolve())
    return paths


def worktree_current_branch(wt: Path) -> str:
    try:
        b = git(["branch", "--show-current"], cwd=wt)
        if b:
            return b
    except subprocess.CalledProcessError:
        pass
    try:
        bisect_path = Path(git(["rev-parse", "--git-path", "BISECT_START"], cwd=wt))
        if bisect_path.is_file():
            return bisect_path.read_text(encoding="utf-8", errors="replace").strip()
    except Exception:
        pass
    return ""


def import_devstack_for_branch(from_root: Path, to_dir: Path, branch: str) -> None:
    stack_name = sanitize_branch_to_conf_name(branch)
    dst_conf = to_dir / ".devstack" / "stack.conf"
    if not dst_conf.exists():
        # Prefer legacy per-branch config storage if it exists in import_from.
        src_legacy = from_root / ".devstack" / "stacks" / f"{stack_name}.conf"
        src_conf: Optional[Path] = None
        if src_legacy.is_file():
            src_conf = src_legacy
        else:
            # Otherwise only copy stack.conf if the import_from worktree is on the same branch.
            src_stack_conf = from_root / ".devstack" / "stack.conf"
            if src_stack_conf.is_file() and current_branch(from_root) == branch:
                src_conf = src_stack_conf

        if src_conf and src_conf.is_file():
            dst_conf.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src_conf, dst_conf)
            if src_conf.parent.name == "stacks":
                ensure_body_dir_directive(dst_conf, f".devstack/pr-bodies/{src_conf.stem}")
            print(f"imported stack config: {dst_conf}")

    src_bodies = from_root / ".devstack" / "pr-bodies" / stack_name
    dst_bodies = to_dir / ".devstack" / "pr-bodies" / stack_name
    if src_bodies.is_dir() and not dst_bodies.exists():
        dst_bodies.mkdir(parents=True, exist_ok=True)
        for item in src_bodies.iterdir():
            if item.is_file():
                shutil.copy2(item, dst_bodies / item.name)
        print(f"imported pr bodies: {dst_bodies}")

    src_readme = from_root / ".devstack" / "README.md"
    dst_readme = to_dir / ".devstack" / "README.md"
    if src_readme.is_file() and not dst_readme.exists():
        dst_readme.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src_readme, dst_readme)


def cmd_wt_sync(args: argparse.Namespace) -> None:
    root = repo_root()
    dry = bool(args.dry_run)
    mode = args.mode
    import_enabled = not args.no_import
    import_from = Path(args.import_from).resolve() if args.import_from else root
    force_script = not args.no_force_script

    print(
        f"syncing devstack to worktrees (mode={mode}, import={1 if import_enabled else 0}, force_script={1 if force_script else 0})",
        flush=True,
    )

    script_py = Path(__file__).resolve()
    script_sh = (script_py.parent / "devstack.sh").resolve()

    for wt in worktree_list_paths(root):
        b = worktree_current_branch(wt)
        print(f"- {wt}{f' ({b})' if b else ''}", flush=True)
        if dry:
            continue
        try:
            if import_enabled and b:
                import_devstack_for_branch(import_from, wt, b)

            dst_stack_dir = wt / ".devstack"
            dst_py = dst_stack_dir / "devstack.py"
            dst_sh = dst_stack_dir / "devstack.sh"
            dst_stack_dir.mkdir(parents=True, exist_ok=True)

            if force_script or not dst_py.exists():
                if mode == "symlink":
                    if dst_py.exists() or dst_py.is_symlink():
                        dst_py.unlink()
                    dst_py.symlink_to(script_py)
                else:
                    if dst_py.exists() and dst_py.resolve() == script_py:
                        pass
                    else:
                        shutil.copy2(script_py, dst_py)
                        dst_py.chmod(0o755)

            if force_script or not dst_sh.exists():
                if mode == "symlink":
                    if dst_sh.exists() or dst_sh.is_symlink():
                        dst_sh.unlink()
                    dst_sh.symlink_to(script_sh)
                else:
                    if dst_sh.exists() and dst_sh.resolve() == script_sh:
                        pass
                    else:
                        shutil.copy2(script_sh, dst_sh)
                        dst_sh.chmod(0o755)

            # Ensure config/body dirs exist for the branch in that worktree.
            init_args = ["init", "--copy" if mode == "copy" else "--symlink"]
            if force_script:
                init_args.append("--force-script")
            run(["python3", str(dst_py), *init_args], cwd=wt)
        except Exception as exc:
            note(f"wt-sync failed for {wt}: {exc}")
            continue


def cmd_wt_feature(args: argparse.Namespace) -> None:
    root = repo_root()
    remote = os.environ.get("DEVSTACK_STACK_REMOTE", "origin")
    main = os.environ.get("DEVSTACK_STACK_MAIN_BRANCH", "main")

    name = args.name
    branch = args.branch or f"feature/{name}"
    base_ref = args.base or f"{remote}/{main}"
    safe = sanitize_branch_to_conf_name(name)
    dir_path = Path(args.dir).resolve() if args.dir else (root.parent / f"FreeCAD-wt-{safe}").resolve()
    if dir_path.exists():
        die(f"worktree path already exists: {dir_path}")

    print("creating worktree:")
    print(f"  branch: {branch}")
    print(f"  base:   {base_ref}")
    print(f"  path:   {dir_path}")

    try:
        run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{branch}"], cwd=root, capture=True)
        exists = True
    except subprocess.CalledProcessError:
        exists = False

    if exists:
        run(["git", "worktree", "add", str(dir_path), branch], cwd=root)
    else:
        run(["git", "worktree", "add", "-b", branch, str(dir_path), base_ref], cwd=root)

    init_mode = "symlink" if args.symlink else "copy"
    run(["python3", str(Path(__file__).resolve()), "init", f"--{init_mode}"], cwd=dir_path)

    if args.build:
        build_args = [
            "build",
            "--preset",
            args.preset,
            "--adapter",
            getattr(args, "adapter", os.environ.get("DEVSTACK_ADAPTER", "auto")),
            "--jobs",
            str(args.jobs),
            "--toolchain",
            args.toolchain,
            "--build-mode",
            args.build_mode,
        ]
        if args.core:
            build_args.append("--core")
        if args.clang_mold:
            build_args.append("--clang-mold")
        if args.distcc:
            build_args.append("--distcc")
        if getattr(args, 'no_distcc', False):
            build_args.append("--no-distcc")
        if args.distcc_hosts:
            build_args += ["--distcc-hosts", args.distcc_hosts]
        if args.distcc_verbose:
            build_args.append("--distcc-verbose")
        if args.ccache_launcher:
            build_args.append("--ccache-launcher")
        if args.env_file:
            build_args += ["--env-file", args.env_file]
        if args.no_env_file:
            build_args.append("--no-env-file")
        if args.target:
            build_args += ["--target", args.target]
        if args.clean:
            build_args.append("--clean")
        run(["python3", str(Path(__file__).resolve()), *build_args], cwd=dir_path)

    print()
    print("next:")
    print(f"  cd {dir_path}")
    print("  ./tools/devstack/devstack.sh list")


def cmd_wt_add(args: argparse.Namespace) -> None:
    root = repo_root()
    remote = os.environ.get("DEVSTACK_STACK_REMOTE", "origin")
    branch = args.branch

    work_branch = branch
    try:
        run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{branch}"], cwd=root, capture=True)
        exists_local = True
    except subprocess.CalledProcessError:
        exists_local = False

    if not exists_local:
        try:
            run(["git", "show-ref", "--verify", "--quiet", f"refs/remotes/{branch}"], cwd=root, capture=True)
            exists_remote = True
        except subprocess.CalledProcessError:
            exists_remote = False
        if exists_remote:
            local_branch = branch
            if branch.startswith(f"{remote}/"):
                local_branch = branch[len(remote) + 1 :]
            try:
                run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{local_branch}"], cwd=root, capture=True)
            except subprocess.CalledProcessError:
                run(["git", "branch", local_branch, branch], cwd=root)
            work_branch = local_branch
        else:
            die(f"unknown branch/ref: {branch}")

    safe = sanitize_branch_to_conf_name(work_branch)
    dir_path = Path(args.dir).resolve() if args.dir else (root.parent / f"FreeCAD-wt-{safe}").resolve()
    if dir_path.exists():
        die(f"worktree path already exists: {dir_path}")

    print("creating worktree:")
    print(f"  branch: {work_branch}")
    print(f"  path:   {dir_path}")
    run(["git", "worktree", "add", str(dir_path), work_branch], cwd=root)

    if not args.no_import:
        import_devstack_for_branch(Path(args.import_from).resolve() if args.import_from else root, dir_path, work_branch)

    init_mode = "symlink" if args.symlink else "copy"
    run(["python3", str(Path(__file__).resolve()), "init", f"--{init_mode}"], cwd=dir_path)

    if args.build:
        build_args = [
            "build",
            "--preset",
            args.preset,
            "--adapter",
            getattr(args, "adapter", os.environ.get("DEVSTACK_ADAPTER", "auto")),
            "--jobs",
            str(args.jobs),
            "--toolchain",
            args.toolchain,
            "--build-mode",
            args.build_mode,
        ]
        if args.core:
            build_args.append("--core")
        if args.clang_mold:
            build_args.append("--clang-mold")
        if args.distcc:
            build_args.append("--distcc")
        if getattr(args, 'no_distcc', False):
            build_args.append("--no-distcc")
        if args.distcc_hosts:
            build_args += ["--distcc-hosts", args.distcc_hosts]
        if args.distcc_verbose:
            build_args.append("--distcc-verbose")
        if args.ccache_launcher:
            build_args.append("--ccache-launcher")
        if args.env_file:
            build_args += ["--env-file", args.env_file]
        if args.no_env_file:
            build_args.append("--no-env-file")
        if args.target:
            build_args += ["--target", args.target]
        if args.clean:
            build_args.append("--clean")
        run(["python3", str(Path(__file__).resolve()), *build_args], cwd=dir_path)

    print()
    print("next:")
    print(f"  cd {dir_path}")
    print("  ./tools/devstack/devstack.sh list")


def _infer_repo_series_name(root: Path) -> str:
    name = root.name
    if "-wt-" in name:
        return name.split("-wt-", 1)[0]
    return name


def _copy_stack_state(from_root: Path, to_dir: Path) -> None:
    """Copy stack.conf + PR bodies into the new worktree (best-effort)."""
    try:
        conf = read_conf(from_root)
    except SystemExit:
        return

    dst_conf = to_dir / ".devstack" / "stack.conf"
    if not dst_conf.exists():
        try:
            dst_conf.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(conf.path, dst_conf)
        except OSError:
            pass

    # Copy PR bodies (small; helps `--layer` usage in the new worktree).
    src_bodies = (from_root / conf.body_dir).resolve()
    dst_bodies = (to_dir / conf.body_dir).resolve()
    if src_bodies.is_dir() and not dst_bodies.exists():
        try:
            dst_bodies.mkdir(parents=True, exist_ok=True)
            for item in src_bodies.iterdir():
                if item.is_file():
                    shutil.copy2(item, dst_bodies / item.name)
        except OSError:
            pass


def cmd_wt_layer(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)

    layer = int(args.layer)
    if layer < 1 or layer > len(conf.entries):
        die(f"layer must be 1..{len(conf.entries)}")

    view = (args.view or "source").strip().lower()
    if view not in ("source", "pr"):
        die("--view must be one of: source|pr")

    entry = conf.entries[layer - 1]
    stack_name = stack_name_from_conf(conf.path)
    key_safe = sanitize_key_to_filename(entry.key)

    # Resolve the ref to check out.
    ref: str
    if view == "source":
        if filtered_mode(conf):
            ref = cut_branch_for_entry(conf, entry)
            try:
                run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{ref}"], cwd=root)
            except subprocess.CalledProcessError:
                die(f"missing cut branch for layer {layer}: {ref} (run: ds update)")
        else:
            ensure_commit_exists(root, entry.sha)
            ref = entry.sha
    else:
        # PR view is for inspection; do not commit directly on generated branches in filtered mode.
        ref = entry.branch
        try:
            run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{ref}"], cwd=root)
        except subprocess.CalledProcessError:
            # Fall back to the SHA if the PR branch doesn't exist yet.
            ensure_commit_exists(root, entry.sha)
            ref = entry.sha

    series = _infer_repo_series_name(root)
    default_dir = root.parent / f"{series}-wt-layer-{key_safe}{'-pr' if view == 'pr' else ''}"
    dir_path = Path(args.dir).resolve() if args.dir else default_dir.resolve()
    if dir_path.exists():
        die(f"worktree path already exists: {dir_path}")

    branch = (args.branch or "").strip()
    if not branch and view == "source":
        branch = f"layer/{sanitize_branch_to_conf_name(stack_name)}/{key_safe}"

    print("creating worktree:")
    print(f"  dir:    {dir_path}")
    print(f"  view:   {view}")
    print(f"  ref:    {ref}")
    if branch:
        print(f"  branch: {branch}")

    def branch_worktree_paths(branch_name: str) -> list[str]:
        try:
            out = git(["worktree", "list", "--porcelain"], cwd=root)
        except subprocess.CalledProcessError:
            return []
        current_path = ""
        paths: list[str] = []
        for line in out.splitlines():
            if line.startswith("worktree "):
                current_path = line.split(" ", 1)[1].strip()
                continue
            if line.startswith("branch "):
                b = line.split(" ", 1)[1].strip()
                if b == f"refs/heads/{branch_name}" and current_path:
                    paths.append(current_path)
        return paths

    if branch:
        try:
            run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{branch}"], cwd=root)
            if not getattr(args, "force", False):
                die(f"branch already exists: {branch} (use --branch to pick a different name, or --force)")
            if current_branch(root) == branch:
                die(f"refusing to delete currently checked out branch: {branch}")
            paths = branch_worktree_paths(branch)
            if paths:
                die(f"refusing to delete {branch}; checked out in worktree(s): {', '.join(paths)}")
            run(["git", "branch", "-D", branch], cwd=root, check=True)
        except subprocess.CalledProcessError:
            pass
        run(["git", "worktree", "add", "-b", branch, str(dir_path), ref], cwd=root)
    else:
        run(["git", "worktree", "add", str(dir_path), ref], cwd=root)

    _copy_stack_state(root, dir_path)

    print()
    print("next:")
    print(f"  cd {dir_path}")
    if view == "pr":
        print("  # PR view is for inspection; avoid committing here if you use `ignore`/filtered stacks.")
    else:
        print("  # Make changes and commit here; then bring them back via rebase/cherry-pick on your main stack branch.")
    print("  ds lint --layer 1")
