from Base.Metadata import export, constmethod
from Base.Vector import Vector
from Geometry import Geometry
from typing import overload


@export(
    PythonName="Part.Note",
    Twin="GeomNote",
    TwinPointer="GeomNote",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/GeometryPy.h",
    Constructor=True,
)
class Note(Geometry):
    """
    Describes a note
    To create a note there are several ways:
    Part.Note()
        Creates a default note
    
    Part.Note(Vector)
        Creates a note for the given coordinates
    
    Part.Note(Vector, str)
        Creates a note with the text (str) for the given coordinates

    """

    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, coordinates: Vector) -> None: ...

    @overload
    def __init__(self, coordinates: Vector, text: str) -> None: ...

    X: float = ...
    """X component of this note."""

    Y: float = ...
    """Y component of this note."""

    Z: float = ...
    """Z component of this note."""

    Text: str = ...
    """Text content of the note."""

    FontSize: float = ...
    """Font size used in the note."""

    Color: tuple = ...
    """Color of the note as an RGBA tuple."""
