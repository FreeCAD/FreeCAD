# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import class_declarations, export, sequence_protocol
from PyObjectBase import PyObjectBase
from typing import Any, Final


@export(
    PythonName="Part.ShapeList",
    TwinPointer="ShapeList",
    Include="Mod/Part/App/ShapeList.h",
    Father="PyObjectBase",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
    Namespace="Part",
    Constructor=True,
    Delete=True,
    NumberProtocol=True,
    RichCompare=True,
)
@sequence_protocol(
    sq_length=True,
    sq_concat=True,
    sq_repeat=True,
    sq_item=True,
    mp_subscript=True,
    sq_ass_item=True,
    mp_ass_subscript=True,
    sq_contains=True,
    sq_inplace_concat=True,
    sq_inplace_repeat=False,
)
@class_declarations("""public:
    /// The value this object holds
    const ShapeList &list() const { return *getShapeListPtr(); }
    ShapeList &list() { return *getShapeListPtr(); }
""")
class ShapeList(PyObjectBase):
    """
    Part.ShapeList class.

    A list of sub-elements of a shape. Shape.Faces and its siblings return a
    view that materializes an element only when it is requested. Reading the
    length or one element therefore avoids constructing the whole list.
    Mutating the list materializes it as an independent, copy-on-write value.
    """

    Count: Final[int] = 0
    """Number of elements in the list. Same as len()."""

    ElementType: Final[str] = ""
    """Shape type of the elements, for example 'Face'."""

    IsView: Final[bool] = False
    """Whether the list is still a view of its source shape."""

    Shape: Final[object] = None
    """The source shape for a view, or None after materialization."""

    def copy(self, *args: Any) -> Any:
        """Return a copy of this list sharing storage until written."""
        ...

    def index(self, shape: object, /) -> int:
        """Return the position of a shape, or raise ValueError."""
        ...

    def count(self, shape: object, /) -> int:
        """Return how many entries are the given shape."""
        ...

    def append(self, shape: object, /) -> None:
        """Append one shape, materializing the list first."""
        ...

    def extend(self, iterable: object, /) -> None:
        """Append every shape from an iterable."""
        ...

    def insert(self, index: int, shape: object, /) -> None:
        """Insert one shape before index."""
        ...

    def pop(self, index: int = -1, /) -> object:
        """Remove and return one entry, the last by default."""
        ...

    def remove(self, shape: object, /) -> None:
        """Remove the first matching shape."""
        ...

    def reverse(self, *args: Any) -> None:
        """Reverse the list in place."""
        ...

    def sort(self, *args: Any, **kwargs: Any) -> None:
        """Sort the list in place using the same arguments as list.sort()."""
        ...

    def clear(self, *args: Any) -> None:
        """Drop every entry from the list."""
        ...
