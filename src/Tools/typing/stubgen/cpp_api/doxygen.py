# pyright: strict

"""Generate the Doxygen XML consumed by the C++ API documentation pipeline."""

from __future__ import annotations

from pathlib import Path
import subprocess

CPP_DOXYGEN_INPUT_DIRS = (
    Path("src/Base"),
    Path("src/App"),
    Path("src/Gui"),
    Path("src/Mod/Part/App"),
)


def quote_path(path: Path) -> str:
    """Quote a filesystem path for a Doxygen configuration value."""

    return f'"{path.as_posix()}"'


def render_doxygen_config(root: Path, out_dir: Path) -> str:
    """Render a minimal configuration tailored for XML-only API extraction."""

    input_list = " ".join(
        quote_path(root / relative_path) for relative_path in CPP_DOXYGEN_INPUT_DIRS
    )
    values = {
        "PROJECT_NAME": "FreeCAD C++ API",
        "OUTPUT_DIRECTORY": quote_path(out_dir),
        "INPUT": input_list,
        "INCLUDE_PATH": quote_path(root / "src"),
        "FILE_PATTERNS": "*.h *.hh *.hpp *.hxx *.c *.cc *.cpp *.cxx",
        "RECURSIVE": "YES",
        "GENERATE_HTML": "NO",
        "GENERATE_LATEX": "NO",
        "GENERATE_MAN": "NO",
        "GENERATE_RTF": "NO",
        "GENERATE_XML": "YES",
        "XML_OUTPUT": "xml",
        "CREATE_SUBDIRS": "NO",
        "EXTRACT_ALL": "YES",
        "EXTRACT_PRIVATE": "NO",
        "EXTRACT_STATIC": "YES",
        "EXTRACT_LOCAL_CLASSES": "NO",
        "JAVADOC_AUTOBRIEF": "YES",
        "MULTILINE_CPP_IS_BRIEF": "YES",
        "FULL_PATH_NAMES": "YES",
        "BUILTIN_STL_SUPPORT": "YES",
        "MACRO_EXPANSION": "YES",
        "PREDEFINED": "__cplusplus=1",
        "HAVE_DOT": "NO",
        "WARNINGS": "NO",
        "WARN_IF_UNDOCUMENTED": "NO",
        "WARN_IF_DOC_ERROR": "NO",
        "QUIET": "YES",
    }
    return "\n".join(f"{key} = {value}" for key, value in values.items()) + "\n"


def write_doxygen_config(root: Path, out_dir: Path) -> Path:
    """Write the generated Doxygen config next to the output tree."""

    out_dir.mkdir(parents=True, exist_ok=True)
    config_path = out_dir / "Doxyfile"
    config_path.write_text(render_doxygen_config(root, out_dir), encoding="utf-8")
    return config_path


def run_doxygen_xml(root: Path, out_dir: Path) -> Path:
    """Generate Doxygen XML and return the resulting XML directory."""

    config_path = write_doxygen_config(root, out_dir)
    subprocess.run(["doxygen", str(config_path)], cwd=root, check=True)
    return out_dir / "xml"
