# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""FreeCAD file-import entry points for Blender subdivision cages."""

from pathlib import Path

import FreeCAD as App

from Forms.blend_export import BlendExportError, export_file
from Forms.blend_import import BlendImportError, import_file


def _show_error(message):
    App.Console.PrintError(f"Blender Forms import: {message}\n")
    if App.GuiUp:
        from PySide import QtWidgets

        QtWidgets.QMessageBox.critical(
            None,
            App.Qt.translate("Forms_Import", "Cannot import Blender file"),
            str(message),
        )


def _show_export_error(message):
    App.Console.PrintError(f"Blender Forms export: {message}\n")
    if App.GuiUp:
        from PySide import QtWidgets

        QtWidgets.QMessageBox.critical(
            None,
            App.Qt.translate("Forms_Export", "Cannot export Blender file"),
            str(message),
        )


def _show_export_rejected(rejected):
    if not rejected or not App.GuiUp:
        return
    from PySide import QtWidgets

    QtWidgets.QMessageBox.warning(
        None,
        App.Qt.translate("Forms_Export", "Some FreeCAD objects were skipped"),
        "\n".join(f"• {reason}" for reason in rejected),
    )


def _show_rejected(rejected):
    if not rejected or not App.GuiUp:
        return
    from PySide import QtWidgets

    QtWidgets.QMessageBox.warning(
        None,
        App.Qt.translate("Forms_Import", "Some Blender objects were skipped"),
        "\n".join(f"• {reason}" for reason in rejected),
    )


def open(filename):
    """Open *filename* in a new FreeCAD document."""
    label = Path(filename).stem or "BlenderImport"
    document = App.newDocument(label=label)
    try:
        _created, rejected = import_file(filename, document)
    except BlendImportError as error:
        App.closeDocument(document.Name)
        _show_error(error)
        return None
    _show_rejected(rejected)
    return document


def insert(filename, docname):
    """Insert *filename* into an existing FreeCAD document."""
    document = App.getDocument(docname)
    try:
        _created, rejected = import_file(filename, document)
    except BlendImportError as error:
        _show_error(error)
        return None
    _show_rejected(rejected)
    return document


def export(objects, filename):
    """Export selected FreeCAD objects to a Blender document."""
    try:
        rejected = export_file(objects, filename)
    except BlendExportError as error:
        _show_export_error(error)
        return None
    _show_export_rejected(rejected)
    return filename
