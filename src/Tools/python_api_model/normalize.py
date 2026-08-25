# pyright: strict

"""Normalize source-side type spellings before they enter ``PythonApiModel``."""

from __future__ import annotations

from dataclasses import replace
import re

from .signatures import CallableSignature

SOURCE_TYPE_ALIASES = {
    "AxisPy": "'FreeCAD.Base.Axis'",
    "MatrixPy": "'FreeCAD.Base.Matrix'",
    "RotationPy": "'FreeCAD.Base.Rotation'",
    "UnitPy": "'FreeCAD.Base.Unit'",
}


def normalize_source_type(text: str | None, module_name: str | None = None) -> str | None:
    """Map source-local aliases and self-qualified names to public spellings."""

    if text is None:
        return None
    if module_name:
        text = re.sub(rf"\b{re.escape(module_name)}\.", "", text)
    for source_name, public_name in SOURCE_TYPE_ALIASES.items():
        text = re.sub(rf"\b{re.escape(source_name)}\b", public_name, text)
    return text


def normalize_signature_types(
    signature: CallableSignature,
    module_name: str | None,
) -> CallableSignature:
    """Normalize annotations on one structured callable signature."""

    return replace(
        signature,
        parameters=tuple(
            replace(
                parameter,
                annotation=normalize_source_type(parameter.annotation, module_name),
            )
            for parameter in signature.parameters
        ),
        return_annotation=normalize_source_type(signature.return_annotation, module_name),
    )
