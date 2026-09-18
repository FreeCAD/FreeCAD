# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from DrawTemplate import DrawTemplate


@export(
    Include="Mod/TechDraw/App/DrawSVGTemplate.h",
    Namespace="TechDraw",
    FatherInclude="DrawTemplatePy.h",
)
class DrawSVGTemplate(DrawTemplate):
    """
    Feature for creating and manipulating Technical Drawing SVG Templates

    Author: Luke Parry (l.parry@warwick.ac.uk)
    License: LGPL-2.1-or-later
    """

    def getEditFieldContent(self, edit_field_name: str, /) -> str | None:
        """getEditFieldContent(EditFieldName) - returns the content of a specific Editable Text Field"""
        ...

    def setEditFieldContent(self, edit_field_name: str, new_content: str, /) -> None:
        """setEditFieldContent(EditFieldName, NewContent) - sets a specific Editable Text Field to a new value"""
        ...

    def translateLabel(self, context: str, base_name: str, unique_name: str, /) -> None:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...
