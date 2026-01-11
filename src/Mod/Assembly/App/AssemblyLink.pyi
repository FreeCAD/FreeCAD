# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Final

from Base.Metadata import export

from App.Part import Part

@export(
    Include="Mod/Assembly/App/AssemblyLink.h",
    Namespace="Assembly",
)
class AssemblyLink(Part):
    """
    This class handles document objects in Assembly

    Author: Ondsel (development@ondsel.com)
    License: LGPL-2.1-or-later
    """

    Joints: Final[list]
    """A list of all joints this assembly link has."""
