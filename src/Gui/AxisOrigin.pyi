# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from typing import TYPE_CHECKING, Any, Final, TypeAlias


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

    if TYPE_CHECKING:
        # these are currently untyped, but this will make typing them later easier.
        _CoinSoPickedPoint: TypeAlias = Any
        _CoinSoPath: TypeAlias = Any
        _CoinSoDetail: TypeAlias = Any
        _CoinSoGroup: TypeAlias = Any

    @constmethod
    def getElementPicked(self, pickedPoint: _CoinSoPickedPoint, /) -> str | None:
        """
        Returns the picked element name.

        pickedPoint : coin.SoPickedPoint
        """

    @constmethod
    def getDetailPath(self, subname: str, path: _CoinSoPath, /) -> _CoinSoDetail | None:
        """
        Returns Coin detail of a subelement.
        Note: Not fully implemented. Currently only returns None.

        subname : str
            String reference to the subelement.
        path: coin.SoPath
            Output Coin path leading to the returned element detail.
        """

    AxisLength: float
    """Get/set the axis length."""

    LineWidth: float
    """Get/set the axis line width for rendering."""

    PointSize: float
    """Get/set the origin point size for rendering."""

    Scale: float
    """Get/set auto scaling factor, 0 to disable."""

    Plane: tuple[float, float]
    """Get/set axis plane size and distance to axis line."""

    Labels: dict[str, str]
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

    Node: Final[_CoinSoGroup]
    """Get the Coin3D node."""
