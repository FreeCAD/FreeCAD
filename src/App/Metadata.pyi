from Base.Metadata import export, class_declarations
from Base.PyObjectBase import PyObjectBase
from typing import Any, List, Dict, overload, Optional


@export(
    Constructor=True,
    Delete=True,
    NumberProtocol=False,
    RichCompare=False,
    )
@class_declarations(
    """public:
    MetadataPy(const Metadata & pla, PyTypeObject *T = &Type)
    :PyObjectBase(new Metadata(pla),T){}
    Metadata value() const
    { return *(getMetadataPtr()); }
        """
)
class Metadata(PyObjectBase):
    """
    App.Metadata class.

    A Metadata object reads an XML-formatted package metadata file and provides
    read and write access to its contents.

    The following constructors are supported:

    Metadata()
    Empty constructor.

    Metadata(metadata)
    Copy constructor.
    metadata : App.Metadata

    Metadata(file)
    Reads the XML file and provides access to the metadata it specifies.
    file : str
        XML file name.

    Metadata(bytes)
    Treats the bytes as UTF-8-encoded XML data and provides access to the metadata it specifies.
    bytes : bytes
        Python bytes-like object.

    Author: Chris Hennes (chennes@pioneerlibrarysystem.org)
    Licence: LGPL
    DeveloperDocu: Metadata
    """

    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, metadata: "Metadata") -> None: ...

    @overload
    def __init__(self, file: str) -> None: ...

    @overload
    def __init__(self, bytes: bytes) -> None: ...

    Name: str = ""
    """String representing the name of this item."""

    Version: str = ""
    """String representing the version of this item in semantic triplet format."""

    Date: str = ""
    """String representing the date of this item in YYYY-MM-DD format (format not currently programmatically enforced)"""

    Type: str = ""
    """String representing the type of this item (text only, no markup allowed)."""

    Description: str = ""
    """String representing the description of this item (text only, no markup allowed)."""

    Maintainer: List[Any] = []
    """List of maintainer objects with 'name' and 'email' string attributes."""

    License: List[Any] = []
    """List of applicable licenses as objects with 'name' and 'file' string attributes."""

    Urls: List[Any] = []
    """
    List of URLs as objects with 'location' and 'type' string attributes, where type
    is one of:
    * website
    * repository
    * bugtracker
    * readme
    * documentation
    """

    Author: List[Any] = []
    """
    List of author objects, each with a 'name' and a (potentially empty) 'email'
    string attribute.
    """

    Depend: List[Any] = []
    """
    List of dependencies, as objects with the following attributes:
    * package
        Required. Must exactly match the contents of the 'name' element in the
        referenced package's package.xml file.
    * version_lt
        Optional. The dependency to the package is restricted to versions less than
        the stated version number.
    * version_lte
        Optional. The dependency to the package is restricted to versions less or
        equal than the stated version number.
    * version_eq
        Optional. The dependency to the package is restricted to a version equal
        than the stated version number.
    * version_gte
        Optional. The dependency to the package is restricted to versions greater
        or equal than the stated version number.
    * version_gt
        Optional. The dependency to the package is restricted to versions greater
        than the stated version number.
    * condition
        Optional. Conditional expression as documented in REP149.
    """

    Conflict: List[Any] = []
    """List of conflicts, format identical to dependencies."""

    Replace: List[Any] = []
    """
    List of things this item is considered by its author to replace. The format is
    identical to dependencies.
    """

    Tag: List[str] = []
    """List of strings."""

    Icon: str = ""
    """Relative path to an icon file."""

    Classname: str = ""
    """
    String representing the name of the main Python class this item
    creates/represents.
    """

    Subdirectory: str = ""
    """
    String representing the name of the subdirectory this content item is located in.
    If empty, the item is in a directory named the same as the content item.
    """

    File: List[Any] = []
    """
    List of files associated with this item.
    The meaning of each file is implementation-defined.
    """

    Content: Dict[str, List["Metadata"]] = {}
    """
    Dictionary of lists of content items: defined recursively, each item is itself
    a Metadata object.
    See package.xml file format documentation for details.
    """

    FreeCADMin: str = ""
    """
    String representing the minimum version of FreeCAD needed for this item.
    If unset it will be 0.0.0.
    """

    FreeCADMax: str = ""
    """
    String representing the maximum version of FreeCAD needed for this item.
    If unset it will be 0.0.0.
    """

    PythonMin: str = ""
    """
    String representing the minimum version of Python needed for this item.
    If unset it will be 0.0.0.
    """

    def getLastSupportedFreeCADVersion(self) -> Optional[str]:
        """
        getLastSupportedFreeCADVersion() -> str or None

        Search through all content package items, and determine if a maximum supported
        version of FreeCAD is set.
        Returns None if no maximum version is set, or if *any* content item fails to
        provide a maximum version (implying that that content item will work with all
        known versions).
        """
        ...

    def getFirstSupportedFreeCADVersion(self) -> Optional[str]:
        """
        getFirstSupportedFreeCADVersion() -> str or None

        Search through all content package items, and determine if a minimum supported
        version of FreeCAD is set.
        Returns 0.0 if no minimum version is set, or if *any* content item fails to
        provide a minimum version (implying that that content item will work with all
        known versions. Technically limited to 0.20 as the lowest known version since
        the metadata standard was added then).
        """
        ...

    def supportsCurrentFreeCAD(self) -> bool:
        """
        supportsCurrentFreeCAD() -> bool

        Returns False if this metadata object directly indicates that it does not
        support the current version of FreeCAD, or True if it makes no indication, or
        specifically indicates that it does support the current version. Does not
        recurse into Content items.
        """
        ...

    def getGenericMetadata(self, name: str) -> List[Any]:
        """
        getGenericMetadata(name) -> list

        Get the list of GenericMetadata objects with key 'name'.
        Generic metadata objects are Python objects with a string 'contents' and a
        dictionary of strings, 'attributes'. They represent unrecognized simple XML tags
        in the metadata file.
        """
        ...

    def addContentItem(self, content_type: str, metadata: "Metadata") -> None:
        """
        addContentItem(content_type,metadata)

        Add a new content item of type 'content_type' with metadata 'metadata'.
        """
        ...

    def removeContentItem(self, content_type: str, name: str) -> None:
        """
        removeContentItem(content_type,name)

        Remove the content item of type 'content_type' with name 'name'.
        """
        ...

    def addMaintainer(self, name: str, email: str) -> None:
        """
        addMaintainer(name, email)

        Add a new Maintainer.
        """
        ...

    def removeMaintainer(self, name: str, email: str) -> None:
        """
        removeMaintainer(name, email)

        Remove the Maintainer.
        """
        ...

    def addLicense(self, short_code: str, path: str) -> None:
        """
        addLicense(short_code,path)

        Add a new License.
        """
        ...

    def removeLicense(self, short_code: str) -> None:
        """
        removeLicense(short_code)

        Remove the License.
        """
        ...

    def addUrl(self, url_type: str, url: str, branch: str) -> None:
        """
        addUrl(url_type,url,branch)

        Add a new Url or type 'url_type' (which should be one of 'repository', 'readme',

        'bugtracker', 'documentation', or 'webpage') If type is 'repository' you

        must also specify the 'branch' parameter.
        """
        ...

    def removeUrl(self, url_type: str, url: str) -> None:
        """
        removeUrl(url_type,url)

        Remove the Url.
        """
        ...

    def addAuthor(self, name: str, email: str) -> None:
        """
        addAuthor(name, email)

        Add a new Author with name 'name', and optionally email 'email'.
        """
        ...

    def removeAuthor(self, name: str, email: str) -> None:
        """
        removeAuthor(name, email)

        Remove the Author.
        """
        ...

    def addDepend(self, name: str, kind: str, optional: bool) -> None:
        """
        addDepend(name, kind, optional)

        Add a new Dependency on package 'name' of kind 'kind' (optional, one of 'auto' (the default),

        'internal', 'addon', or 'python').
        """
        ...

    def removeDepend(self, name: str, kind: str) -> None:
        """
        removeDepend(name, kind)

        Remove the Dependency on package 'name' of kind 'kind' (optional - if unspecified any

        matching name is removed).
        """
        ...

    def addConflict(self, name: str, kind: str) -> None:
        """
        addConflict(name, kind)

        Add a new Conflict. See documentation for addDepend().
        """
        ...

    def removeConflict(self, name: str, kind: str) -> None:
        """
        removeConflict(name, kind)

        Remove the Conflict. See documentation for removeDepend().
        """
        ...

    def addReplace(self, name: str) -> None:
        """
        addReplace(name)

        Add a new Replace.
        """
        ...

    def removeReplace(self, name: str) -> None:
        """
        removeReplace(name)

        Remove the Replace.
        """
        ...

    def addTag(self, tag: str) -> None:
        """
        addTag(tag)

        Add a new Tag.
        """
        ...

    def removeTag(self, tag: str) -> None:
        """
        removeTag(tag)

        Remove the Tag.
        """
        ...

    def addFile(self, filename: str) -> None:
        """
        addFile(filename)

        Add a new File.
        """
        ...

    def removeFile(self, filename: str) -> None:
        """
        removeFile(filename)

        Remove the File.
        """
        ...

    def write(self, filename: str) -> None:
        """
        write(filename)

        Write the metadata to the given file as XML data.
        """
        ...
