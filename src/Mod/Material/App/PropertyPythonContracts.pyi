# SPDX-License-Identifier: LGPL-2.1-or-later

"""Python conversion metadata for Materials property classes."""

PROPERTY_CPP_NAMESPACE = "Materials"

import Materials


class PropertyMaterial:
    def get(self) -> Materials.Material: ...

    def set(self, value: Materials.Material) -> None: ...
