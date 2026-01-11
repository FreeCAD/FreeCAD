# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod, class_declarations
from Base.PyObjectBase import PyObjectBase

@export(
    PythonName="Part.ShapeFix.Root",
    Include="ShapeFix_Root.hxx",
    Constructor=True,
)
@class_declarations(
    """
private:
    Handle(ShapeFix_Root) hRoot;

public:
    void setHandle(Handle(ShapeFix_Root) handle) {
        setTwinPointer(handle.get());
        hRoot = handle;
    }
"""
)
class ShapeFix_Root(PyObjectBase):
    """
    Root class for fixing operations

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Precision: float = ...
    """Basic precision value"""

    MinTolerance: float = ...
    """Minimal allowed tolerance"""

    MaxTolerance: float = ...
    """Maximal allowed tolerance"""

    @constmethod
    def limitTolerance(self) -> float:
        """
        Returns tolerance limited by [MinTolerance,MaxTolerance]
        """
        ...
