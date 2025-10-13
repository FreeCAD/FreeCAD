from Base.PyObjectBase import PyObjectBase
from typing import List


class ApplicationDirectories(PyObjectBase):
    """
    App.ApplicationDirectories class.

    For the time being this class only provides access to the directory versioning methods of its
    C++ counterpart. These are all static methods, so no instance is needed. The main methods of
    this class are migrateAllPaths(), usingCurrentVersionConfig(), and versionStringForPath().

    Author: Chris Hennes (chennes@pioneerlibrarysystem.org)
    Licence: LGPL-2.1-or-later
    DeveloperDocu: ApplicationDirectories
    """

    @staticmethod
    def usingCurrentVersionConfig(path:str) -> bool:
        """
        usingCurrentVersionConfig(path)

        Determine if a given config path is for the current version of the program

        path  : the path to check
        """
        ...

    @staticmethod
    def migrateAllPaths(paths: List[str]) -> None:
        """
        migrateAllPaths(paths)

        Migrate a set of versionable configuration directories from the given paths to a new
        version. The new version's directories cannot exist yet, and the old ones *must* exist.
        If the old paths are themselves versioned, then the new paths will be placed at the same
        level in the directory structure (e.g., they will be siblings of each entry in paths).
        If paths are NOT versioned, the new (versioned) copies will be placed *inside* the
        original paths.

        If the list contains the same path multiple times, the duplicates are ignored, so it is safe
        to pass the same path multiple times.

        Examples:
            Running FreeCAD 1.1, /usr/share/FreeCAD/Config/ -> /usr/share/FreeCAD/Config/v1-1/
            Running FreeCAD 1.1, /usr/share/FreeCAD/Config/v1-1 -> raises exception, path exists
            Running FreeCAD 1.2, /usr/share/FreeCAD/Config/v1-1/ -> /usr/share/FreeCAD/Config/v1-2/
        """
        ...

    @staticmethod
    def versionStringForPath(major:int, minor:int) -> str:
        """
        versionStringForPath(major, minor) -> str

        Given a major and minor version number, return a string that can be used as the name for a
        versioned subdirectory. Only returns the version string, not the full path.
        """
        ...

    @staticmethod
    def isVersionedPath(startingPath:str) -> bool:
        """
        isVersionedPath(startingPath) -> bool

        Determine if a given path is versioned (that is, if its last component contains
        something that this class would have created as a versioned subdirectory). Returns true
        for any path that the *current* version of FreeCAD would recognized as versioned, and false
        for either something that is not versioned, or something that is versioned but for a later
        version of FreeCAD.
        """
        ...

    @staticmethod
    def mostRecentAvailableConfigVersion(startingPath:str) -> str:
        """
        mostRecentAvailableConfigVersion(startingPath) -> str

        Given a base path that is expected to contain versioned subdirectories, locate the
        directory name (*not* the path, only the final component, the version string itself)
        corresponding to the most recent version of the software, up to and including the current
        running version, but NOT exceeding it -- any *later* version whose directories exist
        in the path is ignored. See also mostRecentConfigFromBase().
        """
        ...

    @staticmethod
    def mostRecentConfigFromBase(startingPath: str) -> str:
        """
        mostRecentConfigFromBase(startingPath) -> str

        Given a base path that is expected to contained versioned subdirectories, locate the
        directory corresponding to the most recent version of the software, up to and including
        the current version, but NOT exceeding it. Returns the complete path, not just the final
        component. See also mostRecentAvailableConfigVersion().
        """
        ...

    @staticmethod
    def migrateConfig(oldPath: str, newPath: str) -> None:
        """
        migrateConfig(oldPath, newPath) -> None

        A utility method to copy all files and directories from oldPath to newPath, handling the
        case where newPath might itself be a subdirectory of oldPath (and *not* attempting that
        otherwise-recursive copy).
        """
        ...
