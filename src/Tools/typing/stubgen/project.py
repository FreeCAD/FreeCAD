from pathlib import Path
from dataclasses import dataclass
from functools import cached_property
import json


@dataclass
class Project:
    root: Path

    @cached_property
    def version(self) -> str:
        version_file = self.root / "version.json"
        with open(version_file, encoding="utf-8") as f:
            data = json.load(f)
        suffix = data.get("version_suffix", "")
        if suffix:
            suffix = f".{suffix}"
        major, minor, patch = (
            data["version_major"],
            data["version_minor"],
            data["version_patch"],
        )
        return f"{major}.{minor}.{patch}{suffix}"

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
        )
