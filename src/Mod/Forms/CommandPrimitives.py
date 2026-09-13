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

"""Create and reconfigure Forms primitives."""

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore

import Forms
from Forms.feedback import MODELING_ERRORS, report_modeling_error


def _run_creation(document, label, factory):
    """Create a Form and open its task panel as one guarded transaction."""
    document.openTransaction(label)
    try:
        obj = factory()
        from CommandEdit import show_form_task

        if not show_form_task(obj, creating=True):
            raise RuntimeError("Could not open the Forms creation task")
    except MODELING_ERRORS as error:
        if document.getBookedTransactionID() != 0:
            document.abortTransaction()
        return report_modeling_error(label, error)
    except Exception:
        if document.getBookedTransactionID() != 0:
            document.abortTransaction()
        raise
    return True


class _CreatePrimitiveCommand:
    FormType = ""
    Icon = ""
    MenuText = ""
    ToolTip = ""
    Factory = None

    def GetResources(self):
        return {
            "Pixmap": self.Icon,
            "MenuText": App.Qt.translate("Forms_Create", self.MenuText),
            "ToolTip": App.Qt.translate("Forms_Create", self.ToolTip),
        }

    def IsActive(self):
        from Forms.edit import active_form_session

        return (
            App.ActiveDocument is not None
            and (not Gui.Control.activeDialog() or active_form_session() is not None)
            and App.ActiveDocument.getBookedTransactionID() == 0
        )

    def Activated(self):
        document = App.ActiveDocument
        if document is None or document.getBookedTransactionID() != 0:
            return
        _run_creation(
            document,
            App.Qt.translate("Forms_Create", self.MenuText),
            lambda: self.Factory(document),
        )


class CommandCreateBox(_CreatePrimitiveCommand):
    Icon = "Forms_Box"
    MenuText = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Box")
    ToolTip = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Creates a segmented box control cage")
    Factory = staticmethod(Forms.create_box)


class CommandCreateCylinder(_CreatePrimitiveCommand):
    Icon = "Forms_Cylinder"
    MenuText = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Cylinder")
    ToolTip = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Creates a segmented cylinder control cage")
    Factory = staticmethod(Forms.create_cylinder)


class CommandCreateSphere(_CreatePrimitiveCommand):
    Icon = "Forms_Sphere"
    MenuText = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Sphere")
    ToolTip = QtCore.QT_TRANSLATE_NOOP(
        "Forms_Create", "Creates a latitude-longitude sphere control cage"
    )
    Factory = staticmethod(Forms.create_sphere)


class CommandCreateQuadball(_CreatePrimitiveCommand):
    Icon = "Forms_Sphere"
    MenuText = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Quadball")
    ToolTip = QtCore.QT_TRANSLATE_NOOP(
        "Forms_Create", "Creates a cube-derived quadball control cage"
    )
    Factory = staticmethod(Forms.create_quadball)


def _selected_pipe_path():
    selection = Gui.Selection.getSelection()
    if len(selection) != 1:
        return None
    obj = selection[0]
    shape = getattr(obj, "Shape", None)
    if shape is None or shape.isNull() or not shape.Edges or shape.Faces:
        return None
    return obj


def _selected_face_profile():
    selections = Gui.Selection.getSelectionEx()
    if len(selections) != 1:
        return None
    selection = selections[0]
    if len(selection.SubObjects) == 1:
        candidate = selection.SubObjects[0]
        if candidate.ShapeType in ("Face", "Wire"):
            return candidate
        return None
    if selection.SubElementNames:
        return None
    shape = getattr(selection.Object, "Shape", None)
    if shape is None or shape.isNull():
        return None
    if shape.ShapeType in ("Face", "Wire"):
        return shape
    if not shape.Faces and len(shape.Wires) == 1 and shape.Wires[0].isClosed():
        return shape.Wires[0]
    return None


class CommandCreatePipe(_CreatePrimitiveCommand):
    Icon = "Forms_Pipe"
    MenuText = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Pipe")
    ToolTip = QtCore.QT_TRANSLATE_NOOP(
        "Forms_Create", "Creates a Form pipe from the selected sketch, binder, or wire"
    )

    def IsActive(self):
        return super().IsActive() and _selected_pipe_path() is not None

    def Activated(self):
        path = _selected_pipe_path()
        if path is None:
            return
        document = App.ActiveDocument
        def create():
            obj = Forms.create_pipe(document, path)
            path.Visibility = False
            return obj

        _run_creation(
            document,
            App.Qt.translate("Forms_Create", self.MenuText),
            create,
        )


class CommandCreateFace(_CreatePrimitiveCommand):
    Icon = "Forms_Face"
    MenuText = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Face")
    ToolTip = QtCore.QT_TRANSLATE_NOOP(
        "Forms_Create",
        "Creates an open surface control cage; preselect a face or closed sketch to initialize its shape",
    )

    def Activated(self):
        document = App.ActiveDocument
        if document is None or document.getBookedTransactionID() != 0:
            return
        profile = _selected_face_profile()
        _run_creation(
            document,
            App.Qt.translate("Forms_Create", self.MenuText),
            lambda: Forms.create_face(document, profile=profile),
        )


class CommandCreateTorus(_CreatePrimitiveCommand):
    Icon = "Forms_Torus"
    MenuText = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Torus")
    ToolTip = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Creates a periodic torus control cage")
    Factory = staticmethod(Forms.create_torus)


class CommandCreateTube(_CreatePrimitiveCommand):
    Icon = "Forms_Tube"
    MenuText = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Tube")
    ToolTip = QtCore.QT_TRANSLATE_NOOP("Forms_Create", "Creates a closed hollow tube control cage")
    Factory = staticmethod(Forms.create_tube)


Gui.addCommand("Forms_CreateBox", CommandCreateBox())
Gui.addCommand("Forms_CreateCylinder", CommandCreateCylinder())
Gui.addCommand("Forms_CreateSphere", CommandCreateSphere())
Gui.addCommand("Forms_CreateQuadball", CommandCreateQuadball())
Gui.addCommand("Forms_CreatePipe", CommandCreatePipe())
Gui.addCommand("Forms_CreateFace", CommandCreateFace())
Gui.addCommand("Forms_CreateTorus", CommandCreateTorus())
Gui.addCommand("Forms_CreateTube", CommandCreateTube())
