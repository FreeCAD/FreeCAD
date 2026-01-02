# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.PyObjectBase import PyObjectBase


class ApplicationDirectories(PyObjectBase):
    """
    Provides access to the directory versioning methods of its C++ counterpart.

    These are all static methods, so no instance is needed. The main methods of
    this class are migrateAllPaths(), usingCurrentVersionConfig(), and versionStringForPath().
    """

    @staticmethod
    def usingCurrentVersionConfig(path: str, /) -> bool:
        """
        Determine if a given config path is for the current version of the program.

        Args:
            path: The path to check.
        """
        ...

    @staticmethod
    def migrateAllPaths(paths: list[str], /) -> None:
        """
        Migrate a set of versionable configuration directories from the given paths to a new version.

        The new version's directories cannot exist yet, and the old ones *must* exist.
        If the old paths are themselves versioned, then the new paths will be placed at the same
        level in the directory structure (e.g., they will be siblings of each entry in paths).
        If paths are NOT versioned, the new (versioned) copies will be placed *inside* the
        original paths.

        If the list contains the same path multiple times, the duplicates are ignored, so it is safe
        to pass the same path multiple times.

        Args:
            paths: List of paths to migrate from.

        Examples:
            Running FreeCAD 1.1, /usr/share/FreeCAD/Config/ -> /usr/share/FreeCAD/Config/v1-1/
            Running FreeCAD 1.1, /usr/share/FreeCAD/Config/v1-1 -> raises exception, path exists
            Running FreeCAD 1.2, /usr/share/FreeCAD/Config/v1-1/ -> /usr/share/FreeCAD/Config/v1-2/
        """
        ...

    @staticmethod
    def versionStringForPath(major: int, minor: int, /) -> str:
        """
        Given a major and minor version number, return the name for a versioned subdirectory.

        Args:
            major: Major version number.
            minor: Minor version number.

        Returns:
            A string that can be used as the name for a versioned subdirectory.
            Only returns the version string, not the full path.
        """
        ...

    @staticmethod
    def isVersionedPath(startingPath: str, /) -> bool:
        """
        Determine if a given path is versioned.

        That is, if its last component contains something that this class would have
        created as a versioned subdirectory).

        Args:
            startingPath: The path to check.

        Returns:
            True for any path that the *current* version of FreeCAD would recognize as versioned,
            and False for either something that is not versioned, or something that is versioned
            but for a later version of FreeCAD.
        """
        ...

    @staticmethod
    def mostRecentAvailableConfigVersion(startingPath: str, /) -> str:
        """
        Given a base path that is expected to contain versioned subdirectories, locate the
        directory name (*not* the path, only the final component, the version string itself)
        corresponding to the most recent version of the software, up to and including the current
        running version, but NOT exceeding it -- any *later* version whose directories exist
        in the path is ignored. See also mostRecentConfigFromBase().

        Args:
            startingPath: The path to check.

        Returns:
            Most recent available dir name (not path).
        """
        ...

    @staticmethod
    def mostRecentConfigFromBase(startingPath: str, /) -> str:
        """
        Given a base path that is expected to contained versioned subdirectories, locate the
        directory corresponding to the most recent version of the software, up to and including
        the current version, but NOT exceeding it. Returns the complete path, not just the final
        component. See also mostRecentAvailableConfigVersion().

        Args:
            startingPath: The base path to check.

        Returns:
            Most recent available full path (not just dir name).
        """
        ...

    @staticmethod
    def migrateConfig(oldPath: str, newPath: str, /) -> None:
        """
        A utility method to copy all files and directories from oldPath to newPath, handling the
        case where newPath might itself be a subdirectory of oldPath (and *not* attempting that
        otherwise-recursive copy).

        Args:
            oldPath: Path from.
            newPath: Path to.
        """
        ...
