"""Representative static checks for dynamic App::Property value protocols."""

from typing import Protocol, assert_type, cast

import Draft
import FreeCADGui

from ArchTypeHints import (
    ArchBuildingPartObject,
    ArchComponentObject,
    ArchEquipmentObject,
    DocumentObjectList,
    DocumentObjectListInput,
    DocumentObjectSubLinkList,
    DocumentObjectSubLinkListInput,
    FloatList,
    FloatListInput,
    IntegerList,
    IntegerListInput,
    StringList,
    StringListInput,
    VectorInput,
    VectorValue,
)
from FreeCAD import DocumentObject, _QuantityInput as QuantityInput
from FreeCAD.Base import Quantity, Unit, Vector
from ArchWall import _WallObject
from draftobjects.bezcurve import BezCurveObject
from draftobjects.circle import CircleObject
from draftobjects.clone import CloneObject
from draftobjects.rectangle import RectangleObject
from draftobjects.shape2dview import Shape2DViewObject
from draftobjects.text import TextObject
from draftobjects.type_hints import DraftPointListObject
from draftutils.type_hints import CloneObjectLike, DraftAPI

document_object = cast(DocumentObject, object())
draft_api = cast(DraftAPI, Draft)
assert_type(draft_api.get_type(document_object), str | None)
assert_type(draft_api.precision(), int)


class FeatureTestQuantityObject(Protocol):
    """The unconstrained App::PropertyQuantity field in App::FeatureTest."""

    @property
    def QuantityOther(self) -> Quantity: ...

    @QuantityOther.setter
    def QuantityOther(self, value: QuantityInput) -> None: ...


feature_test = cast(FeatureTestQuantityObject, object())
assert_type(feature_test.QuantityOther, Quantity)
feature_test.QuantityOther = Unit("mm")

point_list = cast(DraftPointListObject, object())
assert_type(point_list.ViewObject, FreeCADGui.ViewProviderDocumentObject | None)
assert_type(point_list.AttachmentSupport, DocumentObjectSubLinkList)
point_list.AttachmentSupport = [(document_object, ("Face1",))]
assert_type(point_list.Points, list[Vector])
point_list.Points = [(0, 0, 1)]

clone = cast(CloneObject, object())
assert_type(clone.Objects, DocumentObjectList)
clone.Objects = [document_object]
assert_type(clone.Scale, Vector)
clone.Scale = (1, 1, 1)

clone_like = cast(CloneObjectLike, object())
assert_type(clone_like.CloneOf, DocumentObject | None)

shape_2d_view = cast(Shape2DViewObject, object())
assert_type(shape_2d_view.Projection, Vector)
shape_2d_view.Projection = (0, 0, 1)

text = cast(TextObject, object())
assert_type(text.Text, list[str])
text.Text = ["line"]

bezcurve = cast(BezCurveObject, object())
assert_type(bezcurve.Continuity, list[int])
bezcurve.Continuity = [0, 1]

circle = cast(CircleObject, object())
assert_type(circle.Radius, Quantity)
circle.Radius = "10 mm"

rectangle = cast(RectangleObject, object())
assert_type(rectangle.Length, Quantity)
rectangle.Length = Unit("mm")

wall = cast(_WallObject, object())
assert_type(wall.OverrideWidth, list[float])
wall.OverrideWidth = [0.2, 0.3]
assert_type(wall.Offset, Quantity)
wall.Offset = Unit("mm")

component = cast(ArchComponentObject, object())
assert_type(component.Base, DocumentObject | None)
component.Base = document_object
assert_type(component.HorizontalArea, Quantity)
component.HorizontalArea = "10 m^2"

building_part = cast(ArchBuildingPartObject, object())
assert_type(building_part.Group, DocumentObjectList)
building_part.Group = [document_object]
assert_type(building_part.Height, Quantity)
building_part.Height = "3 m"

equipment = cast(ArchEquipmentObject, object())
assert_type(equipment.SnapPoints, list[Vector])
equipment.SnapPoints = [(0, 0, 1)]
assert_type(equipment.EquipmentPower, float)
equipment.EquipmentPower = 1500.0

# Keep the public aliases exercised as both getter and setter vocabulary.
objects: DocumentObjectList = [document_object]
objects_input: DocumentObjectListInput = [document_object]
sub_links: DocumentObjectSubLinkList = [(document_object, ("Face1",))]
sub_links_input: DocumentObjectSubLinkListInput = sub_links
vector_input: VectorInput = (0, 0, 1)
vectors: VectorValue = Vector()
strings: StringList = ["a"]
strings_input: StringListInput = strings
floats: FloatList = [1.0]
floats_input: FloatListInput = floats
integers: IntegerList = [1]
integers_input: IntegerListInput = integers
_ = (
    objects_input,
    sub_links_input,
    vector_input,
    vectors,
    strings_input,
    floats_input,
    integers_input,
)
