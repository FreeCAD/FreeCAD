from __future__ import annotations

import argparse
import os
import subprocess
from pathlib import Path
from typing import Optional

from tools.devstack.core.git import (
    current_branch,
    ensure_clean_worktree,
    ensure_commit_exists,
    git,
    repo_root,
    resolve_commitish,
    sanitize_branch_to_conf_name,
    sanitize_key_to_filename,
)
from tools.devstack.core.proc import die, note, run
from tools.devstack.core.stackconf import (
    StackConfig,
    StackEntry,
    base_branch_name,
    cut_branch_for_entry,
    default_body_dir,
    filtered_mode,
    read_conf,
    stack_name_from_conf,
)


def cmd_list(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)
    print(f"Config: {conf.path}")
    for entry in conf.entries:
        subj = ""
        try:
            subj = git(["show", "-s", "--format=%s", entry.sha], cwd=root)
        except subprocess.CalledProcessError:
            subj = ""
        label = entry.branch if entry.key == entry.branch else f"{entry.key} ({entry.branch})"
        body = entry.body or str((Path(conf.body_dir) / f"{sanitize_key_to_filename(entry.key)}.md"))
        print(f"{label:<40} {entry.sha}  {subj}  ({body})")


def cmd_log(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)
    prev = f"refs/remotes/{conf.base_remote_ref}"
    for entry in conf.entries:
        ensure_commit_exists(root, entry.sha)
        print()
        print(f"== {entry.branch} ({prev}..{entry.sha}) ==")
        run(["git", "log", "--oneline", f"{prev}..{entry.sha}"], cwd=root, check=False)
        prev = entry.sha


def resolve_ignored_commits(root: Path, conf: StackConfig) -> set[str]:
    """Resolve ignore entries to full commit SHAs."""
    ignored: set[str] = set()
    for item in conf.ignore:
        ignored.add(resolve_commitish(root, item))
    return ignored


def cmd_update(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)
    current = git(["rev-parse", "--abbrev-ref", "HEAD"], cwd=root)
    if filtered_mode(conf):
        # Source cut points live as local branches (so `rebase --update-refs` can keep them in sync).
        for entry in conf.entries:
            ensure_commit_exists(root, entry.sha)
            cut_branch = cut_branch_for_entry(conf, entry)
            if current in (entry.branch, cut_branch):
                die(f"refusing to move currently checked out branch: {current}")
            run(["git", "branch", "-f", cut_branch, entry.sha], cwd=root)
            print(f"moved {cut_branch} -> {entry.sha}")

        # Generated PR branches omit ignored commits by replaying the source range onto the PR base chain.
        ensure_clean_worktree(root)
        orig = current
        ignored = resolve_ignored_commits(root, conf)
        prev_source = conf.base_remote_ref
        prev_pr_base = conf.base_remote_ref
        for entry in conf.entries:
            ensure_commit_exists(root, entry.sha)
            pr_branch = entry.branch
            from_ref = prev_source
            to_ref = entry.sha
            commits_raw = git(["rev-list", "--reverse", "--no-merges", f"{from_ref}..{to_ref}"], cwd=root)
            commits = [c for c in commits_raw.splitlines() if c and c not in ignored]
            try:
                run(["git", "checkout", "-B", pr_branch, prev_pr_base], cwd=root)
                if commits:
                    run(["git", "cherry-pick", *commits], cwd=root)
            except subprocess.CalledProcessError:
                run(["git", "cherry-pick", "--abort"], cwd=root, check=False)
                run(["git", "checkout", orig], cwd=root, check=False)
                die(
                    f"failed to generate {pr_branch} (conflict or empty cherry-pick); resolve manually or remove the ignore rule"
                )
            print(f"generated {pr_branch}")
            prev_source = entry.sha
            prev_pr_base = pr_branch

        run(["git", "checkout", orig], cwd=root)
        return

    for entry in conf.entries:
        ensure_commit_exists(root, entry.sha)
        if current == entry.branch:
            die(f"refusing to move currently checked out branch: {entry.branch}")
        run(["git", "branch", "-f", entry.branch, entry.sha], cwd=root)
        print(f"moved {entry.branch} -> {entry.sha}")


def select_entries(conf: StackConfig, only_layer: int | None) -> list[StackEntry]:
    if only_layer is None:
        return list(conf.entries)
    if only_layer < 1 or only_layer > len(conf.entries):
        die(f"--only must be 1..{len(conf.entries)}")
    return [conf.entries[only_layer - 1]]


def pr_base_for_layer(conf: StackConfig, layer: int) -> str:
    base = base_branch_name(conf.base_remote_ref)
    if layer <= 1:
        return base
    return conf.entries[layer - 2].branch


def write_conf(root: Path, conf: StackConfig, entries: list[tuple[str, str, str]]) -> None:
    # entries: (key, sha, body_or_empty)
    default_dir = default_body_dir(root, conf.path, conf.pr_prefix)
    lines: list[str] = []
    lines.append("# Cut-point branches for stacked PRs (GitHub).")
    lines.append("#")
    lines.append("# Config directives:")
    lines.append("#   base <remote>/<branch>         Base branch for the first PR (default: origin/main)")
    lines.append("#   pr_prefix <prefix/>            Prefix to apply to non-pr/* branch keys (optional)")
    lines.append("#   body_dir <path>                PR body directory for this stack (optional)")
    lines.append("#   cut_prefix <prefix/>           Source cut-point branch prefix (optional; used when ignore is enabled)")
    lines.append("#   ignore <commit-ish|ref>        Exclude commit from generated PR branches (optional; enables filtered mode)")
    lines.append("#")
    lines.append("# Stack entries:")
    lines.append("#   <branch-key-or-name> <commit-ish> [body-file]")
    lines.append("#")
    lines.append("# This file is generated/updated by `tools/devstack/devstack.sh capture`.")
    lines.append("")
    lines.append(f"base {conf.base_remote_ref}")
    lines.append("")
    if conf.pr_prefix:
        lines.append(f"pr_prefix {conf.pr_prefix}")
        lines.append("")
    if filtered_mode(conf) and conf.cut_prefix:
        lines.append(f"cut_prefix {conf.cut_prefix}")
        lines.append("")
    if conf.body_dir and conf.body_dir.rstrip("/") != default_dir.rstrip("/"):
        lines.append(f"body_dir {conf.body_dir}")
        lines.append("")
    if conf.ignore:
        for item in conf.ignore:
            lines.append(f"ignore {item}")
        lines.append("")
    for key, sha, body in entries:
        if body:
            lines.append(f"{key} {sha} {body}")
        else:
            lines.append(f"{key} {sha}")
    conf.path.parent.mkdir(parents=True, exist_ok=True)
    conf.path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"captured SHAs into {conf.path}")


def cmd_capture(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)

    out_entries: list[tuple[str, str, str]] = []
    for entry in conf.entries:
        branch_name = entry.branch
        if filtered_mode(conf):
            branch_name = cut_branch_for_entry(conf, entry)
        ref = f"refs/heads/{branch_name}"
        try:
            run(["git", "show-ref", "--verify", "--quiet", ref], cwd=root)
        except subprocess.CalledProcessError:
            die(f"missing local branch: {branch_name} (run: devstack.sh update)")
        sha = git(["rev-parse", "--short=10", f"{branch_name}^{{commit}}"], cwd=root)
        default_body = Path(conf.body_dir) / f"{sanitize_key_to_filename(entry.key)}.md"
        body = entry.body
        if body and Path(body) == default_body:
            body = ""
        out_entries.append((entry.key, sha, body))

    write_conf(root, conf, out_entries)


def cmd_rebase(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)
    current = git(["rev-parse", "--abbrev-ref", "HEAD"], cwd=root)
    if current == "HEAD":
        die("detached HEAD; checkout your stack branch first")
    if current.startswith("pr/"):
        die(f"refusing to rebase while on a PR cut-point branch ({current}); checkout your stack branch")

    if filtered_mode(conf):
        # Ensure the source cut-point refs exist and point into the stack history before rebasing,
        # so `--update-refs` keeps them in sync as commits are rewritten.
        for entry in conf.entries:
            ensure_commit_exists(root, entry.sha)
            cut_branch = cut_branch_for_entry(conf, entry)
            if current == cut_branch:
                die(f"refusing to rebase while on a cut-point branch ({current}); checkout your stack branch")
            run(["git", "branch", "-f", cut_branch, entry.sha], cwd=root)
    else:
        cmd_update(argparse.Namespace())
    upstream = conf.base_remote_ref
    print(f"rebasing {current} onto {upstream} (with --update-refs)")
    run(["git", "rebase", "-i", "--update-refs", upstream], cwd=root)
    cmd_capture(argparse.Namespace())
    if filtered_mode(conf):
        cmd_update(argparse.Namespace())
    print("next: devstack.sh push && devstack.sh gh-sync --apply")


def config_add_ignore(conf_path: Path, ignore_item: str) -> None:
    try:
        lines = conf_path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError as exc:
        die(f"failed to read {conf_path}: {exc}")

    for line in lines:
        s = line.strip()
        if s.startswith("ignore "):
            parts = s.split(maxsplit=1)
            if len(parts) == 2 and parts[1].strip() == ignore_item:
                return

    def is_directive(s: str) -> bool:
        return s.startswith(
            (
                "base ",
                "pr_prefix ",
                "prefix ",
                "body_dir ",
                "cut_prefix ",
                "ignore ",
            )
        )

    insert_at: Optional[int] = None
    for i, line in enumerate(lines):
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        if is_directive(s):
            continue
        insert_at = i
        break

    directive = f"ignore {ignore_item}"
    if insert_at is None:
        lines.append(directive)
    else:
        lines.insert(insert_at, directive)

    conf_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def cmd_ignore(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)
    commitish = args.commitish
    name = (args.name or "").strip()
    no_update = bool(args.no_update)

    full = resolve_commitish(root, commitish)
    short = git(["rev-parse", "--short=10", full], cwd=root)
    if not name:
        subj = git(["show", "-s", "--format=%s", full], cwd=root)
        slug = sanitize_key_to_filename(subj.lower())[:40].strip("-")
        name = f"{short}-{slug}" if slug else short

    base = "stack"
    if conf.pr_prefix:
        p = conf.pr_prefix.rstrip("/")
        if p.startswith("pr/"):
            p = p[3:]
        if p:
            base = p
    ignore_branch = f"ignore/{sanitize_branch_to_conf_name(base)}/{sanitize_key_to_filename(name)}"
    run(["git", "branch", "-f", ignore_branch, full], cwd=root)
    print(f"created {ignore_branch} -> {short}")

    config_add_ignore(conf.path, ignore_branch)
    print(f"added ignore to {conf.path}")

    if not no_update:
        cmd_update(argparse.Namespace())


def cmd_extract(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)
    commitish = args.commitish
    to_branch = args.to_branch
    base = args.base or conf.base_remote_ref
    no_ignore = bool(args.no_ignore)
    force = bool(args.force)
    no_update = bool(args.no_update)

    ensure_clean_worktree(root)
    orig = git(["rev-parse", "--abbrev-ref", "HEAD"], cwd=root)
    if orig == "HEAD":
        die("detached HEAD; checkout your stack branch first")

    full = resolve_commitish(root, commitish)

    try:
        run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{to_branch}"], cwd=root)
        exists = True
    except subprocess.CalledProcessError:
        exists = False
    if exists and not force:
        die(f"branch already exists: {to_branch} (use --force to reset it)")

    try:
        run(["git", "checkout", "-B", to_branch, base], cwd=root)
        run(["git", "cherry-pick", full], cwd=root)
    except subprocess.CalledProcessError:
        run(["git", "cherry-pick", "--abort"], cwd=root, check=False)
        run(["git", "checkout", orig], cwd=root, check=False)
        die("extract failed (cherry-pick conflict); resolve manually and re-run")

    run(["git", "checkout", orig], cwd=root)
    print(f"extracted {git(['rev-parse','--short=10', full], cwd=root)} -> {to_branch}")

    if not no_ignore:
        cmd_ignore(argparse.Namespace(commitish=full, name=args.ignore_name or "", no_update=True))

    if not no_update:
        cmd_update(argparse.Namespace())


def cmd_layer_import(args: argparse.Namespace) -> None:
    root = repo_root()
    ensure_clean_worktree(root)
    conf = read_conf(root)

    layer = int(args.layer)
    if layer < 1 or layer > len(conf.entries):
        die(f"layer must be 1..{len(conf.entries)}")

    entry = conf.entries[layer - 1]
    stack_name = stack_name_from_conf(conf.path)
    key_safe = sanitize_key_to_filename(entry.key)
    default_from_branch = f"layer/{sanitize_branch_to_conf_name(stack_name)}/{key_safe}"

    from_branch = (args.from_branch or default_from_branch).strip()
    if not from_branch:
        die("--from must be non-empty")

    try:
        run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{from_branch}"], cwd=root)
    except subprocess.CalledProcessError:
        die(f"missing source branch: {from_branch} (did you run: ds wt-layer {layer}?)")

    onto = (args.onto or "").strip()
    if onto:
        run(["git", "switch", onto], cwd=root, check=True)
        ensure_clean_worktree(root)

    if getattr(args, "replace", False):
        if args.squash:
            die("--squash cannot be used with --replace (replace uses rebase, not cherry-pick)")

        if filtered_mode(conf):
            die("--replace is not supported in filtered mode yet (use ds rebase, or disable ignore filtering)")

        old_layer_tip = conf.entries[layer - 1].sha
        ensure_commit_exists(root, old_layer_tip)

        prev_ref = conf.base_remote_ref if layer == 1 else conf.entries[layer - 2].sha
        prev_tip = resolve_commitish(root, prev_ref)

        try:
            mb = git(["merge-base", prev_tip, from_branch], cwd=root).strip()
        except subprocess.CalledProcessError:
            die(f"could not compute merge-base between {prev_ref} and {from_branch}")
        if mb != prev_tip.strip():
            die(
                f"{from_branch} does not appear to be based on {prev_ref}; "
                f"merge-base={mb[:12]} expected={prev_tip[:12]}"
            )

        def is_ancestor(a: str, b: str) -> bool:
            proc = run(["git", "merge-base", "--is-ancestor", a, b], cwd=root, check=False)
            return proc.returncode == 0

        if not is_ancestor(prev_tip, old_layer_tip):
            die(
                f"layer {layer} cut point {old_layer_tip[:12]} is not a descendant of previous cut point {prev_tip[:12]}; "
                "stack.conf likely needs `ds capture` after rewriting history"
            )
        if not is_ancestor(old_layer_tip, "HEAD"):
            die(
                f"layer {layer} cut point {old_layer_tip[:12]} is not an ancestor of the current branch; "
                "stack.conf/PR branch tips are out of sync with this branch. Run `ds rebase` (or fix the SHAs), then `ds capture`."
            )

        print(f"layer-import (replace): layer={layer} old-tip={old_layer_tip[:12]} onto={from_branch}")
        print("  action: git rebase --update-refs --onto <from_branch> <old-layer-tip>")
        print("  note: this rewrites history for commits after the layer tip")

        if not args.apply:
            print()
            print("note: dry-run; pass --apply to perform the rebase-based replacement")
            return

        try:
            run(["git", "rebase", "--update-refs", "--onto", from_branch, old_layer_tip], cwd=root, check=True)
        except subprocess.CalledProcessError:
            die(
                "layer-import (replace): rebase stopped due to conflicts.\n\n"
                "next:\n"
                "- Resolve conflicts, then run: `git rebase --continue` (or `git rebase --abort`)\n"
                f"- After the rebase finishes successfully: `git branch -f {entry.branch} {from_branch}`\n"
                "- Then run: `ds capture` (refreshes stack.conf SHAs)\n"
            )

        if current_branch(root) == entry.branch:
            die(f"refusing to move currently checked out branch: {entry.branch}")
        run(["git", "branch", "-f", entry.branch, from_branch], cwd=root, check=True)
        cmd_capture(argparse.Namespace())
        print("layer-import (replace): complete (rebased + updated stack.conf)")

        if getattr(args, "update", False):
            cmd_update(argparse.Namespace())
        return

    base = git(["merge-base", "HEAD", from_branch], cwd=root)
    if not base:
        die(f"could not find merge-base between HEAD and {from_branch}")

    revs = git(["rev-list", "--reverse", "--no-merges", f"{base}..{from_branch}"], cwd=root)
    commits = [c for c in revs.splitlines() if c.strip()]
    if not commits:
        print(f"layer-import: no commits to import from {from_branch}")
        return

    print(f"layer-import: from={from_branch} commits={len(commits)} base={base}")
    for c in commits:
        subj = ""
        try:
            subj = git(["show", "-s", "--format=%s", c], cwd=root)
        except subprocess.CalledProcessError:
            subj = ""
        print(f"  {c[:12]} {subj}")

    if not args.apply:
        print()
        print("note: dry-run; pass --apply to cherry-pick these commits onto the current branch")
        return

    if args.squash:
        run(["git", "cherry-pick", "--no-commit", *commits], cwd=root, check=True)
        msg = (args.message or f"Import layer {layer} changes").strip()
        if not msg:
            msg = f"Import layer {layer} changes"
        run(["git", "commit", "-m", msg], cwd=root, check=True)
        print("layer-import: squashed into one commit")
    else:
        run(["git", "cherry-pick", *commits], cwd=root, check=True)
        print("layer-import: cherry-pick complete")

    if getattr(args, "update", False):
        cmd_update(argparse.Namespace())
