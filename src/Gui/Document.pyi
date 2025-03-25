from Base.Metadata import constmethod
from Base.Persistence import Persistence
from Base.Matrix import Matrix
from typing import Any, Final, List, Optional


class Document(Persistence):
    """
    This is a Document class

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def show(self, objName: str) -> None:
        """
        show(objName) -> None

        Show an object.

        objName : str
            Name of the `Gui.ViewProvider` to show.
        """
        ...

    def hide(self, objName: str) -> None:
        """
        hide(objName) -> None

        Hide an object.

        objName : str
            Name of the `Gui.ViewProvider` to hide.
        """
        ...

    def setPos(self, objName: str, matrix: Matrix) -> None:
        """
        setPos(objName, matrix) -> None

        Set the position of an object.

        objName : str
            Name of the `Gui.ViewProvider`.

        matrix : Base.Matrix
            Transformation to apply on the object.
        """
        ...

    def setEdit(self, obj: Any, mod: int = 0, subName: Optional[str] = None) -> bool:
        """
        setEdit(obj, mod=0, subName) -> bool

        Set an object in edit mode.

        obj : str, App.DocumentObject, Gui.ViewPrivider
            Object to set in edit mode.
        mod : int
            Edit mode.
        subName : str
            Subelement name. Optional.
        """
        ...

    def getInEdit(self) -> Optional[Any]:
        """
        getInEdit() -> Gui.ViewProviderDocumentObject or None

        Returns the current object in edit mode or None if there is no such object.
        """
        ...

    def resetEdit(self) -> None:
        """
        resetEdit() -> None

        End the current editing.
        """
        ...

    def addAnnotation(self, annoName: str, fileName: str, modName: str) -> None:
        """
        addAnnotation(annoName, fileName, modName) -> None

        Add an Inventor object from a file.

        annoName : str
            Annotation name.
        fileName : str
            File name.
        modName : str
            Display mode name. Optional.
        """
        ...

    def update(self) -> None:
        """
        update() -> None

        Update the view representations of all objects.
        """
        ...

    def getObject(self, objName: str) -> Optional[Any]:
        """
        getObject(objName) -> object or None

        Return the object with the given name. If no one exists, return None.

        ObjName : str
            Object name.
        """
        ...

    def activeObject(self) -> Optional[Any]:
        """
        activeObject() -> object or None

        The active object of the document. Deprecated, use ActiveObject.
        """
        ...

    def activeView(self) -> Optional[Any]:
        """
        activeView() -> object or None

        The active view of the document. Deprecated, use ActiveView.
        """
        ...

    def createView(self, type: str) -> Optional[Any]:
        """
        createView(type) -> object or None

        Return a newly created view of a given type.

        type : str
            Type name.
        """
        ...

    @constmethod
    def mdiViewsOfType(self, type: str) -> List[Any]:
        """
        mdiViewsOfType(type) -> list of MDIView

        Return a list of mdi views of a given type.

        type : str
            Type name.
        """
        ...

    def save(self) -> bool:
        """
        save() -> bool

        Attempts to save the document
        """
        ...

    def saveAs(self) -> bool:
        """
        saveAs() -> bool

        Attempts to save the document under a new name
        """
        ...

    def sendMsgToViews(self, msg: str) -> None:
        """
        sendMsgToViews(msg) -> None

        Send a message to all views of the document.

        msg : str
        """
        ...

    def mergeProject(self, fileName: str) -> None:
        """
        mergeProject(fileName) -> None

        Merges this document with another project file.

        fileName : str
            File name.
        """
        ...

    def toggleTreeItem(
        self, obj: Any, mod: int = 0, subName: Optional[str] = None
    ) -> None:
        """
        toggleTreeItem(obj, mod=0, subName) -> None

        Change TreeItem of a document object.

        obj : App.DocumentObject
        mod : int
            Item mode.
            0: Toggle, 1: Collapse, 2: Expand, 3: Expand path.
        subName : str
            Subelement name. Optional.
        """
        ...

    def scrollToTreeItem(self, obj: Any) -> None:
        """
        scrollToTreeItem(obj) -> None

        Scroll the tree view to the item of a view object.

        obj : Gui.ViewProviderDocumentObject
        """
        ...

    def toggleInSceneGraph(self, obj: Any) -> None:
        """
        toggleInSceneGraph(obj) -> None

        Add or remove view object from scene graph of all views depending
        on its canAddToSceneGraph().

        obj : Gui.ViewProvider
        """
        ...

    ActiveObject: Any = ...
    """The active object of the document."""

    ActiveView: Any = ...
    """The active view of the document."""

    EditingTransform: Any = ...
    """The editing transformation matrix."""

    InEditInfo: Any = ...
    """A tuple(obj,subname,subElement,editMode) of editing object reference, or None if no object is in edit."""

    EditMode: Final[int] = 0
    """Current edit mode. Only meaningful when there is a current object in edit."""

    Document: Final[Any] = ...
    """The related App document to this Gui document."""

    Transacting: Final[bool] = False
    """Indicate whether the document is undoing/redoing."""

    Modified: bool = False
    """Returns True if the document is marked as modified, and False otherwise."""

    TreeRootObjects: Final[List[Any]] = []
    """The list of tree root objects."""
