# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Validation for machine (.fcm) configuration files.

Machine definitions are data, and a large collection of them is distributed
outside this repository (see https://github.com/FreeCAD/Machines).  Their
correctness is defined by the loader in this package, so the validator lives
next to the loader rather than in the repository that stores the files -- a
separate implementation would drift from ``Machine.from_dict`` and reproduce
the problem it is meant to detect.

Findings come in three severities:

``ERROR``
    The definition will not load, or will load into something unusable:
    invalid JSON, an unknown toolhead type, a broken kinematic chain, an
    unresolvable postprocessor, a duplicate display name.

``WARNING``
    The definition loads, but data in the file is being dropped.  This is the
    tier that catches silent behaviour change: a key that the loader no longer
    reads looks exactly like a key that works.

``INFO``
    The model has fields the file does not.  Normal for a file written by an
    older FreeCAD; worth knowing before deciding to re-save.

Use as a library::

    from Machine.models.validate import validate_file, Severity
    findings = validate_file("My_Machine.fcm")

Use from the command line::

    FreeCADCmd path/to/validate.py --pass MACHINES_DIR [options]

``--pass`` is required: FreeCAD otherwise treats trailing arguments as
documents to open.
"""

import json
import pathlib
import re
import sys
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple

from Machine.models.machine import Machine

__all__ = [
    "Severity",
    "Finding",
    "validate_machine",
    "validate_file",
    "validate_paths",
    "format_findings",
    "main",
]


class Severity(Enum):
    """How much a finding matters."""

    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


@dataclass
class Finding:
    """A single validation result."""

    severity: Severity
    code: str
    message: str
    source: Optional[str] = None

    def __str__(self):
        prefix = f"{self.source}: " if self.source else ""
        return f"{prefix}[{self.severity.value}] {self.code}: {self.message}"

    def to_dict(self) -> Dict[str, Any]:
        return {
            "severity": self.severity.value,
            "code": self.code,
            "message": self.message,
            "source": self.source,
        }


@dataclass
class _Report:
    findings: List[Finding] = field(default_factory=list)

    def add(self, severity, code, message, source=None):
        self.findings.append(Finding(severity, code, message, source))

    def error(self, code, message, source=None):
        self.add(Severity.ERROR, code, message, source)

    def warning(self, code, message, source=None):
        self.add(Severity.WARNING, code, message, source)

    def info(self, code, message, source=None):
        self.add(Severity.INFO, code, message, source)


# Paths the loader still reads under an older spelling.  A round trip through
# to_dict() renames them, which would otherwise look like dropped data, so the
# on-disk path is rewritten to its modern equivalent before the comparison.
#
# This table is the fragile part of the drift check and is maintained by hand.
# It becomes unnecessary once from_dict() can report which keys it consumed;
# until then, keep it in step with the compatibility branches in machine.py.
_PATH_REWRITES = [
    # machine.py: machine_data.get("toolheads", machine_data.get("spindles", []))
    (re.compile(r"^machine\.spindles(\.|$)"), r"machine.toolheads\1"),
    # Toolhead.from_dict(): spindle_type / spindle_wait fallbacks
    (re.compile(r"^(machine\.toolheads)\.spindle_type$"), r"\1.toolhead_type"),
    (re.compile(r"^(machine\.toolheads)\.spindle_wait$"), r"\1.toolhead_wait"),
    # Axis joints are accepted as either {"origin": [], "axis": []} or
    # [[origin], [vector]]; both carry the same data.
    (re.compile(r"^(machine\.axes\.[^.]+\.joint)\..+$"), r"\1"),
    # output_header is read from the header subsection or the output level.
    (re.compile(r"^output\.output_header$"), "output.header.output_header"),
]


def _normalize_path(path: str) -> str:
    """Rewrite an on-disk key path to the spelling the current model uses.

    Rewrites are applied in sequence, since one can expose another: a
    ``machine.spindles.spindle_wait`` path first becomes
    ``machine.toolheads.spindle_wait`` and then ``machine.toolheads.toolhead_wait``.
    """
    for pattern, replacement in _PATH_REWRITES:
        path = pattern.sub(replacement, path)
    return path


def _walk_keys(node: Any, prefix: str = "") -> Iterable[Tuple[str, Any]]:
    """Yield (dotted_path, value) for every mapping key beneath *node*.

    List elements collapse onto the same dotted path, so a key present on any
    toolhead is reported once rather than once per index.
    """
    if isinstance(node, dict):
        for key, value in node.items():
            path = f"{prefix}.{key}" if prefix else key
            yield path, value
            yield from _walk_keys(value, path)
    elif isinstance(node, list):
        for item in node:
            yield from _walk_keys(item, prefix)


def _key_paths(node: Any) -> set:
    return {path for path, _ in _walk_keys(node)}


def _check_dropped_data(raw: Dict[str, Any], machine: Machine, report: _Report, source=None):
    """Compare the file against a fresh serialisation of what was loaded.

    Keys only on disk are data the loader ignored.  Keys only in the fresh
    output are fields the model has gained since the file was written.
    """
    try:
        fresh = machine.to_dict()
    except Exception as exc:  # pragma: no cover - defensive
        report.warning("serialise-failed", f"could not re-serialise for comparison: {exc}", source)
        return

    disk_keys = {_normalize_path(k) for k in _key_paths(raw)}
    model_keys = _key_paths(fresh)

    dropped = sorted(disk_keys - model_keys)
    if dropped:
        report.warning(
            "ignored-keys",
            "data in the file that the loader does not read: " + ", ".join(dropped),
            source,
        )

    added = sorted(model_keys - disk_keys)
    if added:
        report.info(
            "newer-model",
            "fields the model adds that the file predates: " + ", ".join(added),
            source,
        )


def _check_postprocessor(machine: Machine, report: _Report, source=None):
    """Check the referenced postprocessor resolves and its property keys are known."""
    name = machine.postprocessor_file_name
    if not name:
        report.error("no-postprocessor", "postprocessor_file_name is empty", source)
        return

    # Imported lazily: the postprocessor factory pulls in a large part of CAM,
    # and the structural checks above should work without it.
    try:
        from Path.Post.Processor import PostProcessorFactory
    except Exception as exc:  # pragma: no cover - defensive
        report.info("no-post-factory", f"postprocessor checks skipped: {exc}", source)
        return

    try:
        post = PostProcessorFactory.get_post_processor(None, name)
    except Exception as exc:
        report.error(
            "postprocessor-missing",
            f"postprocessor '{name}' could not be loaded: {type(exc).__name__}: {exc}",
            source,
        )
        return

    # A postprocessor that cannot be found is returned as a CAMError rather
    # than raised, so a plain try/except is not enough to detect it.  Its
    # message embeds the whole search path, which is too noisy to repeat here.
    if isinstance(post, Exception):
        report.error(
            "postprocessor-missing",
            f"postprocessor '{name}' was not found on the postprocessor search path "
            f"(looked for '{name}_post.py')",
            source,
        )
        return

    try:
        known = {prop["name"] for prop in post.get_full_property_schema()}
    except Exception as exc:  # pragma: no cover - defensive
        report.info("no-schema", f"property schema unavailable for '{name}': {exc}", source)
        return

    unknown = sorted(set(machine.postprocessor_properties) - known)
    if unknown:
        report.warning(
            "unknown-properties",
            f"postprocessor '{name}' has no such properties: " + ", ".join(unknown),
            source,
        )


def validate_machine(
    machine: Machine,
    raw: Optional[Dict[str, Any]] = None,
    source: Optional[str] = None,
    check_postprocessor: bool = True,
) -> List[Finding]:
    """Validate a loaded :class:`Machine`.

    Args:
        machine: the machine to check.
        raw: the dict it was loaded from, if available.  Enables the
            dropped-data comparison, which cannot run without it.
        source: label used in findings, normally a file name.
        check_postprocessor: resolve the referenced postprocessor.  Turn off
            for structural checks that should not import the post factory.

    Returns:
        Findings, most severe first.
    """
    report = _Report()

    if not machine.name or not machine.name.strip():
        report.error("no-name", "machine name is empty", source)

    if not machine.linear_axes and not machine.rotary_axes:
        report.error("no-axes", "machine defines no axes", source)

    if not machine.toolheads:
        report.warning("no-toolheads", "machine defines no toolheads", source)

    for name, axis in machine.linear_axes.items():
        if axis.min_limit >= axis.max_limit:
            report.error(
                "axis-limits",
                f"linear axis {name}: min_limit ({axis.min_limit}) "
                f">= max_limit ({axis.max_limit})",
                source,
            )
    for name, axis in machine.rotary_axes.items():
        if axis.min_limit >= axis.max_limit:
            report.error(
                "axis-limits",
                f"rotary axis {name}: min_limit ({axis.min_limit}) "
                f">= max_limit ({axis.max_limit})",
                source,
            )

    for message in machine.validate_kinematic_chain():
        report.error("kinematics", message, source)

    if check_postprocessor:
        _check_postprocessor(machine, report, source)

    if raw is not None:
        _check_dropped_data(raw, machine, report, source)

    order = {Severity.ERROR: 0, Severity.WARNING: 1, Severity.INFO: 2}
    return sorted(report.findings, key=lambda f: order[f.severity])


def validate_file(path, check_postprocessor: bool = True) -> List[Finding]:
    """Validate a single .fcm file.

    A file that cannot be read, parsed, or loaded produces a single ERROR;
    no further checks are attempted on it.
    """
    path = pathlib.Path(path)
    source = path.name

    try:
        with open(path, "r", encoding="utf-8") as handle:
            raw = json.load(handle)
    except json.JSONDecodeError as exc:
        return [Finding(Severity.ERROR, "invalid-json", str(exc), source)]
    except OSError as exc:
        return [Finding(Severity.ERROR, "unreadable", str(exc), source)]

    if not isinstance(raw, dict):
        return [
            Finding(
                Severity.ERROR, "not-an-object", "top level of the file is not an object", source
            )
        ]

    try:
        machine = Machine.from_dict(raw)
    except Exception as exc:
        return [
            Finding(
                Severity.ERROR,
                "load-failed",
                f"{type(exc).__name__}: {exc}",
                source,
            )
        ]

    return validate_machine(
        machine, raw=raw, source=source, check_postprocessor=check_postprocessor
    )


def _collect_files(paths: Sequence[str]) -> List[pathlib.Path]:
    found: List[pathlib.Path] = []
    for entry in paths:
        p = pathlib.Path(entry)
        if p.is_dir():
            found.extend(sorted(p.rglob("*.fcm")))
        else:
            found.append(p)
    return found


def validate_paths(
    paths: Sequence[str], check_postprocessor: bool = True
) -> Dict[str, List[Finding]]:
    """Validate files and/or directories, plus checks that span the whole set.

    Display-name collisions can only be seen across files, because
    ``MachineFactory.get_machine()`` resolves machines by name.

    Returns:
        Mapping of file path to its findings, in the order the files were found.
    """
    results: Dict[str, List[Finding]] = {}
    names: Dict[str, List[str]] = {}

    for path in _collect_files(paths):
        key = str(path)
        results[key] = validate_file(path, check_postprocessor=check_postprocessor)

        # Only files that loaded contribute a name.
        if not any(f.code in ("invalid-json", "unreadable", "load-failed") for f in results[key]):
            try:
                with open(path, "r", encoding="utf-8") as handle:
                    name = json.load(handle).get("machine", {}).get("name", "")
            except Exception:
                name = ""
            if name:
                names.setdefault(name.lower(), []).append(key)

    for name, holders in names.items():
        if len(holders) > 1:
            for holder in holders:
                others = [h for h in holders if h != holder]
                results[holder].insert(
                    0,
                    Finding(
                        Severity.ERROR,
                        "duplicate-name",
                        f"machine name also used by: {', '.join(others)}",
                        pathlib.Path(holder).name,
                    ),
                )

    return results


def format_findings(results: Dict[str, List[Finding]], verbose: bool = False) -> str:
    """Render results as text.

    Files with nothing to report are listed only when *verbose*.
    """
    lines: List[str] = []
    for path, findings in results.items():
        shown = findings if verbose else [f for f in findings if f.severity is not Severity.INFO]
        if not shown:
            if verbose:
                lines.append(f"ok    {path}")
            continue
        worst = min(
            (f.severity for f in shown),
            key=lambda s: {Severity.ERROR: 0, Severity.WARNING: 1, Severity.INFO: 2}[s],
        )
        lines.append(f"{worst.value.upper():7} {path}")
        for finding in shown:
            lines.append(f"        {finding.severity.value:7} {finding.code}: {finding.message}")
    return "\n".join(lines)


def _script_args(argv: Optional[Sequence[str]]) -> List[str]:
    """Return the arguments intended for this script.

    FreeCAD passes its own command line through in ``sys.argv``; anything
    after ``--pass`` belongs to the script.
    """
    if argv is not None:
        return list(argv)
    if "--pass" in sys.argv:
        return sys.argv[sys.argv.index("--pass") + 1 :]
    return sys.argv[1:]


def main(argv: Optional[Sequence[str]] = None) -> int:
    """Command line entry point.

    Returns:
        0 when nothing worse than a warning was found (or nothing at all with
        ``--strict``), 1 otherwise, 2 when no files were found.
    """
    import argparse

    parser = argparse.ArgumentParser(
        prog="validate.py",
        description="Validate FreeCAD CAM machine (.fcm) definitions.",
    )
    parser.add_argument("paths", nargs="+", help="files or directories to check")
    parser.add_argument("--strict", action="store_true", help="treat warnings as failures")
    parser.add_argument("--json", action="store_true", help="emit JSON instead of text")
    parser.add_argument(
        "--verbose", action="store_true", help="also report info findings and passing files"
    )
    parser.add_argument(
        "--no-postprocessor-check",
        action="store_true",
        help="skip resolving the referenced postprocessor",
    )
    args = parser.parse_args(_script_args(argv))

    results = validate_paths(args.paths, check_postprocessor=not args.no_postprocessor_check)

    if not results:
        print(f"no .fcm files found in: {', '.join(args.paths)}", file=sys.stderr)
        return 2

    if args.json:
        print(
            json.dumps(
                {path: [f.to_dict() for f in findings] for path, findings in results.items()},
                indent=2,
            )
        )
    else:
        text = format_findings(results, verbose=args.verbose)
        if text:
            print(text)

    errors = sum(1 for fs in results.values() for f in fs if f.severity is Severity.ERROR)
    warnings = sum(1 for fs in results.values() for f in fs if f.severity is Severity.WARNING)

    if not args.json:
        print(
            f"\n{len(results)} definition(s) checked, {errors} error(s), {warnings} warning(s)",
            file=sys.stderr,
        )

    if errors:
        return 1
    if warnings and args.strict:
        return 1
    return 0


def _invoked_as_script() -> bool:
    """Whether this module is being run rather than imported.

    FreeCAD executes a script by importing it under a module name derived from
    the file name, so ``__name__`` is never ``"__main__"`` under FreeCADCmd.
    Fall back to comparing the script FreeCAD was asked to run with this file.
    """
    if __name__ == "__main__":
        return True
    try:
        argv0 = sys.argv[1] if len(sys.argv) > 1 else ""
        return bool(argv0) and pathlib.Path(argv0).resolve() == pathlib.Path(__file__).resolve()
    except OSError:
        return False


if _invoked_as_script():
    _exit_code = main()
    # FreeCAD terminates on SystemExit without flushing Python's buffers, and
    # stdout is block-buffered when redirected, so the report would be lost.
    sys.stdout.flush()
    sys.stderr.flush()
    sys.exit(_exit_code)
