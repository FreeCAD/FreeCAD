from typing import Any

from Base.BaseClass import BaseClass
from Base.Metadata import export

@export(
    Father="BaseClassPy",
    Name="MeasurementPy",
    Twin="Measurement",
    TwinPointer="Measurement",
    Include="Mod/Measure/App/Measurement.h",
    Namespace="Measure",
    FatherInclude="Base/BaseClassPy.h",
    FatherNamespace="Base",
    Constructor=True,
)
class MeasurementPy(BaseClass):
    """
    Make a measurement
    """

    def addReference3D(self) -> Any:
        """add a geometric reference"""
        ...

    def has3DReferences(self) -> Any:
        """does Measurement have links to 3D geometry"""
        ...

    def clear(self) -> Any:
        """measure the difference between references to obtain resultant vector"""
        ...

    def delta(self) -> Any:
        """measure the difference between references to obtain resultant vector"""
        ...

    def length(self) -> Any:
        """measure the length of the references"""
        ...

    def volume(self) -> Any:
        """measure the volume of the references"""
        ...

    def area(self) -> Any:
        """measure the area of the references"""
        ...

    def lineLineDistance(self) -> Any:
        """measure the line-Line Distance of the references. Returns 0 if references are not 2 lines."""
        ...

    def planePlaneDistance(self) -> Any:
        """measure the plane-plane distance of the references. Returns 0 if references are not 2 planes."""
        ...

    def angle(self) -> Any:
        """measure the angle between two edges"""
        ...

    def radius(self) -> Any:
        """measure the radius of an arc or circle edge"""
        ...

    def com(self) -> Any:
        """measure the center of mass for selected volumes"""
        ...
