# SPDX-License-Identifier: LGPL-2.1-or-later

"""Static checks for Draft's dynamic property protocols."""

from typing import assert_type, cast

from FreeCAD.Base import Quantity, Unit

from draftobjects.circle import CircleObject
from draftobjects.ellipse import EllipseObject
from draftobjects.polygon import PolygonObject
from draftobjects.rectangle import RectangleObject

circle = cast(CircleObject, object())
ellipse = cast(EllipseObject, object())
polygon = cast(PolygonObject, object())
rectangle = cast(RectangleObject, object())

assert_type(circle.Radius, Quantity)
assert_type(circle.FirstAngle, Quantity)
assert_type(circle.LastAngle, Quantity)
circle.Radius = 10.0
circle.Radius = "10 mm"
circle.Radius = Quantity("10 mm")
circle.FirstAngle = 15.0
circle.FirstAngle = "15 deg"
circle.LastAngle = Quantity("90 deg")

assert_type(ellipse.MajorRadius, Quantity)
assert_type(ellipse.MinorRadius, Quantity)
ellipse.MajorRadius = 20.0
ellipse.MinorRadius = Quantity("5 mm")

assert_type(polygon.Radius, Quantity)
assert_type(polygon.FilletRadius, Quantity)
assert_type(polygon.ChamferSize, Quantity)
polygon.Radius = 25.0
polygon.FilletRadius = Quantity("2 mm")
polygon.ChamferSize = 1.0
polygon.DrawMode = ["inscribed", "circumscribed"]
polygon.DrawMode = "inscribed"

assert_type(rectangle.Length, Quantity)
assert_type(rectangle.Height, Quantity)
assert_type(rectangle.FilletRadius, Quantity)
assert_type(rectangle.ChamferSize, Quantity)
rectangle.Length = 50.0
rectangle.Length = Unit("mm")
rectangle.Height = Quantity("30 mm")
