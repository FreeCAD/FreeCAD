# ***************************************************************************
# *   Copyright (c) 2024 David Carter <dcarter@davidcarter.ca>              *
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

__author__ = "David Carter"
__url__ = "https://www.davesrocketshop.com"

from abc import ABC, abstractmethod
from dataclasses import dataclass

import Materials

@dataclass
class MaterialLibraryType:
    name: str
    icon: bytes
    readOnly: bool

@dataclass
class MaterialLibraryObjectType:
    UUID: str
    path: str
    name: str

@dataclass
class ModelObjectType:
    libraryName: str
    model: Materials.Model

@dataclass
class MaterialObjectType:
    libraryName: str
    material: Materials.Material

class MaterialManagerExternal(ABC):
    """Abstract base class for all external material managers

    Any external interface should be derivedfrom this base class."""

    @classmethod
    def APIVersion(cls) -> tuple:
        """Returns a tuple of 3 integers describing the API version

        The version returned should be the latest supported version. This method
        allows the interface to use older modules."""
        return (1, 0, 0)

    #
    # Library methods
    #

    @abstractmethod
    def libraries(self) -> list[MaterialLibraryType]:
        """Returns a list of libraries managed by this interface

        The list contains a series of tuples describing all libraries managed by
        this module. Each tuple contains the library name, icon, a boolean to indicate
        if it is a read only library."""

    @abstractmethod
    def modelLibraries(self) -> list[MaterialLibraryType]:
        """Returns a list of libraries managed by this interface

        The list contains a series of tuples describing all libraries managed by
        this module. Each tuple contains the library name, icon, and a boolean to indicate
        if it is a read only library.

        This differs from the libraries() function in that it only returns libraries
        containing model objects."""

    @abstractmethod
    def materialLibraries(self) -> list[MaterialLibraryType]:
        """Returns a list of libraries managed by this interface

        The list contains a series of tuples describing all libraries managed by
        this module. Each tuple contains the library name, icon, and a boolean to indicate
        if it is a read only library.

        This differs from the libraries() function in that it only returns libraries
        containing material objects."""

    @abstractmethod
    def getLibrary(self, libraryName: str) -> MaterialLibraryType:
        """Get the library

        Retrieve the library with the given name"""

    @abstractmethod
    def createLibrary(self, libraryName: str, icon: bytes, readOnly: bool) -> None:
        """Create a new library

        Create a new library with the given name."""

    @abstractmethod
    def renameLibrary(self, oldName: str, newName: str) -> None:
        """Rename an existing library

        Change the name of an existing library"""

    @abstractmethod
    def changeIcon(self, libraryName: str, icon: bytes) -> None:
        """Change the library icon

        Change the library icon"""

    @abstractmethod
    def removeLibrary(self, libraryName: str) -> None:
        """Delete a library and its contents

        Deletes the library and any models or materials it contains"""

    @abstractmethod
    def libraryModels(self, libraryName: str) -> list[MaterialLibraryObjectType]:
        """Returns a list of models managed by this library

        Each list entry is a tuple containing the UUID, path, and name of the model"""

    @abstractmethod
    def libraryMaterials(self, libraryName: str,
                         materialFilter: Materials.MaterialFilter = None,
                         options: Materials.MaterialFilterOptions = None) -> \
                                list[MaterialLibraryObjectType]:
        """Returns a list of materials managed by this library

        Each list entry is a tuple containing the UUID, path, and name of the material"""

    @abstractmethod
    def libraryFolders(self, libraryName: str) -> list[str]:
        """Returns a list of folders managed by this library

        This will return a list of all folders in the library including empty folders"""

    #
    # Folder methods
    #

    @abstractmethod
    def createFolder(self, libraryName: str, path: str) -> None:
        """Create a new folder in the given library"""

    @abstractmethod
    def renameFolder(self, libraryName: str, oldPath: str, newPath: str) -> None:
        """Rename the folder"""

    @abstractmethod
    def deleteRecursive(self, libraryName: str, path: str) -> None:
        """Delete the folder and all of its contents"""

    #
    # Model methods
    #

    @abstractmethod
    def getModel(self, uuid: str) -> ModelObjectType:
        """Retrieve a model given its UUID"""

    @abstractmethod
    def addModel(self, libraryName: str, path: str, model: Materials.Model) -> None:
        """Add a model to a library in the given folder. The folder path is relative to
        the library and will be created if it doesn't already exist.

        This will throw a DatabaseModelExistsError exception if the model already exists."""

    @abstractmethod
    def migrateModel(self, libraryName: str, path: str, model: Materials.Model) -> None:
        """Add the model to the library.

        If the model already exists, then no action is performed."""

    @abstractmethod
    def updateModel(self, libraryName: str, path: str, model: Materials.Model) -> None:
        """Update the given model"""

    @abstractmethod
    def setModelPath(self, libraryName: str, path: str, uuid: str) -> None:
        """Change the model path within the library"""

    @abstractmethod
    def renameModel(self, libraryName: str, name: str, uuid: str) -> None:
        """Change the model name"""

    @abstractmethod
    def moveModel(self, libraryName: str, path: str, uuid: str) -> None:
        """Move a model across libraries

        Move the model to the desired path in a different library. This should also
        remove the model from the old library if that library is managed by this
        interface"""

    @abstractmethod
    def removeModel(self, uuid: str) -> None:
        """Remove the model from the library"""

    #
    # Material methods
    #

    @abstractmethod
    def getMaterial(self, uuid: str) -> MaterialObjectType:
        """ Retrieve a material given its UUID """

    @abstractmethod
    def addMaterial(self, libraryName: str, path: str, material: Materials.Material) -> None:
        """Add a material to a library in the given folder. The folder path is relative to
        the library and will be created if it doesn't already exist.

        This will throw a DatabaseMaterialExistsError exception if the model already exists."""

    @abstractmethod
    def migrateMaterial(self, libraryName: str, path: str, material: Materials.Material) -> None:
        """Add the material to the library in the given folder. The folder path is relative to
        the library and will be created if it doesn't already exist.

        If the material already exists, then no action is performed."""

    @abstractmethod
    def updateMaterial(self, libraryName: str, path: str, material: Materials.Material) -> None:
        """Update the given material"""

    @abstractmethod
    def setMaterialPath(self, libraryName: str, path: str, uuid: str) -> None:
        """Change the material path within the library"""

    @abstractmethod
    def renameMaterial(self, libraryName: str, name: str, uuid: str) -> None:
        """Change the material name"""

    @abstractmethod
    def moveMaterial(self, libraryName: str, path: str, uuid: str) -> None:
        """Move a material across libraries

        Move the material to the desired path in a different library. This should also
        remove the material from the old library if that library is managed by this
        interface"""

    @abstractmethod
    def removeMaterial(self, uuid: str) -> None:
        """Remove the material from the library"""
