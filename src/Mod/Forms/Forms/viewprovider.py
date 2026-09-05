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

"""Shared native BRep presentation and Forms edit-session ownership."""

import FreeCAD as App
from .feedback import MODELING_ERRORS, report_modeling_error
from .topology import cage_edges

FORMS_WORKBENCH = "FormsWorkbench"
ACTIVE_FORM_KEY = "form"


class ViewProviderForm:
    """Native BRep presentation with a Python control-cage overlay."""

    IconName = "Forms_Box.svg"

    def __init__(self, view_object):
        self._ensure_view_properties(view_object)
        self._cage_coordinates = None
        self._draw_style = None
        view_object.Proxy = self

    @staticmethod
    def _ensure_view_properties(view_object):
        if "CageColor" not in view_object.PropertiesList:
            view_object.addProperty(
                "App::PropertyColor", "CageColor", "Control Cage", "Cage line color"
            )
            view_object.CageColor = (0.20, 0.80, 1.00)
        if "CageLineWidth" not in view_object.PropertiesList:
            view_object.addProperty(
                "App::PropertyFloatConstraint",
                "CageLineWidth",
                "Control Cage",
                "Cage line width",
            )
            view_object.CageLineWidth = (2.0, 1.0, 10.0, 0.5)
        if "ShowControlCage" not in view_object.PropertiesList:
            view_object.addProperty(
                "App::PropertyBool",
                "ShowControlCage",
                "Control Cage",
                "Show the control cage over the generated shape",
            )
            view_object.ShowControlCage = False
        if "SelectionPickRadius" not in view_object.PropertiesList:
            view_object.addProperty(
                "App::PropertyIntegerConstraint",
                "SelectionPickRadius",
                "Control Cage",
                "Screen-space pick radius used for vertices and edges while editing",
            )
            view_object.SelectionPickRadius = (16, 5, 30, 1)

    def attach(self, view_object):
        from pivy import coin

        # Python constructors are not called when a proxy is restored from a
        # document, so attach is also the migration point for view properties.
        self.detach()
        self._ensure_view_properties(view_object)
        self.ViewObject = view_object
        self._coin = coin
        root = coin.SoSeparator()
        self._cage_switch = coin.SoSwitch()
        cage = coin.SoSeparator()
        self._cage_coordinates = coin.SoCoordinate3()
        self._draw_style = coin.SoDrawStyle()
        self._lines = coin.SoIndexedLineSet()
        color = coin.SoBaseColor()

        cage.addChild(self._draw_style)
        cage.addChild(color)
        cage.addChild(self._cage_coordinates)
        cage.addChild(self._lines)
        self._color = color
        self._cage_switch.addChild(cage)
        root.addChild(self._cage_switch)

        # Keep the cage outside the display-mode switch. The inherited Part view
        # provider renders the actual Shape (and its selectable Faces/Edges),
        # while this lightweight overlay remains available in every native mode.
        view_object.RootNode.addChild(root)
        self._overlay_root = root
        self.updateData(view_object.Object, "ControlPoints")
        self.onChanged(view_object, "CageColor")
        self.onChanged(view_object, "ShowControlCage")

    def detach(self):
        """Remove the Python-owned overlay before reattach or object deletion."""
        view_object = getattr(self, "ViewObject", None)
        root = getattr(self, "_overlay_root", None)
        if view_object is not None and root is not None:
            try:
                view_object.RootNode.removeChild(root)
            except (AttributeError, RuntimeError):
                pass
        self._overlay_root = None
        self._cage_switch = None
        self._cage_coordinates = None
        self._draw_style = None
        self._lines = None
        self._color = None

    def updateData(self, obj, prop):
        if prop not in ("ControlPoints", "ControlFaces") or getattr(self, "_cage_coordinates", None) is None:
            return
        try:
            points = [(point.x, point.y, point.z) for point in obj.ControlPoints]
            faces = [tuple(int(index) for index in face.split()) for face in obj.ControlFaces]
            valid = (
                bool(points)
                and bool(faces)
                and all(len(face) >= 2 for face in faces)
                and min(min(face) for face in faces) >= 0
                and max(max(face) for face in faces) < len(points)
            )
        except (AttributeError, TypeError, ValueError):
            valid = False
        if not valid:
            self._cage_coordinates.point.setNum(0)
            self._lines.coordIndex.setNum(0)
            return
        indices = []
        for start, end in cage_edges(faces):
            indices.extend((start, end, -1))
        self._cage_coordinates.point.setValues(0, len(points), points)
        self._lines.coordIndex.setValues(0, len(indices), indices)

    def onChanged(self, view_object, prop):
        if (
            prop == "CageColor"
            and "CageColor" in view_object.PropertiesList
            and getattr(self, "_color", None) is not None
        ):
            self._color.rgb = tuple(view_object.CageColor)[:3]
        elif (
            prop == "CageLineWidth"
            and "CageLineWidth" in view_object.PropertiesList
            and self._draw_style is not None
        ):
            self._draw_style.lineWidth = view_object.CageLineWidth
        elif (
            prop == "ShowControlCage"
            and "ShowControlCage" in view_object.PropertiesList
            and getattr(self, "_cage_switch", None) is not None
        ):
            visible = self._coin.SO_SWITCH_ALL
            hidden = self._coin.SO_SWITCH_NONE
            self._cage_switch.whichChild = visible if view_object.ShowControlCage else hidden

    def getDisplayModes(self, _view_object):
        return []

    def getDefaultDisplayMode(self):
        return "Flat Lines"

    def setDisplayMode(self, mode):
        return mode

    def getIcon(self):
        return App.getResourceDir() + "Mod/Forms/Resources/icons/" + self.IconName

    def allowBoxElementSelection(self, _view_object):
        """Use FreeCAD's normal LMB-drag element selection while editing."""
        return True

    def doubleClicked(self, view_object):
        if not App.GuiUp:
            return False
        import FreeCADGui as Gui
        from .edit import active_form_session, finish_active_form_session

        # Tree.cpp opens the view provider's default "Edit" transaction before
        # invoking doubleClicked(). Forms uses one transaction per modeling
        # action, so discard that automatic wrapper immediately.
        document = view_object.Object.Document
        if document.getBookedTransactionID() != 0:
            document.abortTransaction()
        session = active_form_session()
        if session is not None:
            if session.obj == view_object.Object:
                return True
            finish_active_form_session()
        if Gui.Control.activeDialog():
            return True
        self._prepare_edit_workbench()
        Gui.getDocument(view_object.Object.Document.Name).setEdit(view_object.Object, 0)
        # setEdit() is a command-style API and may return None even after the
        # view provider accepted edit mode. Returning that value let the tree
        # fall through to its default label-renaming action.
        return True

    def _prepare_edit_workbench(self):
        """Switch workbenches before setEdit() enters its re-entrant callback."""
        import FreeCADGui as Gui

        current = Gui.activeWorkbench().name()
        if not getattr(self, "_workbench_before_edit", ""):
            self._workbench_before_edit = current
        if current != FORMS_WORKBENCH:
            Gui.activateWorkbench(FORMS_WORKBENCH)

    def _show_only_edited_body_feature(self, view_object):
        """Expose this Body feature while temporarily hiding later features."""
        obj = view_object.Object
        body = obj.getParentGeoFeatureGroup()
        self._visibility_before_edit = []
        if body is None or not body.isDerivedFrom("PartDesign::Body"):
            view_object.Visibility = True
            return
        for feature in body.Group:
            if not hasattr(feature, "ViewObject"):
                continue
            self._visibility_before_edit.append((feature, bool(feature.ViewObject.Visibility)))
            feature.ViewObject.Visibility = feature == obj

    def _restore_body_feature_visibility(self):
        for feature, visible in getattr(self, "_visibility_before_edit", ()):
            if feature.Document is not None:
                feature.ViewObject.Visibility = visible
        self._visibility_before_edit = []

    def setEdit(self, view_object, mode):
        if mode != 0:
            return False
        import FreeCADGui as Gui
        from .edit import active_form_session, finish_active_form_session

        select_whole_form = "FormPlacement" in view_object.Object.PropertiesList and any(
            selection.Object == view_object.Object and not selection.SubElementNames
            for selection in Gui.Selection.getSelectionEx()
        )
        # setEdit() is also called directly after feature creation, without
        # passing through doubleClicked(). Always enter the Forms workbench
        # before constructing the editor and its toolbar-driven actions.
        self._prepare_edit_workbench()
        session = active_form_session()
        if session is not None:
            if session.obj == view_object.Object:
                self._edit_session = session
                return True
            finish_active_form_session()

        if not getattr(self, "_workbench_before_edit", ""):
            self._workbench_before_edit = Gui.activeWorkbench().name()
        gui_document = Gui.getDocument(view_object.Object.Document.Name)
        active_view = gui_document.ActiveView
        self._active_form_before_edit = active_view.getActiveObject(ACTIVE_FORM_KEY)
        active_view.setActiveObject(ACTIVE_FORM_KEY, view_object.Object)
        self._show_only_edited_body_feature(view_object)
        Gui.Selection.clearSelection()
        from .edit import FormEditSession

        try:
            creation_transaction = bool(getattr(self, "_creation_transaction", False))
            self._creation_transaction = False
            self._edit_session = FormEditSession(
                view_object.Object,
                document_edit=True,
                creation_transaction=creation_transaction,
            )
            self._edit_session.start()
            if select_whole_form:
                self._edit_session.select_whole_form()
            return True
        except MODELING_ERRORS as error:
            self._restore_body_feature_visibility()
            active_view.setActiveObject(ACTIVE_FORM_KEY, self._active_form_before_edit)
            previous = self._workbench_before_edit
            if previous != FORMS_WORKBENCH:
                Gui.activateWorkbench(previous)
            self._edit_session = None
            self._active_form_before_edit = None
            self._workbench_before_edit = ""
            return report_modeling_error(
                App.Qt.translate("Forms_Edit", "Edit Form"), error
            )
        except Exception:
            self._restore_body_feature_visibility()
            active_view.setActiveObject(ACTIVE_FORM_KEY, self._active_form_before_edit)
            previous = self._workbench_before_edit
            if previous != FORMS_WORKBENCH:
                Gui.activateWorkbench(previous)
            self._edit_session = None
            self._active_form_before_edit = None
            self._workbench_before_edit = ""
            raise

    def unsetEdit(self, view_object, mode):
        if mode == 0 and getattr(self, "_edit_session", None) is not None:
            self._edit_session.cleanup()
            self._edit_session = None
        if mode == 0 and App.GuiUp:
            import FreeCADGui as Gui

            if Gui.Control.activeDialog():
                Gui.Control.closeDialog()
            gui_document = Gui.getDocument(view_object.Object.Document.Name)
            active_view = gui_document.ActiveView
            if active_view.getActiveObject(ACTIVE_FORM_KEY) == view_object.Object:
                active_view.setActiveObject(
                    ACTIVE_FORM_KEY,
                    getattr(self, "_active_form_before_edit", None),
                )
            self._restore_body_feature_visibility()
            previous = getattr(self, "_workbench_before_edit", "")
            if previous and previous != FORMS_WORKBENCH:
                Gui.activateWorkbench(previous)
            self._active_form_before_edit = None
            self._workbench_before_edit = ""
        return True

    def dumps(self):
        return None

    def loads(self, _state):
        return None

