from typing import Any

from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Include="Mod/Robot/App/RobotObject.h",
    Namespace="Robot",
)
class RobotObject(DocumentObject):
    """
    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    License: LGPL-2.1-or-later
    Robot document object
    """

    def getRobot(self) -> Any:
        """Returns a copy of the robot. Be aware, the robot behaves the same
        like the robot of the object but is a copy!"""
        ...
