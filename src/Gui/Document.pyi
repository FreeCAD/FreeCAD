# SPDX-License-Identifier: LGPL-2.1-or-later

from FreeCADGui import _MDIView
from Base.Metadata import constmethod
from Base.Persistence import Persistence
from Base.Matrix import Matrix
from typing import Any, Final
from App.Document import Document as AppDocument
from App.DocumentObject import DocumentObject
from ViewProvider import ViewProvider
from ViewProviderDocumentObject import ViewProviderDocumentObject

class Document(Persistence):
    """
    This is a Document class

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def show(self, object_name: str, /) -> None:
        """
        Show an object.

        object_name : str
            Name of the `Gui.ViewProvider` to show.
        """

    def hide(self, object_name: str, /) -> None:
        """
        Hide an object.

        object_name : str
            Name of the `Gui.ViewProvider` to hide.
        """

    def setPos(self, object_name: str, matrix: Matrix, /) -> None:
        """
        Set the position of an object.

        object_name : str
            Name of the `Gui.ViewProvider`.

        matrix : Base.Matrix
            Transformation to apply on the object.
        """

    def setEdit(
        self, obj: str | DocumentObject | ViewProvider, mode: int = 0, subname: str = ..., /,
    ) -> bool:
        """
        Set an object in edit mode.

        obj : str, App.DocumentObject, Gui.ViewPrivider
            Object to set in edit mode.
        mode : int
            Edit mode.
        subname : str
            Subelement name. Optional.
        """

    def getInEdit(self) -> ViewProvider | None:
        """
        Returns the current object in edit mode or None if there is no such object.
        """

    def resetEdit(self) -> None:
        """
        End the current editing.
        """

    def addAnnotation(self, name: str, filename: str, mode: str = "Main", /) -> None:
        """
        Add an Inventor object from a file.

        name : str
            Annotation name.
        filename : str
            File name.
        mode : str
            Display mode name. Optional.
        """

    def update(self) -> None:
        """
        Update the view representations of all objects.
        """

    def getObject(self, name: str, /) -> ViewProvider | None:
        """
        Return the object with the given name. If no one exists, return None.

        name : str
            Object name.
        """

    def activeObject(self) -> ViewProvider | None:
        """
        The active object of the document. Deprecated, use ActiveObject.
        """

    def activeView(self) -> _MDIView | None:
        """
        The active view of the document. Deprecated, use ActiveView.
        """

    def createView(self, type: str, /) -> _MDIView | None:
        """
        Return a newly created view of a given type.

        type : str
            Type name.
        """

    @constmethod
    def mdiViewsOfType(self, type: str, /) -> list[_MDIView]:
        """
        Return a list of mdi views of a given type.

        type : str
            Type name.
        """

    def save(self) -> bool:
        """
        Attempts to save the document
        """

    def saveAs(self) -> bool:
        """
        Attempts to save the document under a new name
        """

    def sendMsgToViews(self, msg: str, /) -> None:
        """
        Send a message to all views of the document.

        msg : str
        """

    def mergeProject(self, filename: str, /) -> None:
        """
        Merges this document with another project file.

        filename : str
            File name.
        """

    def toggleTreeItem(self, obj: DocumentObject, mode: int = 0, subname: str = ..., /) -> None:
        """
        Change TreeItem of a document object.

        obj : App.DocumentObject
        mode : int
            Item mode.
            0: Toggle, 1: Collapse, 2: Expand, 3: Expand path.
        subname : str
            Subelement name. Optional.
        """

    def scrollToTreeItem(self, obj: ViewProviderDocumentObject, /) -> None:
        """
        Scroll the tree view to the item of a view object.

        obj : Gui.ViewProviderDocumentObject
        """

    def toggleInSceneGraph(self, obj: ViewProvider, /) -> None:
        """
        Add or remove view object from scene graph of all views depending
        on its canAddToSceneGraph().

        obj : Gui.ViewProvider
        """

    def openCommand(self, name: str, /, **kwargs: Any) -> int:
        """
        openCommand(name) -> int

        Opens a named transaction for the document and returns it's
        id or 0 on failure
        """

    def commitCommand(self) -> None:
        """
        commitCommand() -> None

        Commits the current transaction of the document
        """

    def abortCommand(self) -> None:
        """
        abortCommand() -> None

        Aborts the current transaction of the document
        """

    ActiveObject: ViewProvider | None
    """The active object of the document."""

    ActiveView: _MDIView | None
    """The active view of the document."""

    EditingTransform: Matrix
    """The editing transformation matrix."""

    @property
    def InEditInfo(self) -> tuple[ViewProviderDocumentObject, str, str, int] | None:
        """
        A tuple(obj,subname,subElement,editMode) of editing object reference, or None if no object is in edit.

        Setter accepts a tuple (obj: ViewProviderDocumentObject, subname: str).
        """

    @InEditInfo.setter
    def InEditInfo(self, /, value: Any) -> None: ...  # wont compile for tuple type

    EditMode: Final[int]
    """Current edit mode. Only meaningful when there is a current object in edit."""

    Document: Final[AppDocument | None]
    """The related App document to this Gui document."""

    Transacting: Final[bool]
    """Indicate whether the document is undoing/redoing."""

    Modified: bool
    """Returns True if the document is marked as modified, and False otherwise."""

    TreeRootObjects: Final[list[DocumentObject]]
    """The list of tree root objects."""
