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

"""Parametric Forms box object and its Python view provider."""

import FreeCAD as App

from .cage import update_object_shape
from .feedback import MODELING_ERRORS, report_modeling_error
from .topology import box_control_cage, cage_edges

FORMS_WORKBENCH = "FormsWorkbench"
ACTIVE_FORM_KEY = "form"


class FormFeatureProxy:
    """Shared persistence and recompute lifecycle for every Forms primitive."""

    Type = ""
    ParameterNames = ()

    @classmethod
    def _add_common_properties(cls, obj):
        obj.addProperty("App::PropertyString", "FormType", "Forms", "Forms object type")
        obj.addProperty(
            "App::PropertyInteger", "TopologyVersion", "Forms", "Control-cage data version"
        )
        obj.addProperty(
            "App::PropertyVectorList", "ControlPoints", "Control Cage", "Control-cage vertices"
        )
        obj.addProperty(
            "App::PropertyStringList", "ControlFaces", "Control Cage", "Quad vertex indices"
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "CageMode",
            "Control Cage",
            "Whether the cage follows primitive parameters or is edited directly",
        )
        cls._ensure_symmetry_properties(obj)
        cls._ensure_sharpness_properties(obj)
        cls._ensure_local_edit_properties(obj)
        cls._ensure_conversion_properties(obj)
        cls._ensure_match_properties(obj)

        obj.FormType = cls.Type
        obj.TopologyVersion = 1
        obj.CageMode = ["Parametric", "Editable"]
        obj.CageMode = "Parametric"
        obj.setEditorMode("FormType", 1)
        obj.setEditorMode("TopologyVersion", 1)
        obj.setEditorMode("ControlPoints", 1)
        obj.setEditorMode("ControlFaces", 1)

    def _finish_initialization(self, obj):
        obj.Proxy = self
        self.onChanged(obj, "CageMode")

    def _topology(self, _obj):
        raise NotImplementedError

    @staticmethod
    def _ensure_match_properties(obj):
        if "MatchSupport" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLinkSub",
                "MatchSupport",
                "Match",
                "Associative face or closed-wire support",
            )
        if "MatchBoundary" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyIntegerList",
                "MatchBoundary",
                "Match",
                "Ordered control vertices around the matched opening",
            )
        if "MatchContinuity" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "MatchContinuity",
                "Match",
                "Continuity across the matched transition",
            )
            obj.MatchContinuity = ["Connected", "Tangent"]
            obj.MatchContinuity = "Tangent"
        if "MatchTangentMode" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "MatchTangentMode",
                "Match",
                "Faces that define tangent continuity around the matched opening",
            )
            obj.MatchTangentMode = ["AdjacentFaces", "SelectedFace"]
            obj.MatchTangentMode = "AdjacentFaces"
        if "MatchParameters" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyFloatList",
                "MatchParameters",
                "Match",
                "Normalized support coordinates for the matched boundary",
            )
        if "MatchCornerVertices" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyIntegerList",
                "MatchCornerVertices",
                "Match",
                "Matched control vertices held at corners of the support wire",
            )
        if "MatchCornerEdges" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "MatchCornerEdges",
                "Match",
                "Control edges creased from corners of a tangent matched opening",
            )
        obj.setEditorMode("MatchBoundary", 2)
        obj.setEditorMode("MatchParameters", 2)
        obj.setEditorMode("MatchCornerVertices", 2)
        obj.setEditorMode("MatchCornerEdges", 2)

    @staticmethod
    def _ensure_local_edit_properties(obj):
        if "DissolvedEdges" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "DissolvedEdges",
                "Topology",
                "Internal control edges hidden by logical face merging",
            )
        obj.setEditorMode("DissolvedEdges", 1)
        if "TMeshData" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "TMeshData",
                "Topology",
                "Versioned hierarchical T-mesh topology",
            )
        obj.setEditorMode("TMeshData", 1)
        if "LocalEdgeInserts" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "LocalEdgeInserts",
                "Topology",
                "Exact local BRep edge insertions",
            )
        obj.setEditorMode("LocalEdgeInserts", 1)
        if "LocalControlPoints" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyVectorList",
                "LocalControlPoints",
                "Topology",
                "Persistent controls introduced by local topology edits",
            )
        obj.setEditorMode("LocalControlPoints", 1)

    @staticmethod
    def _ensure_sharpness_properties(obj):
        if "VertexSharpness" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyFloatList",
                "VertexSharpness",
                "Subdivision",
                "Semi-sharp corner value for each control vertex",
            )
        if "EdgeSharpness" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "EdgeSharpness",
                "Subdivision",
                "Semi-sharp control edges encoded as start end value",
            )
        obj.setEditorMode("VertexSharpness", 1)
        obj.setEditorMode("EdgeSharpness", 1)

    @staticmethod
    def _ensure_conversion_properties(obj):
        if "BRepTolerance" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLength",
                "BRepTolerance",
                "BRep Conversion",
                "Maximum accepted distance from sampled Catmull-Clark limit points",
            )
            obj.BRepTolerance = 0.05
        if "MaxRefinement" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyIntegerConstraint",
                "MaxRefinement",
                "BRep Conversion",
                "Maximum patch sampling refinement",
            )
            obj.MaxRefinement = (3, 2, 4, 1)
        if "MaximumDeviation" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLength",
                "MaximumDeviation",
                "BRep Conversion",
                "Measured distance from validation samples to the generated surfaces",
            )
        if "ConversionLevel" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyInteger",
                "ConversionLevel",
                "BRep Conversion",
                "Patch sampling level used for the current BRep",
            )
        if "ConversionStatus" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "ConversionStatus",
                "BRep Conversion",
                "Result of the most recent cage-to-BRep conversion",
            )
        for name in ("MaximumDeviation", "ConversionLevel", "ConversionStatus"):
            obj.setEditorMode(name, 1)

    @staticmethod
    def _ensure_symmetry_properties(obj):
        if "Symmetric" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyBool",
                "Symmetric",
                "Symmetry",
                "Keep the control cage symmetric",
            )
            obj.Symmetric = False
        if "SymmetryPlane" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "SymmetryPlane",
                "Symmetry",
                "Local plane used to mirror control points",
            )
            obj.SymmetryPlane = ["XY", "XZ", "YZ"]
            obj.SymmetryPlane = "YZ"

    def execute(self, obj):
        if obj.CageMode == "Parametric":
            vertices, faces = self._topology(obj)
            obj.ControlPoints = [App.Vector(*point) for point in vertices]
            obj.ControlFaces = [" ".join(str(index) for index in face) for face in faces]
            obj.VertexSharpness = [0.0] * len(vertices)
            obj.EdgeSharpness = []
            obj.LocalEdgeInserts = []
            obj.LocalControlPoints = []
            obj.TMeshData = ""
            obj.DissolvedEdges = []
        update_object_shape(obj)

    def onChanged(self, obj, prop):
        if prop == "CageMode":
            read_only = 0 if obj.CageMode == "Parametric" else 1
            for name in self.ParameterNames:
                obj.setEditorMode(name, read_only)

    def onDocumentRestored(self, obj):
        self._ensure_conversion_properties(obj)
        self._ensure_symmetry_properties(obj)
        self._ensure_sharpness_properties(obj)
        self._ensure_local_edit_properties(obj)
        self._ensure_match_properties(obj)
        obj.Proxy = self
        self.onChanged(obj, "CageMode")


class FormBoxProxy(FormFeatureProxy):
    """Application-side implementation of a form box."""

    Type = "Forms::Box"
    ParameterNames = (
        "Length",
        "Width",
        "Height",
        "XSegments",
        "YSegments",
        "ZSegments",
    )

    def __init__(self, obj):
        self._add_common_properties(obj)
        obj.addProperty("App::PropertyLength", "Length", "Box", "Length along the X axis")
        obj.addProperty("App::PropertyLength", "Width", "Box", "Width along the Y axis")
        obj.addProperty("App::PropertyLength", "Height", "Box", "Height along the Z axis")
        obj.addProperty(
            "App::PropertyIntegerConstraint", "XSegments", "Box", "Control faces along X"
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint", "YSegments", "Box", "Control faces along Y"
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint", "ZSegments", "Box", "Control faces along Z"
        )
        obj.Length = 20.0
        obj.Width = 20.0
        obj.Height = 20.0
        obj.XSegments = (2, 1, 100, 1)
        obj.YSegments = (2, 1, 100, 1)
        obj.ZSegments = (2, 1, 100, 1)
        self._finish_initialization(obj)

    def _topology(self, obj):
        return box_control_cage(
            obj.Length.Value,
            obj.Width.Value,
            obj.Height.Value,
            obj.XSegments,
            obj.YSegments,
            obj.ZSegments,
        )


class ViewProviderFormBox:
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
        if prop not in ("ControlPoints", "ControlFaces") or self._cage_coordinates is None:
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


def create_box(document=None, name="FormBox"):
    """Create and return a new form box."""
    document = document or App.ActiveDocument
    if document is None:
        raise RuntimeError("A document is required to create a form box")
    obj = document.addObject("Part::FeaturePython", name)
    obj.Label = App.Qt.translate("Forms_Create", "Form Box")
    FormBoxProxy(obj)
    if App.GuiUp:
        ViewProviderFormBox(obj.ViewObject)
    obj.recompute()
    return obj
