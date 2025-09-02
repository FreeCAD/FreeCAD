from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Include="Mod/TechDraw/App/DrawTemplate.h",
    Namespace="TechDraw",
)
class DrawTemplate(DocumentObject):
    """
    Author: Luke Parry (l.parry@warwick.ac.uk)
    License: LGPL-2.1-or-later
    Feature for creating and manipulating Technical Drawing Templates
    """
