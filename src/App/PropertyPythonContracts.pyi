# SPDX-License-Identifier: LGPL-2.1-or-later

"""Python conversion metadata for the core ``App::Property*`` classes.

This file is consumed by ``stubgen`` and is not a runtime API. C++ owns the
property inheritance graph and conversion implementations; this source-side
input records the Python shape at conversion roots and override points.
"""

PROPERTY_CPP_NAMESPACE = "App"

from collections.abc import Sequence
from io import IOBase
from typing import TypeAlias

from FreeCAD import Base
from FreeCAD import DocumentObject

_DocumentObjectSubLinkPairInput: TypeAlias = (
    tuple[DocumentObject, str | Sequence[str]]
    | list[DocumentObject | str | Sequence[str]]
)
_DocumentObjectLink: TypeAlias = DocumentObject | None
_DocumentObjectList: TypeAlias = list[DocumentObject | None]
_DocumentObjectListInput: TypeAlias = Sequence[DocumentObject]
_DocumentObjectSubLinkValue: TypeAlias = tuple[DocumentObject, list[str]] | None
_DocumentObjectSubLinkInput: TypeAlias = (
    DocumentObject | None | _DocumentObjectSubLinkPairInput
)
_DocumentObjectSubLinkListItemInput: TypeAlias = (
    DocumentObject | _DocumentObjectSubLinkPairInput
)
_DocumentObjectSubLinkList: TypeAlias = (
    list[tuple[DocumentObject, tuple[str, ...]]]
)
_DocumentObjectSubLinkListInput: TypeAlias = (
    _DocumentObjectSubLinkInput | Sequence[_DocumentObjectSubLinkListItemInput]
)
_XLinkListInput: TypeAlias = _DocumentObjectListInput | _DocumentObjectSubLinkListInput
_XLinkListValue: TypeAlias = _DocumentObjectList | _DocumentObjectSubLinkList
_QuantityInput: TypeAlias = float | str | Base.Quantity | Base.Unit
_QuantityValueInput: TypeAlias = float | str | Base.Quantity
_IntegerConstraintInput: TypeAlias = (
    int | dict[str, int] | tuple[int, int, int, int]
)
_FloatConstraintInput: TypeAlias = (
    float | int | dict[str, float] | tuple[float, float, float, float]
)
_VectorValue: TypeAlias = Base.Vector
_VectorInput: TypeAlias = Base.Vector | tuple[float, float, float]
_VectorList: TypeAlias = list[Base.Vector]
_VectorListInput: TypeAlias = Sequence[_VectorInput]
_StringList: TypeAlias = list[str]
_StringListInput: TypeAlias = Sequence[str]
_FloatList: TypeAlias = list[float]
_FloatListInput: TypeAlias = Sequence[float]
_IntegerList: TypeAlias = list[int]
_IntegerListInput: TypeAlias = Sequence[int]
_FileInput: TypeAlias = str | bytes | IOBase
_FileIncludedInput: TypeAlias = (
    _FileInput | dict[str, str] | tuple[str | bytes, str | bytes]
)
_ColorValue: TypeAlias = tuple[float, float, float, float]
_ColorInput: TypeAlias = (
    int
    | tuple[float, float, float]
    | tuple[float, float, float, float]
    | tuple[int, int, int]
    | tuple[int, int, int, int]
)
_XLinkValue: TypeAlias = (
    DocumentObject | tuple[DocumentObject, str | list[str]] | None
)
_XLinkInput: TypeAlias = (
    DocumentObject
    | None
    | tuple[DocumentObject, str | Sequence[str]]
    | list[DocumentObject | str | Sequence[str]]
)


class PropertyBool:
    def get(self) -> bool: ...

    def set(self, value: bool | int) -> None: ...


class PropertyInteger:
    def get(self) -> int: ...

    def set(self, value: int) -> None: ...


class PropertyIntegerConstraint:
    def set(self, value: _IntegerConstraintInput) -> None: ...


class PropertyFloat:
    def get(self) -> float: ...

    def set(self, value: float) -> None: ...


class PropertyFloatConstraint:
    def set(self, value: _FloatConstraintInput) -> None: ...


class PropertyString:
    def get(self) -> str: ...

    def set(self, value: str) -> None: ...


class PropertyEnumeration:
    def get(self) -> str: ...

    def set(self, value: str | list[str]) -> None: ...


class PropertyMap:
    def get(self) -> dict[str, str]: ...

    def set(self, value: dict[str, str]) -> None: ...


class PropertyUUID:
    def get(self) -> str: ...

    def set(self, value: str) -> None: ...


class PropertyColor:
    def get(self) -> _ColorValue: ...

    def set(self, value: _ColorInput) -> None: ...


class PropertyExpressionEngine:
    READ_ONLY = True

    def get(self) -> list[tuple[str, str | None]]: ...


class PropertyFile:
    def set(self, value: str | dict[str, str]) -> None: ...


class PropertyFileIncluded:
    def get(self) -> str: ...

    def set(self, value: _FileIncludedInput) -> None: ...


class PropertyPlacement:
    def get(self) -> Base.Placement: ...

    def set(self, value: Base.Placement | Base.Matrix) -> None: ...


class PropertyLink:
    def get(self) -> _DocumentObjectLink: ...

    def set(self, value: _DocumentObjectLink) -> None: ...


class PropertyLinkList:
    def get(self) -> _DocumentObjectList: ...

    def set(self, value: _DocumentObjectListInput) -> None: ...


class PropertyLinkSub:
    def get(self) -> _DocumentObjectSubLinkValue: ...

    def set(self, value: _DocumentObjectSubLinkInput) -> None: ...


class PropertyLinkSubList:
    def get(self) -> _DocumentObjectSubLinkList: ...

    def set(self, value: _DocumentObjectSubLinkListInput) -> None: ...


class PropertyQuantity:
    def get(self) -> Base.Quantity: ...

    def set(self, value: _QuantityInput) -> None: ...


class PropertyQuantityConstraint:
    def set(self, value: _QuantityValueInput) -> None: ...


class PropertyPersistentObject:
    def get(self) -> object: ...


class PropertyVector:
    def get(self) -> _VectorValue: ...

    def set(self, value: _VectorInput) -> None: ...


class PropertyVectorList:
    def get(self) -> _VectorList: ...

    def set(self, value: _VectorListInput) -> None: ...


class PropertyStringList:
    def get(self) -> _StringList: ...

    def set(self, value: _StringListInput) -> None: ...


class PropertyFloatList:
    def get(self) -> _FloatList: ...

    def set(self, value: _FloatListInput) -> None: ...


class PropertyIntegerList:
    def get(self) -> _IntegerList: ...

    def set(self, value: _IntegerListInput) -> None: ...


class PropertyXLink:
    def get(self) -> _XLinkValue: ...

    def set(self, value: _XLinkInput) -> None: ...


class PropertyXLinkList:
    def get(self) -> _XLinkListValue: ...

    def set(self, value: _XLinkListInput) -> None: ...


class PropertyXLinkSub:
    def get(self) -> _DocumentObjectSubLinkValue: ...
