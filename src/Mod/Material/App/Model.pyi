from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from typing import Final, List, Dict, overload

@export(
    Include="Mod/Material/App/Model.h",
    Namespace="Materials",
    Constructor=True,
    Delete=True,
)
class Model(BaseClass):
    """
    Material model descriptions.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """
    
    LibraryName: Final[str] = ""
    """Model library name."""
    
    LibraryRoot: Final[str] = ""
    """Model library path."""
    
    LibraryIcon: Final[bytes] = ""
    """Model icon."""
    
    Name: str = ""
    """Model name."""
    
    Type: str = ""
    """Model type."""
    
    Directory: str = ""
    """Model directory."""
    
    UUID: Final[str] = ""
    """Unique model identifier."""
    
    Description: str = ""
    """Description of the model."""
    
    URL: str = ""
    """URL to a detailed description of the model."""
    
    DOI: str = ""
    """Digital Object Identifier (see https://doi.org/)"""
    
    Inherited: Final[List[str]] = []
    """List of inherited models identified by UUID."""
    
    Properties: Final[Dict[str, str]] = {}
    """Dictionary of model properties."""
    
    def addInheritance(self) -> None:
        """
        Add an inherited model.
        """
        ...
    
    def addProperty(self) -> None:
        """
        Add a model property.
        """
        ...
