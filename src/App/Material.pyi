# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export, class_declarations
from Base.PyObjectBase import PyObjectBase
from PropertyPythonContracts import _ColorValue, _ColorInput
from typing import overload


@export(
    Constructor=True,
    Delete=True,
    RichCompare=True,
)
@class_declarations("""public:
    static Base::Color toColor(PyObject* value);
        """)
class Material(PyObjectBase):
    """
    App.Material class.

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    UserDocu: This is the Material class
    """

    @overload
    def __init__(self) -> None:
        ...

    @overload
    def __init__(
        self,
        /,
        DiffuseColor: _ColorInput = ...,
        AmbientColor: _ColorInput = ...,
        SpecularColor: _ColorInput = ...,
        EmissiveColor: _ColorInput = ...,
        Shininess: float = ...,
        Transparency: float = ...,
    ) -> None:
        ...

    def set(self, string: str, /) -> None:
        """
        Set(string) -- Set the material.

        The material must be one of the following values:
        Brass, Bronze, Copper, Gold, Pewter, Plaster, Plastic, Silver, Steel, Stone, Shiny plastic,
        Satin, Metalized, Neon GNC, Chrome, Aluminium, Obsidian, Neon PHC, Jade, Ruby or Emerald.
        """

    @property
    def AmbientColor(self) -> _ColorValue:
        """Ambient color"""

    @AmbientColor.setter
    def AmbientColor(self, color: _ColorInput, /) -> None:
        """Ambient color"""

    @property
    def DiffuseColor(self) -> _ColorValue:
        """Diffuse color"""

    @DiffuseColor.setter
    def DiffuseColor(self, color: _ColorInput, /) -> None:
        """Diffuse color"""

    @property
    def EmissiveColor(self) -> _ColorValue:
        """Emissive color"""

    @EmissiveColor.setter
    def EmissiveColor(self, color: _ColorInput, /) -> None:
        """Emissive color"""

    @property
    def SpecularColor(self) -> _ColorValue:
        """Specular color"""

    @SpecularColor.setter
    def SpecularColor(self, color: _ColorInput, /) -> None:
        """Specular color"""

    Shininess: float
    """Shininess"""

    Transparency: float
    """Transparency"""
