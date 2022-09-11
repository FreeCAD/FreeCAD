# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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

""" Defines the Addon class to encapsulate information about FreeCAD Addons """

import os
from urllib.parse import urlparse
from typing import Dict, Set
from threading import Lock
from enum import IntEnum

import FreeCAD

from addonmanager_macro import Macro
import addonmanager_utilities as utils
from addonmanager_utilities import construct_git_url

translate = FreeCAD.Qt.translate

INTERNAL_WORKBENCHES = {}
INTERNAL_WORKBENCHES["arch"] = "Arch"
INTERNAL_WORKBENCHES["draft"] = "Draft"
INTERNAL_WORKBENCHES["fem"] = "FEM"
INTERNAL_WORKBENCHES["mesh"] = "Mesh"
INTERNAL_WORKBENCHES["openscad"] = "OpenSCAD"
INTERNAL_WORKBENCHES["part"] = "Part"
INTERNAL_WORKBENCHES["partdesign"] = "PartDesign"
INTERNAL_WORKBENCHES["path"] = "Path"
INTERNAL_WORKBENCHES["plot"] = "Plot"
INTERNAL_WORKBENCHES["points"] = "Points"
INTERNAL_WORKBENCHES["raytracing"] = "Raytracing"
INTERNAL_WORKBENCHES["robot"] = "Robot"
INTERNAL_WORKBENCHES["sketcher"] = "Sketcher"
INTERNAL_WORKBENCHES["spreadsheet"] = "Spreadsheet"
INTERNAL_WORKBENCHES["techdraw"] = "TechDraw"


class Addon:
    """Encapsulates information about a FreeCAD addon"""

    class Kind(IntEnum):
        """The type of Addon: Workbench, macro, or package"""

        WORKBENCH = 1
        MACRO = 2
        PACKAGE = 3

        def __str__(self) -> str:
            if self.value == 1:
                return "Workbench"
            if self.value == 2:
                return "Macro"
            if self.value == 3:
                return "Package"
            return "ERROR_TYPE"

    class Status(IntEnum):
        """The installation status of an Addon"""

        NOT_INSTALLED = 0
        UNCHECKED = 1
        NO_UPDATE_AVAILABLE = 2
        UPDATE_AVAILABLE = 3
        PENDING_RESTART = 4
        CANNOT_CHECK = 5  # If we don't have git, etc.

        def __lt__(self, other):
            if self.__class__ is other.__class__:
                return self.value < other.value
            return NotImplemented

        def __str__(self) -> str:
            result = ""
            if self.value == 0:
                result = "Not installed"
            elif self.value == 1:
                result = "Unchecked"
            elif self.value == 2:
                result = "No update available"
            elif self.value == 3:
                result = "Update available"
            elif self.value == 4:
                result = "Restart required"
            elif self.value == 5:
                result = "Can't check"
            else:
                result = "ERROR_STATUS"
            return result

    class Dependencies:
        """Addon dependency information"""

        def __init__(self):
            self.required_external_addons = []  # A list of Addons
            self.blockers = []  # A list of Addons
            self.replaces = []  # A list of Addons
            self.internal_workbenches: Set[str] = set()  # Required internal workbenches
            self.python_required: Set[str] = set()
            self.python_optional: Set[str] = set()

    class ResolutionFailed(RuntimeError):
        """An exception type for dependency resolution failure."""

    # The location of Addon Manager cache files: overridden by testing code
    cache_directory = os.path.join(FreeCAD.getUserCachePath(), "AddonManager")

    # The location of the Mod directory: overridden by testing code
    mod_directory = os.path.join(FreeCAD.getUserAppDataDir(), "Mod")

    def __init__(self, name: str, url: str, status: Status, branch: str):
        self.name = name.strip()
        self.display_name = self.name
        self.url = url.strip()
        self.branch = branch.strip()
        self.python2 = False
        self.obsolete = False
        self.rejected = False
        self.repo_type = Addon.Kind.WORKBENCH
        self.description = None
        self.tags = set()  # Just a cache, loaded from Metadata

        # To prevent multiple threads from running git actions on this repo at the same time
        self.git_lock = Lock()

        # To prevent multiple threads from accessing the status at the same time
        self.status_lock = Lock()
        self.set_status(status)

        # The url should never end in ".git", so strip it if it's there
        parsed_url = urlparse(self.url)
        if parsed_url.path.endswith(".git"):
            self.url = (
                parsed_url.scheme + "://" + parsed_url.netloc + parsed_url.path[:-4]
            )
            if parsed_url.query:
                self.url += "?" + parsed_url.query
            if parsed_url.fragment:
                self.url += "#" + parsed_url.fragment

        if utils.recognized_git_location(self):
            self.metadata_url = construct_git_url(self, "package.xml")
        else:
            self.metadata_url = None
        self.metadata = None
        self.icon = None
        self.cached_icon_filename = ""
        self.macro = None  # Bridge to Gaël Écorchard's macro management class
        self.updated_timestamp = None
        self.installed_version = None

        # Each repo is also a node in a directed dependency graph (referenced by name so
        # they cen be serialized):
        self.requires: Set[str] = set()
        self.blocks: Set[str] = set()

        # And maintains a list of required and optional Python dependencies from metadata.txt
        self.python_requires: Set[str] = set()
        self.python_optional: Set[str] = set()

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
    def from_macro(cls, macro: Macro):
        """Create an Addon object from a Macro wrapper object"""

        if macro.is_installed():
            status = Addon.Status.UNCHECKED
        else:
            status = Addon.Status.NOT_INSTALLED
        instance = Addon(macro.name, macro.url, status, "master")
        instance.macro = macro
        instance.repo_type = Addon.Kind.MACRO
        instance.description = macro.desc
        return instance

    @classmethod
    def from_cache(cls, cache_dict: Dict):
        """Load basic data from cached dict data. Does not include Macro or Metadata information, which must be populated separately."""

        mod_dir = os.path.join(cls.mod_directory, cache_dict["name"])
        if os.path.isdir(mod_dir):
            status = Addon.Status.UNCHECKED
        else:
            status = Addon.Status.NOT_INSTALLED
        instance = Addon(
            cache_dict["name"], cache_dict["url"], status, cache_dict["branch"]
        )

        for key, value in cache_dict.items():
            instance.__dict__[key] = value

        instance.repo_type = Addon.Kind(cache_dict["repo_type"])
        if instance.repo_type == Addon.Kind.PACKAGE:
            # There must be a cached metadata file, too
            cached_package_xml_file = os.path.join(
                instance.cache_directory,
                "PackageMetadata",
                instance.name,
            )
            if os.path.isfile(cached_package_xml_file):
                instance.load_metadata_file(cached_package_xml_file)

        if "requires" in cache_dict:
            instance.requires = set(cache_dict["requires"])
            instance.blocks = set(cache_dict["blocks"])
            instance.python_requires = set(cache_dict["python_requires"])
            instance.python_optional = set(cache_dict["python_optional"])

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
            "python2": self.python2,
            "obsolete": self.obsolete,
            "rejected": self.rejected,
            "requires": list(self.requires),
            "blocks": list(self.blocks),
            "python_requires": list(self.python_requires),
            "python_optional": list(self.python_optional),
        }

    def load_metadata_file(self, file: str) -> None:
        """Read a given metadata file and set it as this object's metadata"""

        if os.path.exists(file):
            metadata = FreeCAD.Metadata(file)
            self.set_metadata(metadata)
        else:
            FreeCAD.Console.PrintLog(f"Internal error: {file} does not exist")

    def set_metadata(self, metadata: FreeCAD.Metadata) -> None:
        """Set the given metadata object as this object's metadata, updating the object's display name
        and package type information to match, as well as updating any dependency information, etc."""

        self.metadata = metadata
        self.display_name = metadata.Name
        self.repo_type = Addon.Kind.PACKAGE
        self.description = metadata.Description
        for url in metadata.Urls:
            if "type" in url and url["type"] == "repository":
                self.url = url["location"]
                if "branch" in url:
                    self.branch = url["branch"]
                else:
                    self.branch = "master"
        self.extract_tags(self.metadata)
        self.extract_metadata_dependencies(self.metadata)

    def version_is_ok(self, metadata) -> bool:
        """Checks to see if the current running version of FreeCAD meets the requirements set by
        the passed-in metadata parameter."""

        dep_fc_min = metadata.FreeCADMin
        dep_fc_max = metadata.FreeCADMax

        fc_major = int(FreeCAD.Version()[0])
        fc_minor = int(FreeCAD.Version()[1])

        try:
            if dep_fc_min and dep_fc_min != "0.0.0":
                required_version = dep_fc_min.split(".")
                if fc_major < int(required_version[0]):
                    return False  # Major version is too low
                if fc_major == int(required_version[0]):
                    if len(required_version) > 1 and fc_minor < int(
                        required_version[1]
                    ):
                        return False  # Same major, and minor is too low
        except ValueError:
            FreeCAD.Console.PrintMessage(
                f"Metadata file for {self.name} has invalid FreeCADMin version info\n"
            )

        try:
            if dep_fc_max and dep_fc_max != "0.0.0":
                required_version = dep_fc_max.split(".")
                if fc_major > int(required_version[0]):
                    return False  # Major version is too high
                if fc_major == int(required_version[0]):
                    if len(required_version) > 1 and fc_minor > int(
                        required_version[1]
                    ):
                        return False  # Same major, and minor is too high
        except ValueError:
            FreeCAD.Console.PrintMessage(
                f"Metadata file for {self.name} has invalid FreeCADMax version info\n"
            )

        return True

    def extract_metadata_dependencies(self, metadata):
        """Read dependency information from a metadata object and store it in this Addon"""

        # Version check: if this piece of metadata doesn't apply to this version of
        # FreeCAD, just skip it.
        if not self.version_is_ok(metadata):
            return

        for dep in metadata.Depend:
            # Simple version for now: eventually support all of the version params...
            self.requires.add(dep["package"])
            FreeCAD.Console.PrintLog(
                f"Package {self.name}: Adding dependency on {dep['package']}\n"
            )
        for dep in metadata.Conflict:
            self.blocks.add(dep["package"])

        # Recurse
        content = metadata.Content
        for _, value in content.items():
            for item in value:
                self.extract_metadata_dependencies(item)

    def verify_url_and_branch(self, url: str, branch: str) -> None:
        """Print diagnostic information for Addon Developers if their metadata is
        inconsistent with the actual fetch location. Most often this is due to using
        the wrong branch name."""

        if self.url != url:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Addon Developer Warning: Repository URL set in package.xml file for addon {} ({}) does not match the URL it was fetched from ({})",
                ).format(self.display_name, self.url, url)
                + "\n"
            )
        if self.branch != branch:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Addon Developer Warning: Repository branch set in package.xml file for addon {} ({}) does not match the branch it was fetched from ({})",
                ).format(self.display_name, self.branch, branch)
                + "\n"
            )

    def extract_tags(self, metadata: FreeCAD.Metadata) -> None:
        """Read the tags from the metadata object"""

        # Version check: if this piece of metadata doesn't apply to this version of
        # FreeCAD, just skip it.
        if not self.version_is_ok(metadata):
            return

        for new_tag in metadata.Tag:
            self.tags.add(new_tag)

        content = metadata.Content
        for _, value in content.items():
            for item in value:
                self.extract_tags(item)

    def contains_workbench(self) -> bool:
        """Determine if this package contains (or is) a workbench"""

        if self.repo_type == Addon.Kind.WORKBENCH:
            return True
        if self.repo_type == Addon.Kind.PACKAGE:
            if self.metadata is None:
                FreeCAD.Console.PrintLog(
                    f"Addon Manager internal error: lost metadata for package {self.name}\n"
                )
                return False
            content = self.metadata.Content
            if not content:
                FreeCAD.Console.PrintLog(
                    f"Package {self.display_name} does not list any content items in its package.xml metadata file.\n"
                )
                return False
            return "workbench" in content
        return False

    def contains_macro(self) -> bool:
        """Determine if this package contains (or is) a macro"""

        if self.repo_type == Addon.Kind.MACRO:
            return True
        if self.repo_type == Addon.Kind.PACKAGE:
            if self.metadata is None:
                FreeCAD.Console.PrintLog(
                    f"Addon Manager internal error: lost metadata for package {self.name}\n"
                )
                return False
            content = self.metadata.Content
            return "macro" in content
        return False

    def contains_preference_pack(self) -> bool:
        """Determine if this package contains a preference pack"""

        if self.repo_type == Addon.Kind.PACKAGE:
            if self.metadata is None:
                FreeCAD.Console.PrintLog(
                    f"Addon Manager internal error: lost metadata for package {self.name}\n"
                )
                return False
            content = self.metadata.Content
            return "preferencepack" in content
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
        store = os.path.join(self.cache_directory, "PackageMetadata")
        self.cached_icon_filename = os.path.join(
            store, self.name, "cached_icon" + file_extension
        )

        return self.cached_icon_filename

    def walk_dependency_tree(self, all_repos, deps):
        """Compute the total dependency tree for this repo (recursive)
        - all_repos is a dictionary of repos, keyed on the name of the repo
        - deps is an Addon.Dependency object encapsulating all the types of dependency
        information that may be needed.
        """

        deps.python_required |= self.python_requires
        deps.python_optional |= self.python_optional
        for dep in self.requires:
            if dep in all_repos:
                if not dep in deps.required_external_addons:
                    deps.required_external_addons.append(all_repos[dep])
                    all_repos[dep].walk_dependency_tree(all_repos, deps)
            else:
                # See if this is an internal workbench:
                if dep.upper().endswith("WB"):
                    real_name = dep[:-2].strip().lower()
                elif dep.upper().endswith("WORKBENCH"):
                    real_name = dep[:-9].strip().lower()
                else:
                    real_name = dep.strip().lower()

                if real_name in INTERNAL_WORKBENCHES:
                    deps.internal_workbenches.add(INTERNAL_WORKBENCHES[real_name])
                else:
                    # Assume it's a Python requirement of some kind:
                    deps.python_required.add(dep)

        for dep in self.blocks:
            if dep in all_repos:
                deps.blockers[dep] = all_repos[dep]

    def status(self):
        """Threadsafe access to the current update status"""
        with self.status_lock:
            return self.update_status

    def set_status(self, status):
        """Threadsafe setting of the update status"""
        with self.status_lock:
            self.update_status = status

    def is_disabled(self):
        """Check to see if the disabling stopfile exists"""

        stopfile = os.path.join(self.mod_directory, self.name, "ADDON_DISABLED")
        return os.path.exists(stopfile)

    def disable(self):
        """Disable this addon from loading when FreeCAD starts up by creating a stopfile"""

        stopfile = os.path.join(self.mod_directory, self.name, "ADDON_DISABLED")
        with open(stopfile, "w", encoding="utf-8") as f:
            f.write(
                "The existence of this file prevents FreeCAD from loading this Addon. To re-enable, delete the file."
            )

    def enable(self):
        """Re-enable loading this addon by deleting the stopfile"""

        stopfile = os.path.join(self.mod_directory, self.name, "ADDON_DISABLED")
        try:
            os.unlink(stopfile)
        except FileNotFoundError:
            pass
