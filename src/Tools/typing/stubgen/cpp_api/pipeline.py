# pyright: strict

"""Coordinate Doxygen extraction and C++ API documentation outputs."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from ..manifest import write_api_manifest
from .doxygen import run_doxygen_xml
from .extract import extract_cpp_api_model
from .markdown import write_cpp_api_markdown_docs
from .starlight import write_cpp_starlight_sidebar_fragment


@dataclass(frozen=True)
class CppDocsOptions:
    """Inputs and output locations for one C++ API documentation run."""

    root: Path
    out_dir: Path
    doxygen_out_dir: Path
    doxygen_xml_dir: Path | None
    run_doxygen: bool
    source_base_url: str | None
    sidebar_out: Path


@dataclass(frozen=True)
class CppDocsResult:
    """Outputs produced by one C++ API documentation run."""

    page_count: int
    xml_dir: Path
    docs_dir: Path
    sidebar_path: Path
    manifest_path: Path


def resolve_xml_dir(options: CppDocsOptions) -> Path:
    """Return the XML input directory, generating it when requested."""

    if options.run_doxygen:
        return run_doxygen_xml(options.root, options.doxygen_out_dir)
    if options.doxygen_xml_dir is not None:
        return options.doxygen_xml_dir
    return options.doxygen_out_dir / "xml"


def generate_cpp_docs(options: CppDocsOptions) -> CppDocsResult:
    """Generate C++ API pages and a Starlight sidebar from Doxygen XML."""

    xml_dir = resolve_xml_dir(options)
    if not xml_dir.exists():
        raise FileNotFoundError(f"Doxygen XML directory does not exist: {xml_dir}")

    model = extract_cpp_api_model(options.root, xml_dir)
    page_count = write_cpp_api_markdown_docs(
        options.out_dir,
        model,
        source_base_url=options.source_base_url,
    )
    manifest_path = options.out_dir / "cpp-api-manifest.json"
    write_api_manifest(
        manifest_path,
        generator="cpp-api",
        pages=page_count,
        counts={
            "classes": len(model.classes),
            "enums": sum(len(namespace.enums) for namespace in model.namespaces)
            + sum(len(klass.enums) for klass in model.classes),
            "functions": sum(len(namespace.functions) for namespace in model.namespaces)
            + sum(
                len(klass.constructors)
                + (1 if klass.destructor is not None else 0)
                + len(klass.methods)
                for klass in model.classes
            ),
            "namespaces": len(model.namespaces),
        },
    )
    sidebar_path = write_cpp_starlight_sidebar_fragment(options.sidebar_out, model)
    return CppDocsResult(
        page_count=page_count,
        xml_dir=xml_dir,
        docs_dir=options.out_dir,
        sidebar_path=sidebar_path,
        manifest_path=manifest_path,
    )
