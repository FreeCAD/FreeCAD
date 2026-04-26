# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Sync version and project name from version.json to all packaging/build files.
#
# Files updated by this script:
#   - pixi.toml (workspace version)
#   - package/rattler-build/pixi.toml (package version, name, description)
#   - package/rattler-build/recipe.yaml (context version, package name)
#   - package/WindowsInstaller/include/declarations.nsh (APP_NAME)
#   - package/fedora/freecad.spec (Version, Name)
#
# Usage:
#   python src/Tools/sync_version.py --check    # verify consistency (no changes)
#   python src/Tools/sync_version.py --update   # update all files
#
# This script is intended to be run from the repository root directory.

import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass
class VersionInfo:
    """Version and project identity parsed from version.json.

    Provides the version in several formats needed by different packaging systems.
    """

    name: str
    major: int
    minor: int
    patch: int
    suffix: str
    build: int

    @classmethod
    def from_json(cls, repo_root: Path) -> "VersionInfo":
        """Load and parse the version.json file.

        repo_root: path to the repository root containing version.json.
        """
        version_file = repo_root / "version.json"
        with open(version_file, encoding="utf-8") as f:
            data = json.load(f)
        return cls(
            name=data["name"],
            major=data["version_major"],
            minor=data["version_minor"],
            patch=data["version_patch"],
            suffix=data["version_suffix"],
            build=data["build_version"],
        )

    @property
    def simple(self) -> str:
        """Version without suffix, e.g. "1.2.0"."""
        return f"{self.major}.{self.minor}.{self.patch}"

    @property
    def complete(self) -> str:
        """Version with suffix after "-" per SemVer convention, e.g. "1.2.0-dev"."""
        return f"{self.simple}-{self.suffix}" if self.suffix else self.simple

    @property
    def rpm(self) -> str:
        """Version with suffix after "~" per RPM convention, e.g. "1.2.0~dev"."""
        return f"{self.simple}~{self.suffix}" if self.suffix else self.simple

    @property
    def conda(self) -> str:
        """Version with suffix appended directly per conda convention, e.g. "1.2.0dev"."""
        return f"{self.simple}{self.suffix}" if self.suffix else self.simple

    @property
    def lowercase_name(self) -> str:
        """Lowercased project name, e.g. "freecad"."""
        return self.name.lower()


def replace_in_toml_section(content: str, section: str, key: str, value: str) -> str:
    """Replace a quoted field value within a specific TOML section.

    Finds the given section header (e.g. "[workspace]"), then within that section
    replaces the value of the given key. Only matches quoted string values.

    content: the full file text.
    section: the section header to match, e.g. "[workspace]".
    key: the TOML key name, e.g. "version".
    value: the new value to set (will be quoted in the output).

    Returns the updated content string (unchanged if no match was found).
    """
    section_pattern = re.escape(section)
    section_match = re.search(section_pattern, content)
    if not section_match:
        return content

    section_start = section_match.start()
    next_section = re.search(r"\n\[", content[section_match.end() :])
    section_end = section_match.end() + next_section.start() if next_section else len(content)

    section_text = content[section_start:section_end]
    field_pattern = rf'({re.escape(key)}\s*=\s*)"[^"]*"'
    new_section = re.sub(field_pattern, rf'\g<1>"{value}"', section_text)
    return content[:section_start] + new_section + content[section_end:]


def sync_workspace_pixi_toml(filepath: Path, version: VersionInfo) -> tuple[str, bool]:
    """Sync the workspace pixi.toml version field.

    Updates the version under the [workspace] section.
    """
    content = filepath.read_text(encoding="utf-8")
    updated = replace_in_toml_section(content, "[workspace]", "version", version.simple)
    return updated, updated != content


def sync_rattler_build_pixi_toml(filepath: Path, version: VersionInfo) -> tuple[str, bool]:
    """Sync the rattler-build pixi.toml version, name, and description fields.

    Updates fields under the [package] section.
    """
    content = filepath.read_text(encoding="utf-8")
    updated = content
    updated = replace_in_toml_section(updated, "[package]", "version", version.conda)
    updated = replace_in_toml_section(updated, "[package]", "name", version.lowercase_name)
    updated = replace_in_toml_section(updated, "[package]", "description", version.name)
    return updated, updated != content


def sync_recipe_yaml(filepath: Path, version: VersionInfo) -> tuple[str, bool]:
    """Sync the rattler-build recipe.yaml version and package name.

    Updates:
      - context.version (quoted value)
      - package.name (unquoted value)
    """
    content = filepath.read_text(encoding="utf-8")
    updated = content

    # context:\n  version: "1.2.0dev"
    updated = re.sub(
        r'(context:\s*\n\s*version:\s*)"[^"]*"',
        rf'\g<1>"{version.conda}"',
        updated,
    )

    # package:\n  name: freecad
    updated = re.sub(
        r"(package:\s*\n\s*name:\s*)\S+",
        rf"\g<1>{version.lowercase_name}",
        updated,
    )

    return updated, updated != content


def sync_declarations_nsh(filepath: Path, version: VersionInfo) -> tuple[str, bool]:
    """Sync the NSIS installer APP_NAME define.

    Updates: !define APP_NAME "FreeCAD"
    """
    content = filepath.read_text(encoding="utf-8")
    updated = re.sub(
        r'(!define APP_NAME\s+)"[^"]*"',
        rf'\g<1>"{version.name}"',
        content,
    )
    return updated, updated != content


def sync_fedora_spec(filepath: Path, version: VersionInfo) -> tuple[str, bool]:
    """Sync the Fedora RPM spec file Name and Version fields.

    Updates:
      - Name: freecad
      - Version: 1.2.0~dev (using RPM "~" convention for pre-release suffixes)
    """
    content = filepath.read_text(encoding="utf-8")
    updated = content

    # Name:           freecad
    updated = re.sub(r"(Name:\s+)\S+", rf"\g<1>{version.lowercase_name}", updated)

    # Version:        1.2.0~dev
    updated = re.sub(r"(Version:\s+)\S+", rf"\g<1>{version.rpm}", updated)

    return updated, updated != content


# Each entry is (relative_path, sync_function).
SYNC_TARGETS = [
    ("pixi.toml", sync_workspace_pixi_toml),
    ("package/rattler-build/pixi.toml", sync_rattler_build_pixi_toml),
    ("package/rattler-build/recipe.yaml", sync_recipe_yaml),
    ("package/WindowsInstaller/include/declarations.nsh", sync_declarations_nsh),
    ("package/fedora/freecad.spec", sync_fedora_spec),
]


def run(repo_root: Path, check_only: bool) -> bool:
    """Process all sync targets, either checking or applying updates.

    repo_root: path to the repository root.
    check_only: if True, only report out-of-sync files without modifying them;
        if False, write updated content to each file.

    Returns True if all files were already in sync, False otherwise.
    """
    version = VersionInfo.from_json(repo_root)
    all_synced = True

    for relative_path, sync_function in SYNC_TARGETS:
        filepath = repo_root / relative_path
        if not filepath.exists():
            print(f"  SKIP: {relative_path} (file not found)")
            continue

        new_content, changed = sync_function(filepath, version)

        if changed:
            all_synced = False
            if check_only:
                print(f"  OUT OF SYNC: {relative_path}")
            else:
                filepath.write_text(new_content, encoding="utf-8")
                print(f"  UPDATED: {relative_path}")
        else:
            print(f"  OK: {relative_path}")

    return all_synced


def main() -> None:
    if len(sys.argv) != 2 or sys.argv[1] not in ("--check", "--update"):
        print(f"Usage: {sys.argv[0]} --check|--update")
        print("  --check   Verify all files match version.json (no changes)")
        print("  --update  Update all files to match version.json")
        sys.exit(2)

    check_only = sys.argv[1] == "--check"
    repo_root = Path(__file__).resolve().parent.parent.parent

    version = VersionInfo.from_json(repo_root)
    print(f"version.json: {version.name} {version.complete}")
    print()

    all_synced = run(repo_root, check_only)

    if not all_synced:
        if check_only:
            print()
            print("Files are out of sync. Run with --update to fix.")
            sys.exit(1)
        else:
            print()
            print("All files updated.")
    else:
        print()
        print("All files are in sync.")


if __name__ == "__main__":
    main()
