# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 Yorik van Havre <yorik@uncreated.net>              *
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
# ***************************************************************************

__title__  = "FreeCAD SweetHome3D Importer"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

## @package importSH3D
#  \ingroup ARCH
#  \brief SH3D (SweetHome3D) file format importer
#
#  This module provides tools to import SH3D files created from Sweet Home 3D.

import os
import zipfile

import FreeCAD
from FreeCAD import Base


DEBUG = True


def open(filename):
    "called when freecad wants to open a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def insert(filename,docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def read(filename):
    "reads the file and creates objects in the active document"

    import BIM.importers.importSH3DHelper
    if DEBUG:
        from importlib import reload
        reload(BIM.importers.importSH3DHelper)

    pi = Base.ProgressIndicator()
    try:
        importer = BIM.importers.importSH3DHelper.SH3DImporter(pi)
        importer.import_sh3d_from_filename(filename)
    finally:
        pi.stop()

    FreeCAD.ActiveDocument.recompute()
