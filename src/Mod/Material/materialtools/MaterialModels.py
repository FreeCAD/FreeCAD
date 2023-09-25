# ***************************************************************************
# *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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
# **************************************************************************/

__title__ = "material model utilities"
__author__ = "David Carter"
__url__ = "http://www.freecad.org"

import os
import io
from pathlib import Path
import yaml

import FreeCAD


unicode = str

__models = {}
__modelsByPath = {}

def _dereference(parent, child):
    # Add the child parameters to the parent
    parentModel = parent["model"]
    parentBase = parent["base"]
    childModel = child["model"]
    childBase = child["base"]
    for name, value in childModel[childBase].items():
        if name not in ["Name", "UUID", "URL", "Description", "DOI", "Inherits"] and \
            name not in parentModel[parentBase]: # Don't add if it's already there
            parentModel[parentBase][name] = value

    print("dereferenced:")
    print(parentModel)

def _dereferenceInheritance(data):
    if not data["dereferenced"]:
        data["dereferenced"] = True # Prevent recursion loops

        model = data["model"]
        base = data["base"]
        if "Inherits" in model[base]:
            print("Model '{0}' inherits from:".format(data["name"]))
            for parent in model[base]["Inherits"]:
                print("\t'{0}'".format(parent))
                print("\t\t'{0}'".format(parent.keys()))
                print("\t\t'{0}'".format(parent["UUID"]))

                # This requires that all models have already been loaded undereferenced
                child = __models[parent["UUID"]]
                if child is not None:
                    _dereference(data, child)

def _dereferenceAll():
    for data in __models.values():
        _dereferenceInheritance(data)

def _scanFolder(folder):
    print("Scanning folder '{0}'".format(folder.absolute()))
    for child in folder.iterdir():
        if child.is_dir():
            _scanFolder(child)
        else:
            if child.suffix.lower() == ".yml":
                data = getModelFromPath(child)

                if data is not None:
                    __models[data["uuid"]] = data
                    __modelsByPath[data["path"]] = data
                    # print(data["model"])
            else:
                print("Extension '{0}'".format(child.suffix.lower()))

def _scanModels(libraries):
    __models = {} # Clear the current library
    __modelsByPath = {}
    print("_scanModels")
    print(libraries)
    for library in libraries:
        _scanFolder(Path(library))

    # Satisfy aany inheritances
    _dereferenceAll()

def getPreferredSaveDirectory():
    pass

def getModelLibraries():

    libraries = []

    # TODO: Expand beyond the standard models as we do for material paths
    path = Path(FreeCAD.getResourceDir()) / "Mod/Material/Resources/Models"
    libraries.append(path)

    _scanModels(libraries)

    return libraries

def getModel(uuid):
    """
        Retrieve the specified model.
    """
    if len(__models) < 1:
        getModelLibraries()

    if uuid not in __models:
        return None
    return __models[uuid]

def getModelFromPath(filePath):
    """
        Retrieve the model at the specified path.

        This may not need public exposure?
    """
    try:
        path = Path(filePath)
        stream = open(path.absolute(), "r")
        model = yaml.safe_load(stream)

        base = "Model"
        if "AppearanceModel" in model:
            base = "AppearanceModel"

        uuid = model[base]["UUID"]
        name = model[base]["Name"]

        data = {}
        data["base"] = base
        data["name"] = name
        data["path"] = path.absolute()
        data["uuid"] = uuid
        data["model"] = model
        data["dereferenced"] = False
        return data
    except Exception as ex:
        print("Unable to load '{0}'".format(path.absolute()))
        print(ex)

    return None

def saveModel(model, path):
    """
        Write the model to the specified path
    """