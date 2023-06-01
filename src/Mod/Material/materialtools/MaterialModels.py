# ***************************************************************************
# *   Copyright (c) 2023 David Carter <dcarter@dvidcarter.ca>               *
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

__title__ = "material model utilities"
__author__ = "David Carter"
__url__ = "http://www.freecad.org"

import os
from os.path import join
import io
from pathlib import Path
import yaml

import FreeCAD


unicode = str

__models = {}

class MaterialModel:
    pass

def _scanFolder(folder):
    print("Scanning folder '{0}'".format(folder.absolute()))
    for child in folder.iterdir():
        if child.is_dir():
            _scanFolder(child)
        else:
            try:
                if child.suffix.lower() == ".yml":
                    stream = open(child.absolute(), "r")
                    model = yaml.safe_load(stream)
                    print(model)
                else:
                    print("Extension '{0}'".format(child.suffix.lower()))
            except Exception as ex:
                print("Unable to load '{0}'".format(child.absolute()))

def _scanModels(libraries):
    __models = {} # Clear the current library
    print("_scanModels")
    print(libraries)
    for library in libraries:
        _scanFolder(Path(library))

def getPreferredSaveDirectory():
    pass

def getModelLibraries():

    libraries = []

    # TODO: Expand beyond the standard models as we do for material paths
    path = Path(FreeCAD.getResourceDir()) / "Mod/Material/Resources/Models"
    libraries.append(path)

    _scanModels(libraries)

    return libraries

def getModel(name, UUID):
    """
        Retrieve the specified model. Where UUID is None, the last loaded match will be returned
    """
    pass

def getModelFromPath(path):
    """
        Retrieve the model at the specified path
    """
    pass

def getConsolidatedModel(name, UUID):
    """
        Retrieve the specified model. Where UUID is None, the last loaded match will be returned.

        The consolidated model dereferences any inherited models returning a final model.
    """
    pass

def getConsolidaatedModelFromPath(path):
    """
        Retrieve the model at the specified path.

        The consolidated model dereferences any inherited models returning a final model.
    """
    pass

def saveModel(model, path):
    """
        Write the model to the specified path
    """