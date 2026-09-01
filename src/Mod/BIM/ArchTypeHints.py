# SPDX-License-Identifier: LGPL-2.1-or-later

"""Static protocols for the Python-facing Arch object boundary."""

from __future__ import annotations

from typing import Any, TYPE_CHECKING, Protocol, Sequence

import FreeCAD as App
from FreeCAD import Vector

if TYPE_CHECKING:
    import FreeCADGui as Gui
    import Part
    from FreeCAD import (
        _DocumentObjectLink as DocumentObjectLink,
        _DocumentObjectList as DocumentObjectList,
        _DocumentObjectListInput as DocumentObjectListInput,
        _DocumentObjectSubLinkInput as DocumentObjectSubLinkInput,
        _DocumentObjectSubLinkList as DocumentObjectSubLinkList,
        _DocumentObjectSubLinkListInput as DocumentObjectSubLinkListInput,
        _DocumentObjectSubLinkValue as DocumentObjectSubLinkValue,
        _FloatList as FloatList,
        _FloatListInput as FloatListInput,
        _IntegerConstraintInput as IntegerConstraintInput,
        _IntegerList as IntegerList,
        _IntegerListInput as IntegerListInput,
        _QuantityInput as QuantityInput,
        _QuantityValueInput as QuantityValueInput,
        _StringList as StringList,
        _StringListInput as StringListInput,
        _VectorInput as VectorInput,
        _VectorValue as VectorValue,
    )
    from FreeCAD.Base import Quantity
else:
    DocumentObjectLink = object
    DocumentObjectList = object
    DocumentObjectListInput = object
    DocumentObjectSubLinkInput = object
    DocumentObjectSubLinkList = object
    DocumentObjectSubLinkListInput = object
    DocumentObjectSubLinkValue = object
    FloatList = object
    FloatListInput = object
    IntegerConstraintInput = object
    IntegerList = object
    IntegerListInput = object
    QuantityInput = object
    QuantityValueInput = object
    StringList = object
    StringListInput = object
    VectorInput = object
    VectorValue = object


class ArchComponentObject(Protocol):
    """Common document properties installed by ``ArchComponent.Component``."""

    @property
    def Base(self) -> DocumentObjectLink: ...

    @Base.setter
    def Base(self, value: DocumentObjectLink) -> None: ...

    @property
    def CloneOf(self) -> DocumentObjectLink: ...

    @CloneOf.setter
    def CloneOf(self, value: DocumentObjectLink) -> None: ...

    @property
    def Additions(self) -> DocumentObjectList: ...

    @Additions.setter
    def Additions(self, value: DocumentObjectListInput) -> None: ...

    @property
    def Subtractions(self) -> DocumentObjectList: ...

    @Subtractions.setter
    def Subtractions(self, value: DocumentObjectListInput) -> None: ...

    Description: str
    Tag: str
    StandardCode: str

    @property
    def Material(self) -> DocumentObjectLink: ...

    @Material.setter
    def Material(self, value: DocumentObjectLink) -> None: ...

    MoveBase: bool
    MoveWithHost: bool

    @property
    def VerticalArea(self) -> Quantity: ...

    @VerticalArea.setter
    def VerticalArea(self, value: QuantityValueInput) -> None: ...

    @property
    def HorizontalArea(self) -> Quantity: ...

    @HorizontalArea.setter
    def HorizontalArea(self, value: QuantityValueInput) -> None: ...

    @property
    def PerimeterLength(self) -> Quantity: ...

    @PerimeterLength.setter
    def PerimeterLength(self, value: QuantityValueInput) -> None: ...

    @property
    def HiRes(self) -> DocumentObjectLink: ...

    @HiRes.setter
    def HiRes(self, value: DocumentObjectLink) -> None: ...

    @property
    def Axis(self) -> DocumentObjectLink: ...

    @Axis.setter
    def Axis(self, value: DocumentObjectLink) -> None: ...

    ViewObject: Gui.ViewProviderDocumentObject | None


class ArchEquipmentObject(ArchComponentObject, Protocol):
    """Document object surface exposed by ``Arch.makeEquipment``."""

    PropertiesList: Sequence[str]
    Shape: Part.Shape
    Placement: App.Placement
    Proxy: Any
    Label: str
    Name: str
    IfcType: str

    Model: str
    ProductURL: str
    StandardCode: str

    @property
    def SnapPoints(self) -> list[Vector]: ...

    @SnapPoints.setter
    def SnapPoints(self, value: Sequence[VectorInput]) -> None: ...

    EquipmentPower: float

    def addProperty(self, *args: Any, **kwargs: Any) -> Any: ...
    def setEditorMode(self, name: str, mode: int | list[str], /) -> None: ...


class ArchBuildingPartObject(Protocol):
    """Document object shape exposed by ``Arch.makeBuildingPart``."""

    Label: str
    IfcType: str

    @property
    def Height(self) -> Quantity: ...

    @Height.setter
    def Height(self, value: QuantityValueInput) -> None: ...

    HeightPropagate: bool

    @property
    def LevelOffset(self) -> Quantity: ...

    @LevelOffset.setter
    def LevelOffset(self, value: QuantityInput) -> None: ...

    @property
    def Area(self) -> Quantity: ...

    @Area.setter
    def Area(self, value: QuantityValueInput) -> None: ...

    Description: str
    Tag: str
    OnlySolids: bool

    @property
    def Group(self) -> DocumentObjectList: ...

    @Group.setter
    def Group(self, value: DocumentObjectListInput) -> None: ...

    ViewObject: Gui.ViewProviderDocumentObject | None

    def addObject(self, obj: App.DocumentObject, /) -> object: ...

    def addObjects(self, objects: Sequence[App.DocumentObject], /) -> object: ...


class ArchSectionPlaneViewObject(Protocol):
    """View properties used while initializing an Arch section plane."""

    @property
    def DisplayLength(self) -> Quantity: ...

    @DisplayLength.setter
    def DisplayLength(self, value: QuantityValueInput) -> None: ...

    @property
    def DisplayHeight(self) -> Quantity: ...

    @DisplayHeight.setter
    def DisplayHeight(self, value: QuantityValueInput) -> None: ...


class ArchSectionPlaneObject(Protocol):
    """Document object shape exposed by ``Arch.makeSectionPlane``."""

    @property
    def Objects(self) -> DocumentObjectList: ...

    @Objects.setter
    def Objects(self, value: DocumentObjectListInput) -> None: ...

    OnlySolids: bool
    Clip: bool
    UseMaterialColorForFill: bool

    @property
    def Depth(self) -> Quantity: ...

    @Depth.setter
    def Depth(self, value: QuantityValueInput) -> None: ...

    Placement: App.Placement
    ViewObject: ArchSectionPlaneViewObject | None
