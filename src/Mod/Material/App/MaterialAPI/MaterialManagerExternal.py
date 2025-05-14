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
    timestamp: str

@dataclass
class MaterialLibraryObjectType:
    UUID: str
    path: str
    name: str

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
        if it is a read only library, and a timestamp that indicates when it was last
        modified."""
        pass

    @abstractmethod
    def modelLibraries(self) -> list[MaterialLibraryType]:
        """Returns a list of libraries managed by this interface

        The list contains a series of tuples describing all libraries managed by
        this module. Each tuple contains the library name, icon, and a boolean to indicate
        if it is a read only library, and a timestamp that indicates when it was last
        modified.

        This differs from the libraries() function in that it only returns libraries
        containing model objects."""
        pass

    @abstractmethod
    def materialLibraries(self) -> list[MaterialLibraryType]:
        """Returns a list of libraries managed by this interface

        The list contains a series of tuples describing all libraries managed by
        this module. Each tuple contains the library name, icon, and a boolean to indicate
        if it is a read only library, and a timestamp that indicates when it was last
        modified.

        This differs from the libraries() function in that it only returns libraries
        containing material objects."""
        pass

    @abstractmethod
    def getLibrary(self, name: str) -> tuple:
        """Get the library

        Retrieve the library with the given name"""
        pass

    @abstractmethod
    def createLibrary(self, name: str, icon: bytes, readOnly: bool) -> None:
        """Create a new library

        Create a new library with the given name"""
        pass

    @abstractmethod
    def renameLibrary(self, oldName: str, newName: str) -> None:
        """Rename an existing library

        Change the name of an existing library"""
        pass

    @abstractmethod
    def changeIcon(self, name: str, icon: bytes) -> None:
        """Change the library icon

        Change the library icon"""
        pass

    @abstractmethod
    def removeLibrary(self, library: str) -> None:
        """Delete a library and its contents

        Deletes the library and any models or materials it contains"""
        pass

    @abstractmethod
    def libraryModels(self, library: str) -> list[MaterialLibraryObjectType]:
        """Returns a list of models managed by this library

        Each list entry is a tuple containing the UUID, path, and name of the model"""
        pass

    @abstractmethod
    def libraryMaterials(self, library: str,
                         filter: Materials.MaterialFilter = None,
                         options: Materials.MaterialFilterOptions = None) -> list[MaterialLibraryObjectType]:
        """Returns a list of materials managed by this library

        Each list entry is a tuple containing the UUID, path, and name of the material"""
        pass

    #
    # Model methods
    #

    @abstractmethod
    def getModel(self, uuid: str) -> Materials.Model:
        """Retrieve a model given its UUID"""
        pass

    @abstractmethod
    def addModel(self, library: str, path: str, model: Materials.Model) -> None:
        """Add a model to a library in the given folder.

        This will throw a DatabaseModelExistsError exception if the model already exists."""
        pass

    @abstractmethod
    def migrateModel(self, library: str, path: str, model: Materials.Model) -> None:
        """Add the model to the library.

        If the model already exists, then no action is performed."""
        pass

    @abstractmethod
    def updateModel(self, library: str, path: str, model: Materials.Model) -> None:
        """Update the given model"""
        pass

    @abstractmethod
    def setModelPath(self, library: str, path: str, model: Materials.Model) -> None:
        """Change the model path within the library"""
        pass

    @abstractmethod
    def renameModel(self, library: str, name: str, model: Materials.Model) -> None:
        """Change the model name"""
        pass

    @abstractmethod
    def moveModel(self, library: str, path: str, model: Materials.Model) -> None:
        """Move a model across libraries

        Move the model to the desired path in a different library. This should also
        remove the model from the old library if that library is managed by this
        interface"""
        pass

    @abstractmethod
    def removeModel(self, model: Materials.Model) -> None:
        """Remove the model from the library"""
        pass

    #
    # Material methods
    #

    @abstractmethod
    def getMaterial(self, uuid: str) -> Materials.Material:
        """ Retrieve a material given its UUID """
        pass

    @abstractmethod
    def addMaterial(self, library: str, path: str, material: Materials.Material) -> None:
        """Add a material to a library in the given folder.

        This will throw a DatabaseMaterialExistsError exception if the model already exists."""
        pass

    @abstractmethod
    def migrateMaterial(self, library: str, path: str, material: Materials.Material) -> None:
        """Add the material to the library in the given folder.

        If the material already exists, then no action is performed."""
        pass

    @abstractmethod
    def updateMaterial(self, library: str, path: str, material: Materials.Material) -> None:
        """Update the given material"""
        pass

    @abstractmethod
    def setMaterialPath(self, library: str, path: str, material: Materials.Material) -> None:
        """Change the material path within the library"""
        pass

    @abstractmethod
    def renameMaterial(self, library: str, name: str, material: Materials.Material) -> None:
        """Change the material name"""
        pass

    @abstractmethod
    def moveMaterial(self, library: str, path: str, material: Materials.Material) -> None:
        """Move a material across libraries

        Move the material to the desired path in a different library. This should also
        remove the material from the old library if that library is managed by this
        interface"""
        pass

    @abstractmethod
    def removeMaterial(self, material: Materials.Material) -> None:
        """Remove the material from the library"""
        pass
