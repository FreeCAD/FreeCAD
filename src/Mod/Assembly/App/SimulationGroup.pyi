# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export

from App.DocumentObjectGroup import DocumentObjectGroup

@export(Include="Mod/Assembly/App/SimulationGroup.h", Namespace="Assembly")
class SimulationGroup(DocumentObjectGroup):
    """
    This class is a group subclass for joints.

    Author: Ondsel (development@ondsel.com)
    License: LGPL-2.1-or-later
    """
