# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.BaseClass import BaseClass


@export(
    Include="Mod/Material/App/MaterialLibrary.h",
    Namespace="Materials",
    Constructor=True,
    Delete=True,
)
class MaterialLibrary(BaseClass):
    """
    Material library.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    Name: str = ...
    """Name of the library"""

    Icon: bytes = ...
    """Icon as an array of bytes."""

    Directory: str = ...
    """Local directory where the library is located. For non-local libraries this will be empty"""

    ReadOnly: bool = ...
    """True if the library is local."""

    Local: bool = ...
    """True if the library is local."""
