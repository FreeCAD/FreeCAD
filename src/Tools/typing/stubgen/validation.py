# pyright: strict

"""Validation for discovery facts that feed the normalized API model."""

from __future__ import annotations

from .model import BindingClass
from .naming import valid_identifier


def validate_public_class_aliases(classes: list[BindingClass]) -> None:
    """Reject binding classes whose public export targets are ambiguous."""

    errors: list[str] = []
    for klass in classes:
        public_names = list(dict.fromkeys(klass.public_names))
        if len(public_names) < 2:
            continue
        if any("." not in public_name for public_name in public_names):
            errors.append(
                f"{klass.source}:{klass.line} {klass.class_name} has unsupported public names: "
                + ", ".join(public_names)
            )
            continue
        if any(
            not valid_identifier(public_name.rsplit(".", 1)[-1]) for public_name in public_names
        ):
            errors.append(
                f"{klass.source}:{klass.line} {klass.class_name} has an invalid public symbol"
            )
    if errors:
        raise ValueError("invalid multi-public class alias plan:\n  " + "\n  ".join(errors))
