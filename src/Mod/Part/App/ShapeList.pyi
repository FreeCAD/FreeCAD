# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import class_declarations, export, sequence_protocol, typing_only
from PyObjectBase import PyObjectBase
from TopoShape import TopoShape
from typing import Any, Final, Iterable, Optional, SupportsIndex

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
    sq_inplace_repeat=True,
)
@class_declarations("""public:
    /// The value this object holds
    const ShapeList &list() const { return *getShapeListPtr(); }
    ShapeList &list() { return *getShapeListPtr(); }

    /// Return the cached Python wrapper for one element, creating it lazily.
    Py::Object elementObject(int index);
    Py::Object elementObjectUnchecked(int index);
    Py::Object elementObjectFromValue(int index, const TopoShape &value);

    /// Return one value, preferring a changed Python wrapper when present.
    TopoShape effectiveElement(int index) const;
    TopoShape effectiveElementUnchecked(int index) const;

    /// Return all values using the C++ bulk path plus Python overrides.
    std::vector<TopoShape> effectiveValues() const;

    /// Materialise the Python list used after the first structural mutation.
    void materialisePythonList();
    bool isMaterialised() const { return _materialised != nullptr; }
    PyObject *materialisedList() const
    {
        return _materialised ? _materialised->ptr() : nullptr;
    }

    /// Return the current Python-visible number of elements.
    int size() const;

private:
    void overlayCachedElements(std::vector<TopoShape> &values) const;
    std::unordered_map<int, std::shared_ptr<Py::Object>> _touched;
    std::unique_ptr<Py::List> _materialised;
""")
class ShapeList(PyObjectBase):
    """
    Part.ShapeList class.

    A lazy list-like view of sub-elements of a shape. Reading the length or
    one element avoids constructing the whole list, and repeated access to a
    retained view returns the same Python wrapper.

    Slices, copy(), concatenation, and repetition return ordinary Python
    lists. The first structural mutation materializes an ordinary Python list
    internally; that list is then authoritative for subsequent access and for
    conversion back to C++. Mutating methods accept only TopoShape objects.
    """

    Count: Final[int] = 0
    """Number of elements in the list. Same as len()."""

    ElementType: Final[str] = ""
    """Shape type of the elements, or 'Shape' for mixed/unknown values."""

    IsView: Final[bool] = False
    """Whether the list is still a view of its source shape."""

    Shape: Final[Optional[TopoShape]] = None
    """The source shape for a view, or None after materialization."""

    def copy(self) -> list[TopoShape]:
        """Return a normal Python list containing the current elements."""
        ...

    @typing_only
    def __add__(self, other: list[TopoShape], /) -> list[TopoShape]: ...
    @typing_only
    def __radd__(self, other: list[TopoShape], /) -> list[TopoShape]: ...
    @typing_only
    def __mul__(self, count: SupportsIndex, /) -> list[TopoShape]: ...
    @typing_only
    def __rmul__(self, count: SupportsIndex, /) -> list[TopoShape]: ...
    @typing_only
    def __iadd__(self, other: Iterable[TopoShape], /) -> ShapeList: ...
    @typing_only
    def __imul__(self, count: SupportsIndex, /) -> ShapeList: ...
    def index(
        self,
        value: object,
        start: SupportsIndex = 0,
        stop: SupportsIndex = ...,
        /,
    ) -> int:
        """Return the first position of value in the requested range."""
        ...

    def count(self, value: object, /) -> int:
        """Return how many entries equal value."""
        ...

    def append(self, shape: TopoShape, /) -> None:
        """Append one shape, materializing the list first."""
        ...

    def extend(self, iterable: Iterable[TopoShape], /) -> None:
        """Append every shape from an iterable."""
        ...

    def insert(self, index: int, shape: TopoShape, /) -> None:
        """Insert one shape before index."""
        ...

    def pop(self, index: int = -1, /) -> TopoShape:
        """Remove and return one entry, the last by default."""
        ...

    def remove(self, value: object, /) -> None:
        """Remove the first matching value, or raise ValueError."""
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
