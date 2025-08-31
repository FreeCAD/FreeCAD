from Base.Metadata import constmethod
from Base.BoundBox import BoundBox
from App.ExtensionContainer import ExtensionContainer
from typing import Any, Final, List, Optional
import enum


class ViewProvider(ExtensionContainer):
    """
    This is the ViewProvider base class

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    class ToggleVisibilityMode(enum.Enum):
        CanToggleVisibility = "CanToggleVisibility"
        NoToggleVisibility = "NoToggleVisibility"


    def addProperty(
        self,
        *,
        type: str,
        name: str,
        group: str,
        doc: str,
        attr: int = 0,
        read_only: bool = False,
        hidden: bool = False,
        locked: bool = False,
    ) -> "ViewProvider":
        """
        addProperty(type, name, group, doc, attr=0, read_only=False, hidden=False, locked=False) -> ViewProvider

        Add a generic property.

        type : str
            Property type.
        name : str
            Property name. Optional.
        group : str
            Property group. Optional.
        attr : int
            Property attributes.
        read_only : bool
            Read only property.
        hidden : bool
            Hidden property.
        locked : bool
            Locked property.
        """
        ...

    def removeProperty(self, name: str) -> bool:
        """
        removeProperty(name) -> bool

        Remove a generic property.
        Only user-defined properties can be removed, not built-in ones.

        name : str
            Property name.
        """
        ...

    def supportedProperties(self) -> list:
        """
        supportedProperties() -> list

        A list of supported property types.
        """
        ...

    def show(self) -> None:
        """
        show() -> None

        Show the object.
        """
        ...

    def hide(self) -> None:
        """
        hide() -> None

        Hide the object.
        """
        ...

    def isVisible(self) -> bool:
        """
        isVisible() -> bool

        Check if the object is visible.
        """
        ...

    def canDragObject(self, obj: Any = None) -> bool:
        """
        canDragObject(obj=None) -> bool

        Check whether the child object can be removed by dragging.
        If 'obj' is not given, check without filter by any particular object.

        obj : App.DocumentObject
            Object to be dragged.
        """
        ...

    def dragObject(self, obj: Any) -> None:
        """
        dragObject(obj) -> None

        Remove a child object by dropping.

        obj : App.DocumentObject
            Object to be dragged.
        """
        ...

    def canDropObject(
        self,
        *,
        obj: Any = None,
        owner: Any = None,
        subname: str,
        elem: Optional[List[str]] = None
    ) -> bool:
        """
        canDropObject(obj=None, owner=None, subname, elem=None) -> bool

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
        ...

    def dropObject(
        self,
        *,
        obj: Any,
        owner: Any = None,
        subname: str,
        elem: Optional[List[str]] = None
    ) -> str:
        """
        dropObject(obj, owner=None, subname, elem=None) -> str

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
        ...

    def canDragAndDropObject(self, obj: Any) -> bool:
        """
        canDragAndDropObject(obj) -> bool

        Check whether the child object can be removed from
        other parent and added here by drag and drop.

        obj : App.DocumentObject
            Object to be dragged and dropped.
        """
        ...

    def replaceObject(self, oldObj: Any, newObj: Any) -> int:
        """
        replaceObject(oldObj, newObj) -> int

        Replace a child object.
        Returns 1 if succeeded, 0 if not found, -1 if not supported.

        oldObj : App.DocumentObject
            Old object.
        newObj : App.DocumentObject
            New object.
        """
        ...

    def doubleClicked(self) -> bool:
        """
        doubleClicked() -> bool

        Trigger double clicking the corresponding tree item of this view object.
        """
        ...

    def addDisplayMode(self, obj: Any, mode: str) -> None:
        """
        addDisplayMode(obj, mode) -> None

        Add a new display mode to the view provider.

        obj : coin.SoNode
            Display mode.
        mode : str
            Name of the display mode.
        """
        ...

    def listDisplayModes(self) -> list:
        """
        listDisplayModes() -> list

        Show a list of all display modes.
        """
        ...

    def toString(self) -> str:
        """
        toString() -> str

        Return a string representation of the Inventor node.
        """
        ...

    def setTransformation(self, trans: Any) -> None:
        """
        setTransformation(trans) -> None

        Set a transformation on the Inventor node.

        trans : Base.Placement, Base.Matrix
        """
        ...

    @constmethod
    def claimChildren(self) -> list:
        """
        claimChildren() -> list

        Returns list of objects that are to be grouped in tree under this object.
        """
        ...

    @constmethod
    def claimChildrenRecursive(self) -> list:
        """
        claimChildrenRecursive() -> list

        Returns list of objects that are to be grouped in tree under this object recursively.
        """
        ...

    def partialRender(self, sub: Any = None, clear: bool = False) -> int:
        """
        partialRender(sub=None, clear=False) -> int

        Render only part of the object.

        sub: None, str, sequence of str
            Refer to the subelement. If it is None then reset the partial rendering.
        clear: bool
            True to add, or False to remove the subelement(s) for rendering.
        """
        ...

    def getElementColors(self, elementName: Optional[str] = None) -> dict:
        """
        getElementColors(elementName) -> dict

        Get a dictionary of the form {elementName : (r,g,b,a)}.
        If no element name is given a dictionary with all the elements is returned.

        elementName : str
            Name of the element. Optional.
        """
        ...

    def setElementColors(self, colors: dict) -> None:
        """
        setElementColors(colors) -> None

        Set element colors.

        colors: dict
            Color dictionary of the form {elementName:(r,g,b,a)}.
        """
        ...

    @constmethod
    def getElementPicked(self, pickPoint: Any) -> str:
        """
        getElementPicked(pickPoint) -> str

        Return the picked subelement.

        pickPoint : coin.SoPickedPoint
        """
        ...

    @constmethod
    def getDetailPath(self, subelement: str, path: Any, append: bool = True) -> Any:
        """
        getDetailPath(subelement, path, append=True) -> coin.SoDetail or None

        Return Coin detail and path of an subelement.

        subname: str
            Dot separated string reference to the sub element.
        pPath: coin.SoPath
            Output coin path leading to the returned element detail.
        append: bool
            If True, path will be first appended with the root node and the mode
            switch node of this view provider.
        """
        ...

    @constmethod
    def signalChangeIcon(self) -> None:
        """
        signalChangeIcon() -> None

        Trigger icon changed signal.
        """
        ...

    def getBoundingBox(
        self, subName: Optional[str] = None, transform: bool = True, view: Any = None
    ) -> BoundBox:
        """
        getBoundingBox(subName, transform=True, view) -> Base.BoundBox

        Obtain the bounding box of this view object.

        subName : str
            Name referring a sub-object. Optional.
        transform: bool
            Whether to apply the transformation matrix of this view provider.
        view: View3DInventorPy
            Default to active view. Optional.
        """
        ...

    Annotation: Any = ...
    """A pivy Separator to add a custom scenegraph to this ViewProvider."""

    Icon: Final[Any] = ...
    """The icon of this ViewProvider."""

    RootNode: Any = ...
    """A pivy Separator with the root of this ViewProvider."""

    SwitchNode: Any = ...
    """A pivy SoSwitch for the display mode switch of this ViewProvider."""

    DefaultMode: int = 0
    """Get/Set the default display mode in turns of coin node index."""

    IV: Final[str] = ""
    """Represents the whole ViewProvider as an Inventor string."""

    CanRemoveChildrenFromRoot: Final[bool] = False
    """Tells the tree view whether to remove the children item from root or not."""

    LinkVisibility: bool = False
    """Get/set visibilities of all links to this view object."""

    DropPrefix: Final[str] = ""
    """Subname referencing the sub-object for holding dropped object."""

    ToggleVisibility: ToggleVisibilityMode = ToggleVisibilityMode.CanToggleVisibility
    """Get/set whether the viewprovider can toggle the visibility of
    the object."""
