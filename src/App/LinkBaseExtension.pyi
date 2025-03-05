from Base.Metadata import export
from DocumentObjectExtension import DocumentObjectExtension
from typing import Any, Final, List, Tuple, Optional, Union, overload


@export(
    Include="App/Link.h",
)
class LinkBaseExtension(DocumentObjectExtension):
    """
    Link extension base class
    Author: Zheng, Lei (realthunder.dev@gmail.com)
    Licence: LGPL
    """

    LinkedChildren: Final[List[Any]] = []
    """Return a flattened (in case grouped by plain group) list of linked children"""

    def configLinkProperty(self, **kwargs) -> Any:
        """
        configLinkProperty(key=val,...): property configuration
        configLinkProperty(key,...): property configuration with default name

        This methode is here to implement what I called Property Design
        Pattern. The extension operates on a predefined set of properties,
        but it relies on the extended object to supply the actual property by
        calling this methode. You can choose a sub set of functionality of
        this extension by supplying only some of the supported properties.

        The 'key' are names used to refer to properties supported by this
        extension, and 'val' is the actual name of the property of your
        object. You can obtain the key names and expected types using
        getLinkPropertyInfo().  You can use property of derived type when
        calling configLinkProperty().  Other types will cause exception to
        ben thrown. The actual properties supported may be different
        depending on the actual extension object underlying this python
        object.

        If 'val' is omitted, i.e. calling configLinkProperty(key,...), then
        it is assumed that the actual property name is the same as 'key'
        """
        ...

    def getLinkExtProperty(self, name: str) -> Any:
        """
        getLinkExtProperty(name): return the property value by its predefined name 
        """
        ...

    def getLinkExtPropertyName(self, name: str) -> str:
        """
        getLinkExtPropertyName(name): lookup the property name by its predefined name 
        """
        ...

    @overload
    def getLinkPropertyInfo(self) -> tuple:
        ...
    
    @overload
    def getLinkPropertyInfo(self, index: int) -> tuple:
        ...
    
    @overload
    def getLinkPropertyInfo(self, name: str) -> tuple:
        ...
    
    def getLinkPropertyInfo(self, arg: Any = None) -> tuple:
        """
        getLinkPropertyInfo(): return a tuple of (name,type,doc) for all supported properties.

        getLinkPropertyInfo(index): return (name,type,doc) of a specific property

        getLinkPropertyInfo(name): return (type,doc) of a specific property
        """
        ...

    def setLink(self, obj: Any, subName: Optional[str] = None, subElements: Optional[Union[str, Tuple[str, ...]]] = None) -> None:
        """
        setLink(obj,subName=None,subElements=None): Set link object.

        setLink([obj,...]),
        setLink([(obj,subName,subElements),...]),
        setLink({index:obj,...}),
        setLink({index:(obj,subName,subElements),...}): set link element of a link group.

        obj (DocumentObject): the object to link to. If this is None, then the link is cleared

        subName (String): Dot separated object path.

        subElements (String|tuple(String)): non-object sub-elements, e.g. Face1, Edge2.
        """
        ...

    def cacheChildLabel(self, enable: bool = True) -> None:
        """
        cacheChildLabel(enable=True): enable/disable child label cache

        The cache is not updated on child label change for performance reason. You must
        call this function on any child label change
        """
        ...

    def flattenSubname(self, subname: str) -> str:
        """
        flattenSubname(subname) -> string

        Return a flattened subname in case it references an object inside a linked plain group
        """
        ...

    def expandSubname(self, subname: str) -> str:
        """
        expandSubname(subname) -> string

        Return an expanded subname in case it references an object inside a linked plain group
        """
        ...
