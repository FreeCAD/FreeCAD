from typing import Any

from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Father="DocumentObjectPy",
    Name="RobotObjectPy",
    Twin="RobotObject",
    TwinPointer="RobotObject",
    Include="Mod/Robot/App/RobotObject.h",
    Namespace="Robot",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
)
class RobotObjectPy(DocumentObject):
    """
    Robot document object
    """

    def getRobot(self) -> Any:
        """Returns a copy of the robot. Be aware, the robot behaves the same
        like the robot of the object but is a copy!"""
        ...
