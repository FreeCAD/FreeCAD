# SPDX-License-Identifier: LGPL-2.1-or-later

from FreeCADGui import _View3DInventor
from PySide import QtGui
from Base.Metadata import constmethod
from Base.BoundBox import BoundBox
from Base.Matrix import Matrix
from Base.Placement import Placement
from App.ExtensionContainer import ExtensionContainer
from App.DocumentObject import DocumentObject
from typing import TYPE_CHECKING, Any, Final, Self, TypeAlias
from enum import Enum
from collections.abc import Sequence

class ViewProvider(ExtensionContainer):
    """
    This is the ViewProvider base class

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    if TYPE_CHECKING:  # because generate deletes top-level declaration
        _CoinSoNode: TypeAlias = Any
        _CoinSoPickedPoint: TypeAlias = Any
        _CoinSoPath: TypeAlias = Any
        _CoinSoDetail: TypeAlias = Any
        _CoinSoSeparator: TypeAlias = Any
        _CoinSoSwitch: TypeAlias = Any

        class _ToggleVisibilityMode(Enum):
            CanToggleVisibility = "CanToggleVisibility"
            NoToggleVisibility = "NoToggleVisibility"

    def addProperty(
        self,
        /,
        type: str,
        name: str,
        group: str = "",
        doc: str = "",
        attr: int = 0,
        read_only: bool = False,
        hidden: bool = False,
        locked: bool = False,
        enum_vals: list[str] | None = None,
    ) -> Self:
        """
        Add a generic property.

        type : str
            Property type.
        name : str
            Property name.
        group : str
            Property group. Defaults to "".
        doc : str
            Property documentation. Defaults to "".
        attr : int
            Property attributes. Defaults to 0.
        read_only : bool
            Whether the property is read-only. Defaults to False.
        hidden : bool
            Whether the property is hidden. Defaults to False.
        locked : bool
            Whether the property is locked. Defaults to False.
        enum_vals : list[str] | None
            Initial values for an enumeration property. Defaults to None.
        """

    def removeProperty(self, name: str, /) -> bool:
        """
        Remove a generic property.
        Only user-defined properties can be removed, not built-in ones.

        name : str
            Property name.
        """

    def supportedProperties(self) -> list[str]:
        """
        A list of supported property types.
        """

    def show(self) -> None:
        """
        Show the object.
        """

    def hide(self) -> None:
        """
        Hide the object.
        """

    def isVisible(self) -> bool:
        """
        Check if the object is visible.
        """

    def canDragObject(self, obj: DocumentObject | None = None, /) -> bool:
        """
        Check whether the child object can be removed by dragging.
        If 'obj' is not given, check without filter by any particular object.

        obj : App.DocumentObject
            Object to be dragged.
        """

    def dragObject(self, obj: DocumentObject, /) -> None:
        """
        Remove a child object by dropping.

        obj : App.DocumentObject
            Object to be dragged.
        """

    def canDropObject(
        self,
        /,
        obj: DocumentObject | None = None,
        owner: DocumentObject | None = None,
        subname: str = ...,
        elem: Sequence[str] | None = None,
    ) -> bool:
        """
        Check whether the child object can be added by dropping.
        If 'obj' is not given, check without filter by any particular object.

        obj : App.DocumentObject
            Object to be dropped.
        owner : App.DocumentObject
            Parent object of the dropping object.
        subname : str
            Subname reference to the dropping object. Optional.
        elem : sequence of str
            Non-objects subelements selected when the object is
            being dropped.
        """

    def dropObject(
        self,
        /,
        obj: DocumentObject,
        owner: DocumentObject | None = None,
        subname: str = ...,
        elem: Sequence[str] | None = None,
    ) -> str:
        """
        Add a child object by dropping.

        obj : App.DocumentObject
            Object to be dropped.
        owner : App.DocumentObject
            Parent object of the dropping object.
        subname : str
            Subname reference to the dropping object. Optional.
        elem : sequence of str
            Non-objects subelements selected when the object is
            being dropped.
        """

    def canDragAndDropObject(self, obj: DocumentObject, /) -> bool:
        """
        Check whether the child object can be removed from
        other parent and added here by drag and drop.

        obj : App.DocumentObject
            Object to be dragged and dropped.
        """

    def replaceObject(self, oldObj: DocumentObject, newObj: DocumentObject, /) -> int:
        """
        Replace a child object.
        Returns 1 if succeeded, 0 if not found, -1 if not supported.

        oldObj : App.DocumentObject
            Old object.
        newObj : App.DocumentObject
            New object.
        """

    def doubleClicked(self, root: DocumentObject = ..., subname: str = "", /) -> bool:
        """
        Trigger double clicking this view object, optionally specifying its root and subobject path.
        """

    def addDisplayMode(self, obj: _CoinSoNode, mode: str, /) -> None:
        """
        Add a new display mode to the view provider.

        obj : coin.SoNode
            Display mode.
        mode : str
            Name of the display mode.
        """

    def listDisplayModes(self) -> list[str]:
        """
        Show a list of all display modes.
        """

    def toString(self) -> str:
        """
        Return a string representation of the Inventor node.
        """

    def setTransformation(self, trans: Matrix | Placement, /) -> None:
        """
        Set a transformation on the Inventor node.

        trans : Base.Placement, Base.Matrix
        """

    @constmethod
    def claimChildren(self) -> list[DocumentObject | None]:
        """
        Returns list of objects that are to be grouped in tree under this object.
        """

    @constmethod
    def claimChildrenRecursive(self) -> list[DocumentObject | None]:
        """
        Returns list of objects that are to be grouped in tree under this object recursively.
        """

    def partialRender(self, sub: str | Sequence[str] | None = None, clear: bool = False, /) -> int:
        """
        Render only part of the object.

        sub: None, str, sequence of str
            Refer to the subelement. If it is None then reset the partial rendering.
        clear: bool
            True to add, or False to remove the subelement(s) for rendering.
        """

    def getElementColors(
        self, elementName: str = ..., /
    ) -> dict[str, tuple[float, float, float, float]]:
        """
        Get a dictionary of the form {elementName : (r,g,b,a)}.
        If no element name is given a dictionary with all the elements is returned.

        elementName : str
            Name of the element. Optional.
        """

    def setElementColors(self, colors: dict[str, Sequence[float]], /) -> None:
        """
        Set element colors.

        colors: dict
            Color dictionary of the form {elementName:(r,g,b,a)}.
        """

    @constmethod
    def getElementPicked(self, pickPoint: _CoinSoPickedPoint, /) -> str | None:
        """
        Return the picked subelement.

        pickPoint : coin.SoPickedPoint
        """

    @constmethod
    def getDetailPath(
        self, subelement: str, path: _CoinSoPath, append: bool = True, /
    ) -> _CoinSoDetail:
        """
        Return Coin detail and path of an subelement.

        subname: str
            Dot separated string reference to the sub element.
        pPath: coin.SoPath
            Output coin path leading to the returned element detail.
        append: bool
            If True, path will be first appended with the root node and the mode
            switch node of this view provider.
        """

    @constmethod
    def signalChangeIcon(self) -> None:
        """
        Trigger icon changed signal.
        """

    def getBoundingBox(
        self,
        /,
        subname: str = ...,
        transform: bool = True,
        view: _View3DInventor = ...,
        mat: Matrix = ...,
        depth: int = 0,
    ) -> BoundBox:
        """
        Obtain the bounding box of this view object.

        subname : str, optional
            Name referring a sub-object.
        transform: bool
            Whether to apply the transformation matrix of this view provider.
        view: View3DInventorPy, optional
            The view to use. Defaults to active view.
        mat: Matrix, optional
            Initial transformation matrix.
        depth: int
            Current recursion depth.
        """
    Annotation: _CoinSoSeparator
    """A pivy Separator to add a custom scenegraph to this ViewProvider."""

    Icon: Final[QtGui.QIcon]
    """The icon of this ViewProvider."""

    RootNode: _CoinSoSeparator
    """A pivy Separator with the root of this ViewProvider."""

    SwitchNode: _CoinSoSwitch
    """A pivy SoSwitch for the display mode switch of this ViewProvider."""

    DefaultMode: int
    """Get/Set the default display mode in turns of coin node index."""

    IV: Final[str] = ""
    """Represents the whole ViewProvider as an Inventor string."""

    CanRemoveChildrenFromRoot: Final[bool]
    """Tells the tree view whether to remove the children item from root or not."""

    LinkVisibility: bool
    """Get/set visibilities of all links to this view object."""

    DropPrefix: Final[str]
    """Subname referencing the sub-object for holding dropped object."""

    ToggleVisibility: _ToggleVisibilityMode | str
    """Get/set whether the viewprovider can toggle the visibility of
    the object."""
