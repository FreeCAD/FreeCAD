# SPDX-License-Identifier: LGPL-2.1-or-later

"""Python conversion metadata for Part property classes."""

PROPERTY_CPP_NAMESPACE = "Part"

from collections.abc import Sequence

import Part

class PropertyGeometryList:
    def get(self) -> list[Part.Geometry]: ...
    def set(self, value: Part.Geometry | Sequence[Part.Geometry]) -> None: ...

class PropertyPartShape:
    def get(self) -> Part.Shape: ...
    def set(self, value: Part.Shape) -> None: ...

class PropertyTopoShapeList:
    def get(self) -> list[Part.Shape]: ...
    def set(self, value: Part.Shape | Sequence[Part.Shape]) -> None: ...
