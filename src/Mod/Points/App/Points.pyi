# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final

from Base.Metadata import constmethod, export
from Data import object

@export(
    Father="ComplexGeoDataPy",
    Twin="PointKernel",
    TwinPointer="PointKernel",
    Include="Mod/Points/App/Points.h",
    Namespace="Points",
    FatherInclude="App/ComplexGeoDataPy.h",
    FatherNamespace="Data",
    Constructor=True,
)
class Points(object):
    """
    Points() -- Create an empty points object.

    This class allows one to manipulate the Points object by adding new points, deleting facets, importing from an STL file,
    transforming and much more.

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    License: LGPL-2.1-or-later
    """

    @constmethod
    def copy(self) -> Any:
        """Create a copy of this points object"""
        ...

    def read(self) -> Any:
        """Read in a points object from file."""
        ...

    @constmethod
    def write(self) -> Any:
        """Write the points object into file."""
        ...

    @constmethod
    def writeInventor(self) -> Any:
        """Write the points in OpenInventor format to a string."""
        ...

    def addPoints(self) -> Any:
        """add one or more (list of) points to the object"""
        ...

    @constmethod
    def fromSegment(self) -> Any:
        """Get a new point object from a given segment"""
        ...

    @constmethod
    def fromValid(self) -> Any:
        """Get a new point object from points with valid coordinates (i.e. that are not NaN)"""
        ...
    CountPoints: Final[int]
    """Return the number of vertices of the points object."""

    Points: Final[list]
    """A collection of points
With this attribute it is possible to get access to the points of the object

for p in pnt.Points:
	print p"""
