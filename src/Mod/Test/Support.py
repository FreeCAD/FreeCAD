# SPDX-License-Identifier: LGPL-2.1-or-later

"""State-isolation helpers shared by FreeCAD tests."""

from collections.abc import Iterator
from contextlib import contextmanager
from typing import Any, Literal, TypeAlias

import FreeCAD


_ParameterType: TypeAlias = Literal[
    "Boolean", "Integer", "Unsigned Long", "Float", "String"
]
_ParameterValue: TypeAlias = bool | int | float | str
_ParameterEntry: TypeAlias = tuple[_ParameterType, str, _ParameterValue]

_SETTERS: dict[_ParameterType, str] = {
    "Boolean": "SetBool",
    "Integer": "SetInt",
    "Unsigned Long": "SetUnsigned",
    "Float": "SetFloat",
    "String": "SetString",
}
_REMOVERS: dict[_ParameterType, str] = {
    "Boolean": "RemBool",
    "Integer": "RemInt",
    "Unsigned Long": "RemUnsigned",
    "Float": "RemFloat",
    "String": "RemString",
}


def _type_for(value: _ParameterValue) -> _ParameterType:
    if isinstance(value, bool):
        return "Boolean"
    if isinstance(value, int):
        return "Integer"
    if isinstance(value, float):
        return "Float"
    return "String"


def _set_parameter(
    group: Any,
    name: str,
    value: _ParameterValue,
    value_type: _ParameterType | None = None,
) -> None:
    parameter_type = value_type or _type_for(value)
    getattr(group, _SETTERS[parameter_type])(name, value)


def _remove_parameter(group: Any, name: str) -> None:
    """Remove every typed parameter with this name."""
    for remover in _REMOVERS.values():
        try:
            getattr(group, remover)(name)
        except (AttributeError, RuntimeError):
            pass


@contextmanager
def temporary_preference(
    path: str,
    key: str,
    value: _ParameterValue,
    value_type: _ParameterType | None = None,
) -> Iterator[None]:
    """Temporarily set a parameter and restore all typed values for its key.

    FreeCAD parameters are keyed by both type and name. Snapshotting every
    entry with the requested name prevents a temporary value of another type
    from surviving when the context exits.
    """
    group = FreeCAD.ParamGet(path)
    original: tuple[_ParameterEntry, ...] = tuple(
        entry for entry in (group.GetContents() or []) if entry[1] == key
    )

    try:
        _set_parameter(group, key, value, value_type)
        yield
    finally:
        _remove_parameter(group, key)
        for parameter_type, name, old_value in original:
            _set_parameter(group, name, old_value, parameter_type)
