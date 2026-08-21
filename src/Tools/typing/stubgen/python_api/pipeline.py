# pyright: strict

"""Coordinate Python API model extraction and documentation outputs."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .extract import extract_curated_api_model
from .markdown import write_api_markdown_docs
from .starlight import write_starlight_sidebar_fragment
from ..class_merge import normalize_api_model_binding_class_headers
from ..source_inputs import collect_binding_classes


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


def generate_python_docs(options: PythonDocsOptions) -> PythonDocsResult:
    """Generate Python API pages and a Starlight sidebar."""

    classes = collect_binding_classes(options.root, options.source_dir)
    model = extract_curated_api_model(
        options.root,
        options.source_dir,
        binding_classes=classes,
    )
    model = normalize_api_model_binding_class_headers(options.root, classes, model)
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
    )
