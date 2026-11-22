from pathlib import Path
from dataclasses import dataclass, field
from functools import cached_property
import json
import re

API_VERSION_FILE = "src/Tools/typing/api_version.json"
FREECAD_VERSION_FILE = "version.json"

DEV_SUFFIX = "dev"

SUFFIX_PATTERN = re.compile(r"^([A-Za-z]+)(\d+)$")

# The versions that version.json is allowed to use, mapped to their normalized PEP 440 form so the
# rendered version matches the filename. FreeCAD only ever uses these suffixes (and doesn't always
# use all of them). If we ever change versioning schemes to support other forms of not-quite-a-
# release-yet suffix, add them here.
PRE_RELEASE_SUFFIXES = {
    "a": "a",
    "alpha": "a",
    "b": "b",
    "beta": "b",
    "rc": "rc",
}


@dataclass
class Project:
    root: Path
    version_override: str | None = field(default=None, kw_only=True)
    patch_number: int = field(default=0, kw_only=True)
    dev_build: int | None = field(default=None, kw_only=True)

    @cached_property
    def package_name(self) -> str:
        return self._read_json(API_VERSION_FILE)["package_name"]

    @cached_property
    def api_version(self) -> tuple[int, int]:
        data = self._read_json(API_VERSION_FILE)
        return int(data["api_version_major"]), int(data["api_version_minor"])

    @cached_property
    def freecad_version_suffix(self) -> str:
        return self._read_json(FREECAD_VERSION_FILE).get("version_suffix", "").strip().lstrip(".")

    @cached_property
    def freecad_version(self) -> str:
        """The FreeCAD version the stubs were generated from, recorded for provenance only."""
        data = self._read_json(FREECAD_VERSION_FILE)
        suffix = self.freecad_version_suffix
        version = f"{data['version_major']}.{data['version_minor']}.{data['version_patch']}"
        return f"{version}.{suffix}" if suffix else version

    @cached_property
    def pre_release(self) -> str | None:
        """The normalized PEP 440 pre-release segment implied by version.json, if any. Nothing for
        dev, which is handled through a wholly different code path since it doesn't publish to the
        real PyPI, but to TestPyPI instead.
        """
        suffix = self.freecad_version_suffix
        if not suffix or suffix.lower() == DEV_SUFFIX:
            return None
        match = SUFFIX_PATTERN.match(suffix)
        spelling = PRE_RELEASE_SUFFIXES.get(match[1].lower()) if match else None
        if spelling is None:
            accepted = ", ".join(sorted(PRE_RELEASE_SUFFIXES))
            raise ValueError(
                f"version_suffix {suffix!r} in {FREECAD_VERSION_FILE} is not a version this "
                f"package can be published under. Expected an empty suffix, {DEV_SUFFIX!r}, or "
                f"one of {accepted} followed by a number (for example RC1)."
            )
        return f"{spelling}{int(match[2])}"

    @cached_property
    def version(self) -> str:
        """Compose the PEP 440 package version."""
        if self.version_override is not None:
            return self.version_override
        major, minor = self.api_version
        version = f"{major}.{minor}.{self.patch_number}"
        if self.dev_build is not None:
            return f"{version}.dev{self.dev_build}"
        if self.pre_release is not None:
            return f"{version}{self.pre_release}"
        return version

    def _read_json(self, relative_path: str) -> dict:
        with open(self.root / relative_path, encoding="utf-8") as f:
            return json.load(f)

    def _render_template(self, template: str, out_path: Path, /, **vars) -> None:
        text = Path(__file__).with_name(template).read_text(encoding="utf-8")
        out_path.write_text(
            text.format(**vars),
            encoding="utf-8",
        )

    def write_pyproject(self, out_dir: Path) -> None:
        """Write the package pyproject.toml from a template, filling metadata."""
        self._render_template(
            "PYPROJECT_TEMPLATE.toml",
            out_dir / "pyproject.toml",
            version=self.version,
        )

    def write_readme(self, out_dir: Path) -> None:
        """Write the package README from a template, filling in the version."""
        self._render_template(
            "README_TEMPLATE.md",
            out_dir / "README.md",
            version=self.version,
            freecad_version=self.freecad_version,
        )
