# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   Copyright (c) 2022 FreeCAD Project Association                             #
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Helper for importers.
"""

import FreeCAD
import importlib

def OpenInsertObject(importerName, objectPath, importMethod, docName = ""):
    """Load the importer module and call the import method with suitable arguments."""
    try:
        importArgs = []
        importKwargs = {}

        importerModule = importlib.import_module(importerName)

        if docName:
            importArgs.append(docName)
        if hasattr(importerModule, "importOptions"):
            importKwargs["options"] = importerModule.importOptions(objectPath)

        getattr(importerModule, importMethod)(objectPath, *importArgs, **importKwargs)

    except FreeCAD.Base.AbortIOException:
        FreeCAD.Console.PrintLog("Import aborted by the user.\n")
