# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from typing import Any, Final, Tuple, Dict

@export(
    Constructor=True,
    Delete=True,
)
class AxisOrigin(BaseClass):
    """
    Gui.AxisOrigin class.

    Class for creating a Coin3D representation of a coordinate system.

    Author: Zheng, Lei (realthunder.dev@gmail.com)
    Licence: LGPL
    """

    @constmethod
    def getElementPicked(self, pickedPoint: Any, /) -> str:
        """
        Returns the picked element name.

        pickedPoint : coin.SoPickedPoint
        """
        ...

    @constmethod
    def getDetailPath(self, subname: str, path: Any, /) -> Any:
        """
        Returns Coin detail of a subelement.
        Note: Not fully implemented. Currently only returns None.

        subname : str
            String reference to the subelement.
        path: coin.SoPath
            Output Coin path leading to the returned element detail.
        """
        ...
    AxisLength: float = 0.0
    """Get/set the axis length."""

    LineWidth: float = 0.0
    """Get/set the axis line width for rendering."""

    PointSize: float = 0.0
    """Get/set the origin point size for rendering."""

    Scale: float = 0.0
    """Get/set auto scaling factor, 0 to disable."""

    Plane: Tuple[Any, ...] = ()
    """Get/set axis plane size and distance to axis line."""

    Labels: Dict[str, str] = {}
    """
    Get/set axis component names as a dictionary.
    Available keys are:
    'O': origin
    'X': x axis
    'Y': y axis
    'Z': z axis
    'XY': xy plane
    'XZ': xz plane
    'YZ': yz plane
    """

    Node: Final[Any] = ...
    """Get the Coin3D node."""
