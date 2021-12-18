# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD

import os
from typing import Dict

from addonmanager_macro import Macro


class AddonManagerRepo:
    "Encapsulate information about a FreeCAD addon"

    from enum import IntEnum

    class RepoType(IntEnum):
        WORKBENCH = 1
        MACRO = 2
        PACKAGE = 3

        def __str__(self) -> str:
            if self.value == 1:
                return "Workbench"
            elif self.value == 2:
                return "Macro"
            elif self.value == 3:
                return "Package"

    class UpdateStatus(IntEnum):
        NOT_INSTALLED = 0
        UNCHECKED = 1
        NO_UPDATE_AVAILABLE = 2
        UPDATE_AVAILABLE = 3
        PENDING_RESTART = 4

        def __lt__(self, other):
            if self.__class__ is other.__class__:
                return self.value < other.value
            return NotImplemented

        def __str__(self) -> str:
            if self.value == 0:
                return "Not installed"
            elif self.value == 1:
                return "Unchecked"
            elif self.value == 2:
                return "No update available"
            elif self.value == 3:
                return "Update available"
            elif self.value == 4:
                return "Restart required"

    def __init__(self, name: str, url: str, status: UpdateStatus, branch: str):
        self.name = name.strip()
        self.display_name = self.name
        self.url = url.strip()
        self.branch = branch.strip()
        self.update_status = status
        self.repo_type = AddonManagerRepo.RepoType.WORKBENCH
        self.description = None
        from addonmanager_utilities import construct_git_url

        self.metadata_url = (
            "" if not self.url else construct_git_url(self, "package.xml")
        )
        self.metadata = None
        self.icon = None
        self.cached_icon_filename = ""
        self.macro = None  # Bridge to Gaël Écorchard's macro management class
        self.updated_timestamp = None
        self.installed_version = None

    def __str__(self) -> str:
        result = f"FreeCAD {self.repo_type}\n"
        result += f"Name: {self.name}\n"
        result += f"URL: {self.url}\n"
        result += (
            "Has metadata\n" if self.metadata is not None else "No metadata found\n"
        )
        if self.macro is not None:
            result += "Has linked Macro object\n"
        return result

    @classmethod
    def from_macro(self, macro: Macro):
        if macro.is_installed():
            status = AddonManagerRepo.UpdateStatus.UNCHECKED
        else:
            status = AddonManagerRepo.UpdateStatus.NOT_INSTALLED
        instance = AddonManagerRepo(macro.name, macro.url, status, "master")
        instance.macro = macro
        instance.repo_type = AddonManagerRepo.RepoType.MACRO
        instance.description = macro.desc
        return instance

    @classmethod
    def from_cache(self, data: Dict):
        """Load basic data from cached dict data. Does not include Macro or Metadata information, which must be populated separately."""

        mod_dir = os.path.join(FreeCAD.getUserAppDataDir(), "Mod", data["name"])
        if os.path.isdir(mod_dir):
            status = AddonManagerRepo.UpdateStatus.UNCHECKED
        else:
            status = AddonManagerRepo.UpdateStatus.NOT_INSTALLED
        instance = AddonManagerRepo(data["name"], data["url"], status, data["branch"])
        instance.display_name = data["display_name"]
        instance.repo_type = AddonManagerRepo.RepoType(data["repo_type"])
        instance.description = data["description"]
        instance.cached_icon_filename = data["cached_icon_filename"]
        if instance.repo_type == AddonManagerRepo.RepoType.PACKAGE:
            # There must be a cached metadata file, too
            cached_package_xml_file = os.path.join(
                FreeCAD.getUserCachePath(), "AddonManager", "PackageMetadata", instance.name
            )
            if os.path.isfile(cached_package_xml_file):
                instance.load_metadata_file(cached_package_xml_file)
        return instance

    def to_cache(self) -> Dict:
        """Returns a dictionary with cache information that can be used later with from_cache to recreate this object."""

        return {
            "name": self.name,
            "display_name": self.display_name,
            "url": self.url,
            "branch": self.branch,
            "repo_type": int(self.repo_type),
            "description": self.description,
            "cached_icon_filename": self.get_cached_icon_filename(),
        }

    def load_metadata_file (self, file:str) -> None:
        if os.path.isfile(file):
            metadata = FreeCAD.Metadata(file)
            self.set_metadata(metadata)

    def set_metadata (self, metadata:FreeCAD.Metadata) -> None:
        self.metadata = metadata
        self.display_name = metadata.Name
        self.repo_type = AddonManagerRepo.RepoType.PACKAGE
        self.description = metadata.Description
        for url in metadata.Urls:
            if "type" in url and url["type"] == "repository":
                self.url = url["location"]
                if "branch" in url:
                    self.branch = url["branch"]
                else:
                    self.branch = "master"

    def contains_workbench(self) -> bool:
        """Determine if this package contains (or is) a workbench"""

        if self.repo_type == AddonManagerRepo.RepoType.WORKBENCH:
            return True
        elif self.repo_type == AddonManagerRepo.RepoType.PACKAGE:
            content = self.metadata.Content
            return "workbench" in content
        else:
            return False

    def contains_macro(self) -> bool:
        """Determine if this package contains (or is) a macro"""

        if self.repo_type == AddonManagerRepo.RepoType.MACRO:
            return True
        elif self.repo_type == AddonManagerRepo.RepoType.PACKAGE:
            content = self.metadata.Content
            return "macro" in content
        else:
            return False

    def contains_preference_pack(self) -> bool:
        """Determine if this package contains a preference pack"""

        if self.repo_type == AddonManagerRepo.RepoType.PACKAGE:
            content = self.metadata.Content
            return "preferencepack" in content
        else:
            return False

    def get_cached_icon_filename(self) -> str:
        """Get the filename for the locally-cached copy of the icon"""

        if self.cached_icon_filename:
            return self.cached_icon_filename

        if not self.metadata:
            return ""

        real_icon = self.metadata.Icon
        if not real_icon:
            # If there is no icon set for the entire package, see if there are any workbenches, which
            # are required to have icons, and grab the first one we find:
            content = self.metadata.Content
            if "workbench" in content:
                wb = content["workbench"][0]
                if wb.Icon:
                    if wb.Subdirectory:
                        subdir = wb.Subdirectory
                    else:
                        subdir = wb.Name
                    real_icon = subdir + wb.Icon

        real_icon = real_icon.replace(
            "/", os.path.sep
        )  # Required path separator in the metadata.xml file to local separator

        _, file_extension = os.path.splitext(real_icon)
        store = os.path.join(
            FreeCAD.getUserCachePath(), "AddonManager", "PackageMetadata"
        )
        self.cached_icon_filename = os.path.join(
            store, self.name, "cached_icon" + file_extension
        )

        return self.cached_icon_filename
