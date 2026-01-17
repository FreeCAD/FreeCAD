from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import subprocess
from pathlib import Path

from tools.devstack.commands.stack import cmd_update, pr_base_for_layer, select_entries
from tools.devstack.core.frontmatter import strip_body_frontmatter, title_from_body_frontmatter, title_with_number
from tools.devstack.core.git import ensure_commit_exists, git, repo_root, resolve_commitish, sanitize_key_to_filename
from tools.devstack.core.proc import die, have_cmd, note, run
from tools.devstack.core.stackconf import base_branch_name, filtered_mode, key_number, read_conf, resolved_body_file


def body_file_for_gh(root: Path, entry, body_file: Path) -> Path:
    """Return a body-file path safe for `gh pr create/edit` (frontmatter stripped)."""
    try:
        text = body_file.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return body_file

    stripped = strip_body_frontmatter(text)
    if stripped == text:
        return body_file

    tmp_dir = root / ".devstack" / "tmp" / "gh-sync-bodies"
    tmp_dir.mkdir(parents=True, exist_ok=True)
    out = tmp_dir / f"{sanitize_key_to_filename(entry.key)}.md"
    out.write_text(stripped, encoding="utf-8")
    return out


def shlex_quote(s: str) -> str:
    return shlex.quote(s)


def cmd_push(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)
    remote = os.environ.get("DEVSTACK_STACK_REMOTE", "origin")
    only = getattr(args, "only", None)
    for entry in select_entries(conf, only):
        print(f"pushing {entry.branch}")
        run(["git", "push", "--force-with-lease", remote, f"{entry.branch}:{entry.branch}"], cwd=root)


def cmd_pr_layer(args: argparse.Namespace) -> None:
    layer = int(args.layer)
    cmd_update(argparse.Namespace())
    cmd_push(argparse.Namespace(only=layer))
    cmd_gh_sync(argparse.Namespace(apply=bool(args.apply), only=layer))


def gh_check() -> None:
    if not have_cmd("gh"):
        die("GitHub CLI 'gh' not found on PATH")


def _parse_github_owner_repo(remote_url: str) -> str:
    url = (remote_url or "").strip()
    if not url:
        return ""
    m = re.match(r"^git@github\.com:(?P<owner>[^/]+)/(?P<repo>[^/]+?)(?:\.git)?$", url)
    if m:
        return f"{m.group('owner')}/{m.group('repo')}"
    m = re.match(r"^https?://github\.com/(?P<owner>[^/]+)/(?P<repo>[^/]+?)(?:\.git)?/?$", url)
    if m:
        return f"{m.group('owner')}/{m.group('repo')}"
    return ""


def gh_default_repo(root: Path) -> str:
    env_repo = (os.environ.get("DEVSTACK_GH_REPO", "")).strip()
    if env_repo:
        return env_repo

    candidates: list[str] = []
    remote = os.environ.get("DEVSTACK_STACK_REMOTE", "").strip() or "origin"
    candidates.append(remote)
    for r in ("origin", "upstream", "fork"):
        if r not in candidates:
            candidates.append(r)

    for r in candidates:
        try:
            url = git(["remote", "get-url", r], cwd=root).strip()
        except subprocess.CalledProcessError:
            continue
        repo = _parse_github_owner_repo(url)
        if repo:
            return repo
    return ""


def gh_head_ref(root: Path, *, base_repo: str, branch: str) -> str:
    branch = (branch or "").strip()
    base_repo = (base_repo or "").strip()
    if not base_repo or not branch:
        return branch

    push_remote = (os.environ.get("DEVSTACK_STACK_REMOTE", "") or "origin").strip()
    try:
        url = git(["remote", "get-url", push_remote], cwd=root).strip()
    except subprocess.CalledProcessError:
        return branch

    push_repo = _parse_github_owner_repo(url)
    if not push_repo or push_repo == base_repo:
        return branch

    owner = push_repo.split("/", 1)[0]
    return f"{owner}:{branch}"


def gh_pr_number_for_head(root: Path, head_branch: str, repo: str) -> str:
    repo_args = ["--repo", repo] if repo else []

    def parse_number(stdout: str) -> str:
        try:
            data = json.loads(stdout or "")
            if not data:
                return ""
            return str(data[0].get("number") or "")
        except Exception:
            return ""

    proc = run(
        ["gh", "pr", "list", *repo_args, "--head", head_branch, "--state", "all", "--json", "number", "--limit", "1"],
        capture=True,
        check=False,
    )
    num = parse_number(proc.stdout)
    if num:
        return num

    search_head = head_branch.split(":", 1)[-1]
    proc2 = run(
        [
            "gh",
            "pr",
            "list",
            *repo_args,
            "--search",
            f"head:{search_head}",
            "--state",
            "all",
            "--json",
            "number",
            "--limit",
            "1",
        ],
        capture=True,
        check=False,
    )
    return parse_number(proc2.stdout)


def gh_pr_url(root: Path, pr_number: str, repo: str) -> str:
    pr_number = (pr_number or "").strip()
    if not pr_number:
        return ""
    repo_args = ["--repo", repo] if repo else []
    proc = run(
        ["gh", "pr", "view", pr_number, *repo_args, "--json", "url", "--jq", ".url"],
        cwd=root,
        capture=True,
        check=False,
    )
    if proc.returncode != 0:
        return ""
    return (proc.stdout or "").strip()


def cmd_gh_sync(args: argparse.Namespace) -> None:
    root = repo_root()
    conf = read_conf(root)
    gh_check()

    apply = bool(args.apply)
    if not apply:
        note("dry-run; pass --apply to create/edit PRs")

    repo = gh_default_repo(root)
    repo_args = ["--repo", repo] if repo else []

    base = base_branch_name(conf.base_remote_ref)
    only = getattr(args, "only", None)
    entries = select_entries(conf, only)
    if only is not None:
        base = pr_base_for_layer(conf, only)

    for entry in entries:
        head_commit = entry.sha
        if filtered_mode(conf):
            try:
                head_commit = resolve_commitish(root, entry.branch)
            except Exception:
                die(f"missing local branch: {entry.branch} (run: devstack.sh update)")
        else:
            ensure_commit_exists(root, entry.sha)

        title = git(["show", "-s", "--format=%s", head_commit], cwd=root) or entry.branch
        body_file = resolved_body_file(conf, entry)
        draft_title = title_from_body_frontmatter(body_file) if body_file.is_file() else ""
        if draft_title:
            title = draft_title
        else:
            title = title_with_number(title, key_number(entry.key))
        title = " ".join((title or "").splitlines()).strip()

        head_ref = gh_head_ref(root, base_repo=repo, branch=entry.branch) if repo else entry.branch
        pr_number = gh_pr_number_for_head(root, head_ref, repo)
        if pr_number:
            cmd = ["gh", "pr", "edit", pr_number, *repo_args, "--base", base, "--title", title]
        else:
            cmd = ["gh", "pr", "create", *repo_args, "--head", head_ref, "--base", base, "--title", title]

        if body_file.is_file():
            gh_body_file = body_file_for_gh(root, entry, body_file)
            cmd.extend(["--body-file", str(gh_body_file)])
        else:
            cmd.extend(["--body", f"Stacked PR: {entry.branch} ({entry.sha})"])

        if apply:
            try:
                run(cmd, cwd=root, check=True, capture=True)
            except subprocess.CalledProcessError as exc:
                note("gh-sync failed")
                if not repo:
                    note("hint: set a default repo for gh, or pass it via env:")
                    note("  gh repo set-default OWNER/REPO")
                    note("  export DEVSTACK_GH_REPO=OWNER/REPO")
                out = ((exc.stdout or "") + "\n" + (exc.stderr or "")).strip()
                if "projectCards" in out or "Projects (classic) is being deprecated" in out:
                    ver = ""
                    try:
                        proc = run(["gh", "--version"], cwd=root, check=False, capture=True)
                        ver = (proc.stdout or "").splitlines()[0].strip()
                    except Exception:
                        ver = ""
                    die(
                        "your `gh` CLI is using a deprecated Projects Classic GraphQL field (projectCards); "
                        "upgrade `gh` and retry"
                        + (f" (current: {ver})" if ver else "")
                    )
                raise

            if not pr_number:
                pr_number = gh_pr_number_for_head(root, head_ref, repo)
            url = gh_pr_url(root, pr_number, repo)
            if url:
                print(url)
        else:
            print(" ".join(shlex_quote(x) for x in cmd))
        base = entry.branch

