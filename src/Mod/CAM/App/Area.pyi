# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Any, Final

from Base.BaseClass import BaseClass
from Base.Metadata import export

@export(
    Include="Mod/CAM/App/Area.h",
    Namespace="Path",
    Constructor=True,
    Delete=True,
)
class Area(BaseClass):
    """
    FreeCAD python wrapper of libarea

    Path.Area(key=value ...)

    The constructor accepts the same parameters as setParams(...) to configure the object
    All arguments are optional.

    Author: Zheng, Lei (realthunder.dev@gmail.com)
    License: LGPL-2.1-or-later
    """

    def add(self, **kwargs) -> Any:
        """"""
        ...

    def setPlane(self) -> None:
        """
        Set the working plane.

        The supplied shape does not need to be planar. Area will try to find planar
        sub-shape (face, wire or edge). If more than one planar sub-shape is found, it
        will prefer the top plane parallel to XY0 plane. If no working plane are set,
        Area will try to find a working plane from the added children shape using the
        same algorithm
        """
        ...

    def getShape(self, **kwargs) -> Any:
        """
        Return the resulting shape

        * index (-1): the index of the section. -1 means all sections. No effect on planar shape.
        * rebuild: clean the internal cache and rebuild
        """
        ...

    def makeOffset(self, **kwargs) -> Any:
        """Make an offset of the shape."""
        ...

    def makePocket(self, **kwargs) -> Any:
        """Generate pocket toolpath of the shape."""
        ...

    def makeSections(self, **kwargs) -> Any:
        """Make a list of area holding the sectioned children shapes on given heights."""
        ...

    def getClearedArea(self) -> Any:
        """Gets the area cleared when a tool of the specified diameter follows the gcode represented in the path, ignoring cleared space above zmax and path segments that don't affect space within the x/y space of bbox."""
        ...

    def getRestArea(self) -> Any:
        """Rest machining: Gets the area left to be machined, assuming some of this area has already been cleared by previous tool paths."""
        ...

    def toTopoShape(self) -> Any:
        """Convert the Area object to a TopoShape."""
        ...

    def setParams(self, **kwargs) -> Any:
        """Set algorithm parameters."""
        ...

    def setDefaultParams(self, **kwargs) -> Any:
        """Static method to set the default parameters of all following Path.Area, plus the following additional parameters."""
        ...

    def getDefaultParams(self) -> Any:
        """Static method to return the current default parameters."""
        ...

    def getParamsDesc(self, **kwargs) -> Any:
        """Returns a list of supported parameters and their descriptions."""
        ...

    def getParams(self) -> Any:
        """Get current algorithm parameters as a dictionary."""
        ...

    def abort(self, **kwargs) -> Any:
        """Abort the current operation."""
        ...
    Sections: Final[list]
    """List of sections in this area."""

    Workplane: Any
    """The current workplane. If no plane is set, it is derived from the added shapes."""

    Shapes: Final[list]
    """A list of tuple: [(shape,op), ...] containing the added shapes together with their operation code"""
