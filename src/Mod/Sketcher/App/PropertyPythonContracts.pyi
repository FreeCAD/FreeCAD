# SPDX-License-Identifier: LGPL-2.1-or-later

"""Python conversion metadata for Sketcher property classes."""

PROPERTY_CPP_NAMESPACE = "Sketcher"

import Sketcher

class PropertyConstraintList:
    def get(self) -> list[Sketcher.Constraint]: ...
    def set(self, value: Sketcher.Constraint | list[Sketcher.Constraint]) -> None: ...
