# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from collections.abc import Callable
from typing import Any, TypeVar

"""
This file keeps auxiliary metadata to be used by the Python API stubs.
"""

_ClassT = TypeVar("_ClassT")
_FuncT = TypeVar("_FuncT", bound=Callable[..., Any])

def export(**kwargs: Any) -> Callable[[_ClassT], _ClassT]:
    """
    A decorator to attach metadata to a class.
    """
    ...

def module(**kwargs: Any) -> None:
    """
    Attach metadata to a generated Python extension module surface.
    """
    ...

def constmethod(method: _FuncT, /) -> _FuncT: ...
def no_args(method: _FuncT, /) -> _FuncT: ...
def forward_declarations(source_code: str, /) -> Callable[[_ClassT], _ClassT]:
    """
    A decorator to attach forward declarations to a class.
    """
    ...

def class_declarations(source_code: str, /) -> Callable[[_ClassT], _ClassT]:
    """
    A decorator to attach forward declarations to a class.
    """
    ...

def typing_only(method: _FuncT, /) -> _FuncT:
    """
    Mark a method as typing-only so it is ignored by binding code generation.
    Use class-body if TYPE_CHECKING blocks for typing-only attributes.
    """
    ...

def sequence_protocol(**kwargs: Any) -> Callable[[_ClassT], _ClassT]:
    """
    A decorator to attach sequence protocol metadata to a class.
    """
    ...
