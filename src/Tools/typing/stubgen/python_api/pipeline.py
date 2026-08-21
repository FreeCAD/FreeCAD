# pyright: strict

"""Coordinate Python API model extraction and documentation outputs."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from ..diagnostics import MergeDiagnostics
from ..discovery import collect_methods, collect_type_registrations
from ..parsing import iter_source_files
from ..source_inputs import (
    collect_binding_classes,
    load_stub_signature_overrides,
    supplement_module_methods_from_stub_signatures,
)
from ..class_merge import normalize_api_model_binding_class_headers
from .adapters import merge_discovered_bindings
from .extract import extract_curated_api_model_with_diagnostics
from .markdown import write_api_markdown_docs
from .starlight import write_starlight_sidebar_fragment


@dataclass(frozen=True)
class PythonDocsOptions:
    """Inputs and output locations for one Python API documentation run."""

    root: Path
    source_dir: Path
    out_dir: Path
    source_base_url: str | None
    sidebar_out: Path


@dataclass(frozen=True)
class PythonDocsResult:
    """Outputs produced by one Python API documentation run."""

    page_count: int
    docs_dir: Path
    sidebar_path: Path
    diagnostics: MergeDiagnostics


def generate_python_docs(options: PythonDocsOptions) -> PythonDocsResult:
    """Generate Python API pages and a Starlight sidebar."""

    source_files = list(iter_source_files(options.root, options.source_dir))
    type_registrations = collect_type_registrations(options.root, source_files)
    methods = collect_methods(options.root, options.source_dir)
    methods = supplement_module_methods_from_stub_signatures(
        options.root,
        options.source_dir,
        methods,
    )
    classes = collect_binding_classes(
        options.root,
        options.source_dir,
        type_registrations,
    )
    stub_signature_overrides = load_stub_signature_overrides(
        options.root,
        options.source_dir,
        methods,
        type_registrations,
    )
    model, diagnostic_items = extract_curated_api_model_with_diagnostics(
        options.root,
        options.source_dir,
        binding_classes=classes,
    )
    diagnostics = MergeDiagnostics(diagnostic_items)
    model = normalize_api_model_binding_class_headers(options.root, classes, model)
    model = merge_discovered_bindings(
        model,
        methods,
        type_registrations,
        stub_signature_overrides,
    )
    page_count = write_api_markdown_docs(
        options.out_dir,
        model,
        source_base_url=options.source_base_url,
    )
    sidebar_path = write_starlight_sidebar_fragment(options.sidebar_out, model)
    return PythonDocsResult(
        page_count=page_count,
        docs_dir=options.out_dir,
        sidebar_path=sidebar_path,
        diagnostics=diagnostics,
    )
