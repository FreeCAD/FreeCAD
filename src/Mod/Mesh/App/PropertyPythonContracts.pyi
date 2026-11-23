# SPDX-License-Identifier: LGPL-2.1-or-later

"""Python conversion metadata for Mesh property classes."""

PROPERTY_CPP_NAMESPACE = "Mesh"

import Mesh

class PropertyMeshKernel:
    def get(self) -> Mesh.Mesh: ...
    def set(self, value: Mesh.Mesh | list[list[float]]) -> None: ...
