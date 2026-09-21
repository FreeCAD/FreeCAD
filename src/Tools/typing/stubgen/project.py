from pathlib import Path
from dataclasses import dataclass, field
from functools import cached_property
import json

API_VERSION_FILE = "src/Tools/typing/api_version.json"
FREECAD_VERSION_FILE = "version.json"


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
        return data["api_version_major"], data["api_version_minor"]

    @cached_property
    def freecad_version(self) -> str:
        """The FreeCAD version the stubs were generated from, recorded for provenance only."""
        data = self._read_json(FREECAD_VERSION_FILE)
        suffix = data.get("version_suffix", "").strip().lstrip(".")
        version = f"{data['version_major']}.{data['version_minor']}.{data['version_patch']}"
        return f"{version}.{suffix}" if suffix else version

    @cached_property
    def version(self) -> str:
        """Compose the PEP 440 package version.

        The major and minor come from api_version.json; the patch is assigned from the package
        index at publish time, so it counts published releases rather than build attempts.
        """
        if self.version_override is not None:
            return self.version_override
        major, minor = self.api_version
        version = f"{major}.{minor}.{self.patch_number}"
        if self.dev_build is not None:
            return f"{version}.dev{self.dev_build}"
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
