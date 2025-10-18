# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Include="Mod/Robot/App/RobotObject.h",
    Namespace="Robot",
)
class RobotObject(DocumentObject):
    """
    Robot document object

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    License: LGPL-2.1-or-later
    """

    def getRobot(self) -> Any:
        """Returns a copy of the robot. Be aware, the robot behaves the same
        like the robot of the object but is a copy!"""
        ...
