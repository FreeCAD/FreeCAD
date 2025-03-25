# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 The FreeCAD project association AISBL              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""The Addon Catalog is the main list of all Addons along with their various
sources and compatible versions. Added in FreeCAD 1.1 to replace .gitmodules."""

from dataclasses import dataclass
from hashlib import sha256
from typing import Any, Dict, List, Optional, Tuple
from addonmanager_metadata import Version
from Addon import Addon

import addonmanager_freecad_interface as fci


@dataclass
class AddonCatalogEntry:
    """Each individual entry in the catalog, storing data about a particular version of an
    Addon. Note that this class needs to be identical to the one that is used in the remote cache
    generation, so don't make changes here without ensuring that the classes are synchronized."""

    freecad_min: Optional[Version] = None
    freecad_max: Optional[Version] = None
    repository: Optional[str] = None
    git_ref: Optional[str] = None
    zip_url: Optional[str] = None
    note: Optional[str] = None
    branch_display_name: Optional[str] = None

    def __init__(self, raw_data: Dict[str, str]) -> None:
        """Create an AddonDictionaryEntry from the raw JSON data"""
        super().__init__()
        for key, value in raw_data.items():
            if hasattr(self, key):
                if key in ("freecad_min", "freecad_max"):
                    value = Version(from_string=value)
                setattr(self, key, value)

    def is_compatible(self) -> bool:
        """Check whether this AddonCatalogEntry is compatible with the current version of FreeCAD"""
        if self.freecad_min is None and self.freecad_max is None:
            return True
        current_version = Version(from_list=fci.Version())
        if self.freecad_min is None:
            return current_version <= self.freecad_max
        if self.freecad_max is None:
            return current_version >= self.freecad_min
        return self.freecad_min <= current_version <= self.freecad_max

    def unique_identifier(self) -> str:
        """Return a unique identifier of the AddonCatalogEntry, guaranteed to be repeatable: when
        given the same basic information, the same ID is created. Used as the key when storing
        the metadata for a given AddonCatalogEntry."""
        sha256_hash = sha256()
        sha256_hash.update(str(self).encode("utf-8"))
        return sha256_hash.hexdigest()


class AddonCatalog:
    """A catalog of addons grouped together into sets representing versions that are
    compatible with different versions of FreeCAD and/or represent different available branches
    of a given addon (e.g. a Development branch that users are presented)."""

    def __init__(self, data: Dict[str, Any]):
        self._original_data = data
        self._dictionary = {}
        self._parse_raw_data()

    def _parse_raw_data(self):
        self._dictionary = {}  # Clear pre-existing contents
        for key, value in self._original_data.items():
            if key == "_meta":  # Don't add the documentation object to the tree
                continue
            self._dictionary[key] = []
            for entry in value:
                self._dictionary[key].append(AddonCatalogEntry(entry))

    def load_metadata_cache(self, cache: Dict[str, Any]):
        """Given the raw dictionary, couple that with the remote metadata cache to create the
        final working addon dictionary. Only create Addons that are compatible with the current
        version of FreeCAD."""
        for value in self._dictionary.values():
            for entry in value:
                sha256_hash = entry.unique_identifier()
                print(sha256_hash)
                if sha256_hash in cache and entry.is_compatible():
                    entry.addon = Addon.from_cache(cache[sha256_hash])

    def get_available_addon_ids(self) -> List[str]:
        """Get a list of IDs that have at least one entry compatible with the current version of
        FreeCAD"""
        id_list = []
        for key, value in self._dictionary.items():
            for entry in value:
                if entry.is_compatible():
                    id_list.append(key)
                    break
        return id_list

    def get_available_branches(self, addon_id: str) -> List[Tuple[str, str]]:
        """For a given ID, get the list of available branches compatible with this version of
        FreeCAD along with the branch display name. Either field may be empty, but not both. The
        first entry in the list is expected to be the "primary"."""
        if addon_id not in self._dictionary:
            return []
        result = []
        for entry in self._dictionary[addon_id]:
            if entry.is_compatible():
                result.append((entry.git_ref, entry.branch_display_name))
        return result

    def get_addon_from_id(self, addon_id: str, branch: Optional[Tuple[str, str]] = None) -> Addon:
        """Get the instantiated Addon object for the given ID and optionally branch. If no
        branch is provided, whichever branch is the "primary" branch will be returned (i.e. the
        first branch that matches). Raises a ValueError if no addon matches the request."""
        if addon_id not in self._dictionary:
            raise ValueError(f"Addon '{addon_id}' not found")
        for entry in self._dictionary[addon_id]:
            if not entry.is_compatible():
                continue
            if not branch or entry.branch_display_name == branch:
                return entry.addon
        raise ValueError(f"Addon '{addon_id}' has no compatible branches named '{branch}'")
