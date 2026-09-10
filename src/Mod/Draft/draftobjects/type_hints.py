# SPDX-License-Identifier: LGPL-2.1-or-later

"""Shared typing protocols for Draft object shapes."""

from __future__ import annotations

from typing import TYPE_CHECKING, Protocol

import FreeCAD as App

if TYPE_CHECKING:
    import FreeCADGui as Gui
    from FreeCAD import (
        _DocumentObjectLink as DocumentObjectLink,
        _DocumentObjectList as DocumentObjectList,
        _DocumentObjectListInput as DocumentObjectListInput,
        _DocumentObjectSubLinkList as DocumentObjectSubLinkList,
        _DocumentObjectSubLinkListInput as DocumentObjectSubLinkListInput,
        _IntegerList as IntegerList,
        _IntegerListInput as IntegerListInput,
        _QuantityInput as QuantityInput,
        _QuantityValueInput as QuantityValueInput,
        _StringList as StringList,
        _StringListInput as StringListInput,
        _VectorInput as VectorInput,
        _VectorList as VectorList,
        _VectorListInput as VectorListInput,
        _VectorValue as VectorValue,
    )

if not TYPE_CHECKING:
    QuantityInput = object
    DocumentObjectLink = object
    DocumentObjectList = object
    DocumentObjectListInput = object
    DocumentObjectSubLinkList = object
    DocumentObjectSubLinkListInput = object
    IntegerList = object
    IntegerListInput = object
    VectorInput = object
    QuantityValueInput = object
    StringList = object
    StringListInput = object
    VectorList = object
    VectorListInput = object
    VectorValue = object


class DraftDocumentObject(Protocol):
    """Core object surface shared by typed Draft document protocols."""

    Placement: App.Placement
    ViewObject: Gui.ViewProviderDocumentObject | None


class DraftMakeFaceObject(DraftDocumentObject, Protocol):
    """Draft object with common dynamic properties used by make_* helpers."""

    @property
    def AttachmentSupport(self) -> DocumentObjectSubLinkList: ...

    @AttachmentSupport.setter
    def AttachmentSupport(self, value: DocumentObjectSubLinkListInput) -> None: ...

    MakeFace: bool


class DraftPointListObject(DraftMakeFaceObject, Protocol):
    """Draft object with point-list properties used by curve make_* helpers."""

    Closed: bool

    @property
    def Points(self) -> VectorList: ...

    @Points.setter
    def Points(self, value: VectorListInput) -> None: ...

    def addExtension(self, identifier: str, /) -> None: ...
