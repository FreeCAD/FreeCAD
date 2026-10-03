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

"""Registry and lifecycle access for the active Forms editor."""

import FreeCADGui as Gui

_active_session = None


def active_form_session(obj=None):
    """Return the live Forms task session, optionally restricted to *obj*."""
    session = _active_session
    if session is None or session.cleaned:
        return None
    return session if obj is None or session.obj == obj else None


def finish_active_form_session():
    """Accept and close the live Forms editor, irrespective of active document."""
    session = active_form_session()
    if session is None:
        return False
    if session.document_edit:
        gui_document = Gui.getDocument(session.obj.Document.Name)
        gui_document.resetEdit()
    else:
        session.accept()
    return True


def set_active_form_session(session):
    """Register an editor, or clear the current registration with None."""
    global _active_session
    _active_session = session
