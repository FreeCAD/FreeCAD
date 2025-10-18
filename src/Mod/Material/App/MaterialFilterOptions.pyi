# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.BaseClass import BaseClass


@export(
    Include="Mod/Material/App/MaterialFilter.h",
    Namespace="Materials",
    Constructor=True,
    Delete=True,
)
class MaterialFilterOptions(BaseClass):
    """
    Material filtering options.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    IncludeFavorites: bool = ...
    """Include materials marked as favorite."""

    IncludeRecent: bool = ...
    """Include recently used materials."""

    IncludeEmptyFolders: bool = ...
    """Include empty folders."""

    IncludeEmptyLibraries: bool = ...
    """Include empty libraries."""

    IncludeLegacy: bool = ...
    """Include materials using the older legacy format."""
