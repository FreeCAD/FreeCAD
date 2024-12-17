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

import Materials

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
    def libraries(self) -> list:
        """Returns a list of libraries managed by this interface

        The list contains a series of tuples describing all libraries managed by
        this module. Each tuple containes the library name, icon, and a boolean to indicate
        if it is a read only library."""
        pass

    @abstractmethod
    def modelLibraries(self) -> list:
        """Returns a list of libraries managed by this interface

        The list contains a series of tuples describing all libraries managed by
        this module. Each tuple containes the library name, icon, and a boolean to indicate
        if it is a read only library.

        This differs from the libraries() function in that it only returns libraries
        containing model objects."""
        pass

    @abstractmethod
    def materialLibraries(self) -> list:
        """Returns a list of libraries managed by this interface

        The list contains a series of tuples describing all libraries managed by
        this module. Each tuple containes the library name, icon, and a boolean to indicate
        if it is a read only library.

        This differs from the libraries() function in that it only returns libraries
        containing material objects."""
        pass

    @abstractmethod
    def createLibrary(self, name: str, icon: str, readOnly: bool) -> None:
        """Create a new library

        Create a new library with the given name"""
        pass

    @abstractmethod
    def renameLibrary(self, oldName: str, newName: str) -> None:
        """Rename an existing library

        Change the name of an existing library"""
        pass

    @abstractmethod
    def changeIcon(self, name: str, icon: str) -> None:
        """Change the library icon

        Change the library icon"""
        pass

    @abstractmethod
    def removeLibrary(self, library: str) -> None:
        """Delete a library and its contents

        Deletes the library and any models or materials it contains"""
        pass

    @abstractmethod
    def libraryModels(self, library: str) -> list:
        """Returns a list of models managed by this library

        Each list entry is a tuple containing the UUID, path, and name of the model"""
        pass

    @abstractmethod
    def libraryMaterials(self, library: str,
                         filter: Materials.MaterialFilter = None,
                         options: Materials.MaterialFilterOptions = None) -> list:
        """Returns a list of materials managed by this library

        Each list entry is a tuple containing the UUID, path, and name of the material"""
        pass

    #
    # Model methods
    #

    @abstractmethod
    def getModel(self, uuid: str) -> Materials.Model:
        pass

    @abstractmethod
    def addModel(self, library: str, path: str, model: Materials.Model) -> None:
        pass

    @abstractmethod
    def migrateModel(self, library: str, path: str, model: Materials.Model) -> None:
        pass

    @abstractmethod
    def updateModel(self, library: str, path: str, model: Materials.Model) -> None:
        pass

    @abstractmethod
    def setModelPath(self, library: str, path: str, model: Materials.Model) -> None:
        pass

    @abstractmethod
    def renameModel(self, library: str, name: str, model: Materials.Model) -> None:
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
        pass

    #
    # Material methods
    #

    @abstractmethod
    def getMaterial(self, uuid: str) -> Materials.Material:
        pass

    @abstractmethod
    def addMaterial(self, library: str, path: str, material: Materials.Material) -> None:
        pass

    @abstractmethod
    def migrateMaterial(self, library: str, path: str, material: Materials.Material) -> None:
        pass

    @abstractmethod
    def updateMaterial(self, library: str, path: str, material: Materials.Material) -> None:
        pass

    @abstractmethod
    def setMaterialPath(self, library: str, path: str, material: Materials.Material) -> None:
        pass

    @abstractmethod
    def renameMaterial(self, library: str, name: str, material: Materials.Material) -> None:
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
        pass
