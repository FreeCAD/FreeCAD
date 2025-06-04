from PropertyContainer import PropertyContainer
from DocumentObject import DocumentObject
from typing import Final, List, Tuple, Sequence


class Document(PropertyContainer):
    """
    This is a Document class
    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    DependencyGraph: Final[str] = ""
    """The dependency graph as GraphViz text"""

    ActiveObject: Final[DocumentObject] = None
    """The last created object in this document"""

    Objects: Final[List[DocumentObject]] = []
    """The list of objects in this document"""

    TopologicalSortedObjects: Final[List[DocumentObject]] = []
    """The list of objects in this document in topological sorted order"""

    RootObjects: Final[List[DocumentObject]] = []
    """The list of root objects in this document"""

    RootObjectsIgnoreLinks: Final[List[DocumentObject]] = []
    """The list of root objects in this document ignoring references from links."""

    UndoMode: int = 0
    """The Undo mode of the Document (0 = no Undo, 1 = Undo/Redo)"""

    UndoRedoMemSize: Final[int] = 0
    """The size of the Undo stack in byte"""

    UndoCount: Final[int] = 0
    """Number of possible Undos"""

    RedoCount: Final[int] = 0
    """Number of possible Redos"""

    UndoNames: Final[List[str]] = []
    """A list of Undo names"""

    RedoNames: Final[List[str]] = []
    """A List of Redo names"""

    Name: Final[str] = ""
    """The internal name of the document"""

    RecomputesFrozen: bool = False
    """Returns or sets if automatic recomputes for this document are disabled."""

    HasPendingTransaction: Final[bool] = False
    """Check if there is a pending transaction"""

    InList: Final[List["Document"]] = []
    """A list of all documents that link to this document."""

    OutList: Final[List["Document"]] = []
    """A list of all documents that this document links to."""

    Restoring: Final[bool] = False
    """Indicate if the document is restoring"""

    Partial: Final[bool] = False
    """Indicate if the document is partially loaded"""

    Importing: Final[bool] = False
    """Indicate if the document is importing. Note the document will also report Restoring while importing"""

    Recomputing: Final[bool] = False
    """Indicate if the document is recomputing"""

    Transacting: Final[bool] = False
    """Indicate whether the document is undoing/redoing"""

    OldLabel: Final[str] = ""
    """Contains the old label before change"""

    Temporary: Final[bool] = False
    """Check if this is a temporary document"""

    def save(self) -> None:
        """
        Save the document to disk
        """
        ...

    def saveAs(self) -> None:
        """
        Save the document under a new name to disk
        """
        ...

    def saveCopy(self) -> None:
        """
        Save a copy of the document under a new name to disk
        """
        ...

    def load(self) -> None:
        """
        Load the document from the given path
        """
        ...

    def restore(self) -> None:
        """
        Restore the document from disk
        """
        ...

    def isSaved(self) -> bool:
        """
        Checks if the document is saved
        """
        ...

    def getProgramVersion(self) -> str:
        """
        Get the program version that a project file was created with
        """
        ...

    def getFileName(self) -> str:
        """
        For a regular document it returns its file name property.
        For a temporary document it returns its transient directory.
        """
        ...

    def getUniqueObjectName(self, objName: str) -> str:
        """
        getUniqueObjectName(objName) -> objName

        Return the same name, or the name made unique, for Example Box -> Box002 if there are conflicting name
        already in the document.

        ObjName : str
        Object name.
        """
        ...

    def mergeProject(self) -> None:
        """
        Merges this document with another project file
        """
        ...

    def exportGraphviz(self) -> None:
        """
        Export the dependencies of the objects as graph
        """
        ...

    def openTransaction(self, name: str) -> None:
        """
                openTransaction(name) - Open a new Undo/Redo transaction.

        This function no long creates a new transaction, but calls
        FreeCAD.setActiveTransaction(name) instead, which will auto creates a
        transaction with the given name when any change happed in any opened document.
        If more than one document is changed, all newly created transactions will have
        the same internal ID and will be undo/redo together.
        """
        ...

    def abortTransaction(self) -> None:
        """
        Abort an Undo/Redo transaction (rollback)
        """
        ...

    def commitTransaction(self) -> None:
        """
        Commit an Undo/Redo transaction
        """
        ...

    def addObject(
        self,
        *,
        type: str,
        name: str = None,
        objProxy: object = None,
        viewProxy: object = None,
        attach: bool = False,
        viewType: str = None,
    ) -> DocumentObject:
        """
        addObject(type, name=None, objProxy=None, viewProxy=None, attach=False, viewType=None)

        Add an object to document

        type (String): the type of the document object to create.
        name (String): the optional name of the new object.
        objProxy (Object): the Python binding object to attach to the new document object.
        viewProxy (Object): the Python binding object to attach the view provider of this object.
        attach (Boolean): if True, then bind the document object first before adding to the document
                to allow Python code to override view provider type. Once bound, and before adding to
                the document, it will try to call Python binding object's attach(obj) method.
        viewType (String): override the view provider type directly, only effective when attach is False.
        """
        ...

    def addProperty(
        self,
        *,
        type: str,
        name: str,
        group: str = "",
        doc: str = "",
        attr: int = 0,
        read_only: bool = False,
        hidden: bool = False,
        locked: bool = False,
    ) -> "Document":
        """
        addProperty(type: string, name: string, group="", doc="", attr=0, read_only=False, hidden=False, locked=False) -- Add a generic property.
        """
        ...

    def removeProperty(self, string: str) -> None:
        """
        removeProperty(string) -- Remove a generic property.
        Note, you can only remove user-defined properties but not built-in ones.
        """
        ...

    def removeObject(self) -> None:
        """
        Remove an object from the document
        """
        ...

    def copyObject(
        self,
        object: DocumentObject,
        *,
        recursive: bool = False,
        return_all: bool = False,
    ) -> Tuple[DocumentObject, ...]:
        """
        copyObject(object, recursive=False, return_all=False)
        Copy an object or objects from another document to this document.

        object: can either be a single object or sequence of objects
        recursive: if True, also recursively copies internal objects
        return_all: if True, returns all copied objects, or else return only the copied
                    object corresponding to the input objects.
        """
        ...

    def moveObject(
        self, object: DocumentObject, with_dependencies: bool = False
    ) -> DocumentObject:
        """
        moveObject(object, bool with_dependencies = False)
        Transfers an object from another document to this document.

        object: can either a single object or sequence of objects
        with_dependencies: if True, all internal dependent objects are copied too.
        """
        ...

    def importLinks(self, object: DocumentObject = None) -> Tuple[DocumentObject, ...]:
        """
        importLinks(object|[object...])

        Import any externally linked object given a list of objects in
        this document.  Any link type properties of the input objects
        will be automatically reassigned to the imported object

        If no object is given as input, it import all externally linked
        object of this document.
        """
        ...

    def undo(self) -> None:
        """
        Undo one transaction
        """
        ...

    def redo(self) -> None:
        """
        Redo a previously undone transaction
        """
        ...

    def clearUndos(self) -> None:
        """
        Clear the undo stack of the document
        """
        ...

    def clearDocument(self) -> None:
        """
        Clear the whole document
        """
        ...

    def setClosable(self, closable: bool) -> None:
        """
        Set a flag that allows or forbids to close a document
        """
        ...

    def isClosable(self) -> bool:
        """
        Check if the document can be closed. The default value is True
        """
        ...

    def setAutoCreated(self, autoCreated: bool) -> None:
        """
        Set a flag that indicates if a document is autoCreated
        """
        ...

    def isAutoCreated(self) -> bool:
        """
        Check if the document is autoCreated. The default value is False
        """
        ...

    def recompute(self, objs: Sequence[DocumentObject] = None) -> int:
        """
        recompute(objs=None): Recompute the document and returns the amount of recomputed features
        """
        ...

    def mustExecute(self) -> bool:
        """
        Check if any object must be recomputed
        """
        ...

    def purgeTouched(self) -> None:
        """
        Purge the touched state of all objects
        """
        ...

    def isTouched(self) -> bool:
        """
        Check if any object is in touched state
        """
        ...

    def getObject(self, name: str) -> DocumentObject:
        """
        Return the object with the given name
        """
        ...

    def getObjectsByLabel(self, label: str) -> List[DocumentObject]:
        """
        Return the objects with the given label name.
        NOTE: It's possible that several objects have the same label name.
        """
        ...

    def findObjects(
        self, *, Type: str = None, Name: str = None, Label: str = None
    ) -> List[DocumentObject]:
        """
        findObjects([Type=string], [Name=string], [Label=string]) -> list
        Return a list of objects that match the specified type, name or label.
        Name and label support regular expressions. All parameters are optional.
        """
        ...

    def getLinksTo(
        self, obj: DocumentObject, options: int = 0, maxCount: int = 0
    ) -> Tuple[DocumentObject, ...]:
        """
        getLinksTo(obj, options=0, maxCount=0): return objects linked to 'obj'

        options: 1: recursive, 2: check link array. Options can combine.
        maxCount: to limit the number of links returned
        """
        ...

    def supportedTypes(self) -> List[str]:
        """
        A list of supported types of objects
        """
        ...

    def getTempFileName(self) -> str:
        """
        Returns a file name with path in the temp directory of the document.
        """
        ...

    def getDependentDocuments(self, sort: bool = True) -> List[DocumentObject]:
        """
        getDependentDocuments(sort=True)

        Returns a list of documents that this document directly or indirectly links to including itself.

        sort: whether to topologically sort the return list
        """
        ...
