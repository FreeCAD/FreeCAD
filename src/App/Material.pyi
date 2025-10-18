# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, class_declarations
from Base.PyObjectBase import PyObjectBase
from typing import Any, overload


@export(
    Constructor=True,
    Delete=True,
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

    def set(self, string: str, /) -> None:
        """
        Set(string) -- Set the material.

        The material must be one of the following values:
        Brass, Bronze, Copper, Gold, Pewter, Plaster, Plastic, Silver, Steel, Stone, Shiny plastic,
        Satin, Metalized, Neon GNC, Chrome, Aluminium, Obsidian, Neon PHC, Jade, Ruby or Emerald.
        """
        ...

    AmbientColor: Any = ...
    """Ambient color"""

    DiffuseColor: Any = ...
    """Diffuse color"""

    EmissiveColor: Any = ...
    """Emissive color"""

    SpecularColor: Any = ...
    """Specular color"""

    Shininess: float = 0.0
    """Shininess"""

    Transparency: float = 0.0
    """Transparency"""
