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

"""Command for editing a Forms control cage."""

import FreeCAD as App
import FreeCADGui as Gui

from Forms.feedback import MODELING_ERRORS, report_modeling_error


def selected_form():
    selection = Gui.Selection.getSelection()
    if len(selection) != 1:
        return None
    obj = selection[0]
    return obj if getattr(obj, "FormType", "").startswith("Forms::") else None


def show_form_task(obj, creating=False):
    if obj is None:
        return False
    from Forms.edit import active_form_session, finish_active_form_session

    session = active_form_session()
    if session is not None:
        if session.obj == obj:
            return True
        finish_active_form_session()
    if Gui.Control.activeDialog():
        return False
    Gui.Selection.clearSelection()
    if not creating:
        proxy = obj.ViewObject.Proxy
        prepare = getattr(proxy, "_prepare_edit_workbench", None)
        if prepare is not None:
            prepare()
        Gui.getDocument(obj.Document.Name).setEdit(obj, 0)
        return True
    from Forms.edit import FormEditSession

    FormEditSession(obj).start()
    return True


class CommandEditForm:
    def GetResources(self):
        return {
            "Pixmap": "Std_TransformManip",
            "MenuText": App.Qt.translate("Forms_Edit", "Edit Form"),
            "ToolTip": App.Qt.translate(
                "Forms_Edit", "Edits control points with the transform manipulator"
            ),
        }

    def IsActive(self):
        from Forms.edit import active_form_session

        return (
            App.ActiveDocument is not None
            and (not Gui.Control.activeDialog() or active_form_session() is not None)
            and selected_form() is not None
        )

    def Activated(self):
        try:
            show_form_task(selected_form())
        except MODELING_ERRORS as error:
            report_modeling_error(App.Qt.translate("Forms_Edit", "Edit Form"), error)


Gui.addCommand("Forms_Edit", CommandEditForm())
