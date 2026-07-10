# SPDX-License-Identifier: LGPL-2.1-or-later

"""Command-line interface for the Python API deprecation inventory."""

from __future__ import annotations

import argparse
from dataclasses import asdict
import json
from pathlib import Path
import sys
from typing import Sequence

from .deprecations import RELEASE_RE, _release_key, manifest, scan_repository
from .model import DeprecationRecord, Diagnostic, ScanResult


def _print_diagnostics(diagnostics: Sequence[Diagnostic]) -> None:
    for item in diagnostics:
        print(f"{item.source}:{item.line}: {item.severity}: {item.message}")


def _write_manifest(result: ScanResult, output: str) -> None:
    rendered = json.dumps(manifest(result), indent=2, sort_keys=True) + "\n"
    if output == "-":
        sys.stdout.write(rendered)
    else:
        output_path = Path(output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(rendered, encoding="utf-8")


def _list_records(records: Sequence[DeprecationRecord]) -> None:
    if not records:
        print("No matching deprecations.")
        return
    print(f"{'Symbol':48} {'Deprecated':10} {'Removal':10} Replacement")
    for record in records:
        print(
            f"{record.symbol:48} {record.deprecated_in:10} "
            f"{record.removed_in:10} {record.replacement or '-'}"
        )


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="repository root")
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("check", help="validate deprecation metadata")
    manifest_parser = subparsers.add_parser("manifest", help="write deterministic JSON")
    manifest_parser.add_argument("--output", default="-", help="output path or - for stdout")
    list_parser = subparsers.add_parser("list", help="list deprecations")
    list_parser.add_argument("--remove-by", help="show APIs removed by this release")
    list_parser.add_argument("--format", choices=("table", "json"), default="table")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    result = scan_repository(args.root.resolve())
    errors = [item for item in result.diagnostics if item.severity == "error"]

    if args.command == "check":
        _print_diagnostics(result.diagnostics)
        print(f"Found {len(result.records)} deprecations and {len(errors)} errors.")
    elif args.command == "manifest":
        _write_manifest(result, args.output)
    elif args.command == "list":
        records = list(result.records)
        if args.remove_by:
            if not RELEASE_RE.fullmatch(args.remove_by):
                print(f"invalid release: {args.remove_by}", file=sys.stderr)
                return 2
            target = _release_key(args.remove_by)
            records = [record for record in records if _release_key(record.removed_in) <= target]
        if args.format == "json":
            print(json.dumps([asdict(record) for record in records], indent=2, sort_keys=True))
        else:
            _list_records(records)
    return 1 if errors else 0
