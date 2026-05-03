# pyright: strict

"""Helpers for generating Doxygen XML as C++ API docs input.

The long-term docs plan is to consume Doxygen XML and render native Starlight
pages from a neutral C++ API model. This module owns the Doxygen-specific
preparation so the extractor only needs to read the resulting XML tree.
"""

from __future__ import annotations

from pathlib import Path
import subprocess

CPP_DOXYGEN_INPUT_DIRS = (
    Path("src/Base"),
    Path("src/App"),
    Path("src/Gui"),
    Path("src/Mod/Part/App"),
)
DOXYGEN_CONFIG_TEMPLATE = Path("src/Doc/BuildDevDoc.cfg.in")


def quote_path(path: Path) -> str:
    return f'"{path.as_posix()}"'


def render_doxygen_config(root: Path, out_dir: Path) -> str:
    """Render a Doxygen config tailored for XML-only C++ API extraction."""

    template = (root / DOXYGEN_CONFIG_TEMPLATE).read_text(encoding="utf-8")
    input_list = " \\\n                         ".join(
        quote_path(root / relative_path) for relative_path in CPP_DOXYGEN_INPUT_DIRS
    )
    config = template
    config = config.replace("@DOXYGEN_OUTPUT_DIR@", out_dir.as_posix())
    config = config.replace("@DOXYGEN_INPUT_LIST@", input_list)
    config = config.replace("@DOXYGEN_EXCLUDE_LIST@", "")
    config = config.replace("@DOXYGEN_INCLUDE_PATH@", "")
    config = config.replace("@DOXYGEN_IMAGE_PATH@", "")
    config = config.replace("@DOXYGEN_TAGFILES@", "")
    config = config.replace("@DOXYGEN_LAYOUT_FILE@", "")
    config = config.replace("@HAVE_DOT@", "NO")
    config += "\n"
    config += "GENERATE_HTML           = NO\n"
    config += "GENERATE_QHP            = NO\n"
    config += "GENERATE_XML            = YES\n"
    config += "XML_OUTPUT              = xml\n"
    config += "HAVE_DOT                = NO\n"
    config += "PAPER_TYPE              = a4\n"
    config += "QUIET                   = YES\n"
    return config


def write_doxygen_config(root: Path, out_dir: Path) -> Path:
    """Write the generated Doxygen XML config next to the output tree."""

    out_dir.mkdir(parents=True, exist_ok=True)
    config_path = out_dir / "Doxyfile"
    config_path.write_text(render_doxygen_config(root, out_dir), encoding="utf-8")
    return config_path


def run_doxygen_xml(root: Path, out_dir: Path) -> Path:
    """Generate Doxygen XML and return the resulting XML directory."""

    config_path = write_doxygen_config(root, out_dir)
    subprocess.run(["doxygen", str(config_path)], cwd=root, check=True)
    return out_dir / "xml"
