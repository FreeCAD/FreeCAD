# pyright: strict

"""Validation for discovery facts that feed the normalized API model."""

from __future__ import annotations

from collections.abc import Mapping, Sequence

from .discovery import group_methods, public_type_targets_for_context
from .model import BindingClass, BindingMethod
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


def validate_discovered_bindings(
    methods: Sequence[BindingMethod],
    type_registrations: Mapping[str, Sequence[str]],
) -> None:
    """Reject malformed discovered names before model construction."""

    errors: list[str] = []
    seen: set[str] = set()

    def add_error(message: str) -> None:
        if message not in seen:
            seen.add(message)
            errors.append(message)

    for method in methods:
        if not valid_identifier(method.python_name):
            add_error(
                f"{method.source}:{method.line} discovered invalid Python method name "
                f"{method.python_name!r}"
            )

    registrations = {key: list(names) for key, names in type_registrations.items()}
    for registration_key in sorted(registrations):
        for public_name in registrations[registration_key]:
            if "." not in public_name:
                add_error(
                    f"registration {registration_key!r} has unsupported public target "
                    f"{public_name!r}"
                )
                continue
            class_symbol = public_name.rsplit(".", 1)[-1]
            if not valid_identifier(class_symbol):
                add_error(
                    f"registration {registration_key!r} has invalid public class symbol "
                    f"{class_symbol!r} in {public_name!r}"
                )

    _, type_methods, _ = group_methods(list(methods))
    for context_name, context_methods in sorted(type_methods.items()):
        for target in public_type_targets_for_context(
            context_name,
            context_methods,
            registrations,
        ):
            source = context_methods[0]
            if not valid_identifier(target.class_symbol):
                add_error(
                    f"{source.source}:{source.line} discovered invalid public class symbol "
                    f"{target.class_symbol!r}"
                )
            if target.variable_symbol and not valid_identifier(target.variable_symbol):
                add_error(
                    f"{source.source}:{source.line} discovered invalid public variable symbol "
                    f"{target.variable_symbol!r}"
                )
            for base_symbol in target.base_symbols:
                if not valid_identifier(base_symbol):
                    add_error(
                        f"{source.source}:{source.line} discovered invalid public base symbol "
                        f"{base_symbol!r}"
                    )

    if errors:
        raise ValueError("invalid discovered Python identifiers:\n  " + "\n  ".join(errors))
