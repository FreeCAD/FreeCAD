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

"""Part Design Forms primitives and associative boundary matching."""

from .feature import reset_cage

import re

import FreeCAD as App
import Part

from .box import FormBoxProxy
from .feature import FormFeatureProxy
from .viewprovider import ViewProviderForm as ViewProviderFormBox
from .cage import update_object_shape
from .placement import global_placement
from .primitives import (
    FormCylinderProxy,
    FormFaceProxy,
    FormSphereProxy,
    FormQuadballProxy,
    FormTorusProxy,
    FormTubeProxy,
    ViewProviderFormCylinder,
    ViewProviderFormFace,
    ViewProviderFormSphere,
    ViewProviderFormQuadball,
    ViewProviderFormTorus,
    ViewProviderFormTube,
    update_profile_face_shape,
)
from .pipe import FormPipeProxy, ViewProviderFormPipe, fused_pipe_shape, update_pipe_shape

PRIMITIVES = {
    "Box": (FormBoxProxy, ViewProviderFormBox, "Form Box"),
    "Cylinder": (FormCylinderProxy, ViewProviderFormCylinder, "Form Cylinder"),
    "Sphere": (FormSphereProxy, ViewProviderFormSphere, "Form Sphere"),
    "Quadball": (FormQuadballProxy, ViewProviderFormQuadball, "Form Quadball"),
    "Pipe": (FormPipeProxy, ViewProviderFormPipe, "Form Pipe"),
    "Face": (FormFaceProxy, ViewProviderFormFace, "Form Face"),
    "Torus": (FormTorusProxy, ViewProviderFormTorus, "Form Torus"),
    "Tube": (FormTubeProxy, ViewProviderFormTube, "Form Tube"),
}

OPERATIONS = ("Additive", "Subtractive")

# PartDesign::FeatureAddSub owns the Operation property of a Body feature and
# derives the additive or subtractive type of the feature from its value.
# These are the values of its additive and subtractive Python features; the
# first one is the operation a Form computes.
_NATIVE_OPERATIONS = {
    "Additive": ("Union",),
    "Subtractive": ("Subtraction", "Common"),
}


MATCH_CORNER_SHARPNESS = 10.0


def _primitive_proxy(obj):
    form_type = str(obj.FormType).removeprefix("Forms::")
    if form_type not in PRIMITIVES:
        raise ValueError(f"Unsupported Part Design Form primitive: {form_type}")
    return PRIMITIVES[form_type][0]


# Compatibility exports for existing scripts.
from .matching import (
    _linked_support,
    _linked_face,
    _closest_point,
    _face_match_parameters,
    _face_point,
    _wire_point,
    _wire_fraction,
    _wire_match_parameters,
    _edge_direction_from_vertex,
    _wire_corner_points,
    _apply_match_corner_sharpness,
    _apply_match_corner_creases,
    _edge_contains_points,
    _neighboring_support_face,
    _surface_tangent_plane,
    _project_to_planes,
    _boundary_tangent_planes,
    apply_match_constraints,
    _matched_boundary,
    _validate_match_mode,
    preview_match_shape,
    match_boundary,
    _boundary_edges,
    _cap_matched_form,
)


def _partdesign_operation(obj):
    if "Operation" in obj.PropertiesList:
        operation = str(obj.Operation)
        if operation in OPERATIONS:
            # A Form that is no Part Design add/sub feature, or a document
            # saved when Forms replaced the native values by its own ones.
            return operation
        for name, values in _NATIVE_OPERATIONS.items():
            if operation in values:
                return name
    if obj.isDerivedFrom("PartDesign::FeatureSubtractivePython"):
        return "Subtractive"
    return "Additive"


class PartDesignFormProxy(FormFeatureProxy):
    """Forms cage that adds material to or removes material from a Body."""

    def __init__(self, obj, primitive, base_feature=None, operation="Additive", path_object=None):
        if primitive not in PRIMITIVES:
            raise ValueError(f"Unsupported Part Design Form primitive: {primitive}")
        if operation not in OPERATIONS:
            raise ValueError(f"Unsupported Part Design Form operation: {operation}")
        if primitive == "Pipe":
            FormPipeProxy(obj, path_object)
        else:
            PRIMITIVES[primitive][0](obj)
        obj.Proxy = self
        self._ensure_partdesign_properties(obj, operation)
        obj.BaseFeature = base_feature
        obj.PrimitiveKind = primitive
        self.onChanged(obj, "CageMode")

    @staticmethod
    def _ensure_native_operation(obj, operation):
        """Keep the Operation of a Part Design add/sub feature, which Part
        Design reads for patterns, dress-ups and previews, in line with the
        Form's operation. Return whether it had to be changed."""
        values = _NATIVE_OPERATIONS[operation]
        if str(obj.Operation) in values:
            return False
        enumeration = obj.getEnumerationsOfProperty("Operation") or []
        if not all(value in enumeration for value in values):
            # Forms used to replace the native values by its own ones, which
            # made a subtractive Form an additive feature for Part Design.
            obj.Operation = list(values)
        obj.Operation = values[0]
        return True

    @staticmethod
    def _ensure_partdesign_properties(obj, operation="Additive"):
        """Return whether the native Operation of the feature was corrected."""
        group = "Part Design Form"
        migrated = False
        if obj.isDerivedFrom("PartDesign::FeatureAddSub"):
            migrated = PartDesignFormProxy._ensure_native_operation(obj, operation)
        else:
            if "Operation" not in obj.PropertiesList:
                obj.addProperty(
                    "App::PropertyEnumeration", "Operation", group, "Boolean operation"
                )
            obj.Operation = list(OPERATIONS)
            obj.Operation = operation
        if "PrimitiveKind" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString", "PrimitiveKind", group, "Primitive kind"
            )
        if "BaseFeature" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLink",
                "BaseFeature",
                group,
                "Preceding Part Design feature",
            )
        if "FormPlacement" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyPlacement",
                "FormPlacement",
                group,
                "Initial placement of the Form cage",
            )
        if "FormShape" not in obj.PropertiesList:
            obj.addProperty(
                "Part::PropertyPartShape",
                "FormShape",
                group,
                "Uncombined editable Form geometry",
            )
        if "EditingForm" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyBool",
                "EditingForm",
                group,
                "Show the base and Form as separate shapes while editing",
            )
        FormFeatureProxy._ensure_match_properties(obj)
        if "CombinationStatus" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "CombinationStatus",
                group,
                "Result of combining the Form with the preceding feature",
            )
        for name in (
            "Operation",
            "PrimitiveKind",
            "FormShape",
            "EditingForm",
            "MatchBoundary",
            "CombinationStatus",
        ):
            obj.setEditorMode(name, 2)
        return migrated

    def _topology(self, obj):
        proxy = _primitive_proxy(obj)
        vertices, faces = proxy._topology(self, obj)
        placement = obj.FormPlacement
        transformed = []
        for point in vertices:
            value = placement.multVec(App.Vector(*point))
            transformed.append((value.x, value.y, value.z))
        return transformed, faces

    def execute(self, obj):
        if obj.CageMode == "Parametric":
            vertices, faces = self._topology(obj)
            reset_cage(obj, vertices, faces)
        if str(obj.FormType) == "Forms::Pipe":
            update_pipe_shape(obj)
        elif not (
            str(obj.FormType) == "Forms::Face" and update_profile_face_shape(self, obj)
        ):
            update_object_shape(obj)
        form_shape = obj.Shape
        obj.FormShape = form_shape
        if "AddSubShape" in obj.PropertiesList:
            obj.AddSubShape = form_shape
        if obj.BaseFeature is not None:
            # Boolean history created below belongs to this Part Design
            # feature. Retag a private copy so generated/modified element
            # names record this object's ID without changing BaseFeature.
            base = obj.BaseFeature.Shape.copy()
            base.Tag = obj.ID
        else:
            base = Part.Shape()
        operation = _partdesign_operation(obj)
        if base.isNull() and operation == "Subtractive":
            # As for native Part Design subtractive features: there is nothing
            # to cut, and the tool must not become the Body's material.
            # FormShape and AddSubShape keep the tool for the preview.
            obj.Shape = Part.Shape()
            obj.CombinationStatus = App.Qt.translate(
                "Forms_PartDesign", "Cannot subtract a Form without a base feature"
            )
            raise RuntimeError(obj.CombinationStatus)
        if base.isNull():
            obj.CombinationStatus = App.Qt.translate(
                "Forms_PartDesign", "Form only; no preceding feature"
            )
            return
        if obj.EditingForm:
            # Keep the editable tool as the selectable shape. Its faces are
            # hidden by the view provider while Part Design renders AddSubShape
            # through its native preview framework.
            obj.Shape = form_shape
            obj.CombinationStatus = App.Qt.translate(
                "Forms_PartDesign", "Editing form preview"
            )
            return
        result = None
        try:
            tool = (
                fused_pipe_shape(form_shape)
                if str(obj.FormType) == "Forms::Pipe"
                else _cap_matched_form(obj, form_shape)
            )
            if "AddSubShape" in obj.PropertiesList:
                # Downstream Part Design patterns consume this native tool shape.
                obj.AddSubShape = tool
            if tool.ShapeType != "Solid":
                shown = Part.makeCompound([base, form_shape])
                status = App.Qt.translate(
                    "Forms_PartDesign", "Open form; match or thicken it before combining"
                )
            else:
                result = base.fuse(tool) if operation == "Additive" else base.cut(tool)
                if result.isNull() or not result.isValid() or len(result.Solids) != 1:
                    result = None
                    shown = Part.makeCompound([base, tool])
                    status = App.Qt.translate(
                        "Forms_PartDesign", "Form does not produce a valid single-solid result"
                    )
        except (Part.OCCError, RuntimeError, ValueError) as error:
            result = None
            shown = Part.makeCompound([base, form_shape])
            message = App.Qt.translate("Forms_PartDesign", "Not combined: %1")
            status = message.replace("%1", str(error))
        if result is None:
            obj.CombinationStatus = status
            if operation == "Subtractive":
                # As for native Part Design subtractive features, a cut that
                # fails is an error: the base and the tool shown together
                # would make the tool material for the following features.
                # Nothing is removed; AddSubShape keeps the tool for the
                # preview.
                obj.Shape = base
                raise RuntimeError(status)
            # The base and the tool shown together are the material the
            # additive Form is meant to add, e.g. while its face is open.
            obj.Shape = shown
            return
        obj.Shape = result
        if operation == "Additive":
            obj.CombinationStatus = App.Qt.translate(
                "Forms_PartDesign", "Valid fused additive form"
            )
        else:
            obj.CombinationStatus = App.Qt.translate(
                "Forms_PartDesign", "Valid cut subtractive form"
            )

    def show_edit_shape(self, obj, enabled):
        """Expose the selectable control patches of a profile Form Face while
        it is edited, as the standalone Form Face does."""
        self._show_edit_shape = bool(enabled)
        obj.touch()
        obj.Document.recompute()

    def onChanged(self, obj, prop):
        proxy = _primitive_proxy(obj) if "FormType" in obj.PropertiesList else None
        if prop == "CageMode" and proxy is not None:
            read_only = 0 if obj.CageMode == "Parametric" else 1
            for name in proxy.ParameterNames:
                obj.setEditorMode(name, read_only)

    def onDocumentRestored(self, obj):
        self._show_edit_shape = False
        self._ensure_conversion_properties(obj)
        self._ensure_symmetry_properties(obj)
        self._ensure_sharpness_properties(obj)
        self._ensure_local_edit_properties(obj)
        self._ensure_partdesign_properties(obj, _partdesign_operation(obj))
        # Edit mode itself is not restored by FreeCAD. Do not leave a recovered
        # document displaying the temporary, uncombined tool shape.
        obj.EditingForm = False
        obj.Proxy = self
        self.onChanged(obj, "CageMode")
        obj.touch()
        obj.recompute()


class AdditiveFormProxy(PartDesignFormProxy):
    """Backward-compatible additive Part Design Form proxy."""

    def __init__(self, obj, primitive, base_feature=None, path_object=None):
        super().__init__(obj, primitive, base_feature, "Additive", path_object)


class SubtractiveFormProxy(PartDesignFormProxy):
    """Subtractive Part Design Form proxy."""

    def __init__(self, obj, primitive, base_feature=None, path_object=None):
        super().__init__(obj, primitive, base_feature, "Subtractive", path_object)


class ViewProviderPartDesignForm(ViewProviderFormBox):
    def __init__(self, view_object):
        super().__init__(view_object)

    def getIcon(self):
        view_object = getattr(self, "ViewObject", None)
        obj = getattr(view_object, "Object", None)
        primitive = str(getattr(obj, "PrimitiveKind", ""))
        if primitive not in PRIMITIVES:
            primitive = "Box"
        operation = _partdesign_operation(obj) if obj is not None else "Additive"
        return f":/icons/PartDesign_{operation}Form{primitive}.svg"

    def claimChildren(self):
        obj = getattr(getattr(self, "ViewObject", None), "Object", None)
        path = getattr(obj, "PathObject", None) if obj is not None else None
        if path is None:
            return []
        return [path]

    def _show_only_edited_body_feature(self, view_object):
        """Show the editable tool with the native Part Design preview color."""
        obj = view_object.Object
        body = obj.getParentGeoFeatureGroup()
        if body is None or not body.isDerivedFrom("PartDesign::Body"):
            super()._show_only_edited_body_feature(view_object)
            return

        self._visibility_before_edit = [
            (feature, bool(feature.ViewObject.Visibility))
            for feature in body.Group
            if hasattr(feature, "ViewObject")
        ]
        self._transparency_before_edit = int(view_object.Transparency)
        self._shape_color_before_edit = tuple(view_object.ShapeColor)[:3]
        preview_color = tuple(getattr(view_object, "PreviewColor", view_object.ShapeColor))
        view_object.ShapeColor = preview_color[:3]
        # The standard Part Design preview is intentionally unpickable and very
        # transparent. Forms instead displays the real tool shape so its faces,
        # edges, and vertices remain available to the editor.
        view_object.Transparency = 0

        if obj.BaseFeature is not None:
            view_object.showPreviousFeature(True)
            # Part Design permits an editing feature to remain visible beside
            # its predecessor. Use the normal live view provider so topology
            # and placement changes are reflected immediately.
            view_object.Visibility = True
        else:
            view_object.Visibility = True

    def _restore_body_feature_visibility(self):
        view_object = getattr(self, "ViewObject", None)
        if view_object is not None:
            if hasattr(self, "_transparency_before_edit"):
                view_object.Transparency = self._transparency_before_edit
                del self._transparency_before_edit
            if hasattr(self, "_shape_color_before_edit"):
                view_object.ShapeColor = self._shape_color_before_edit
                del self._shape_color_before_edit
        super()._restore_body_feature_visibility()

    def setEdit(self, view_object, mode):
        if mode != 0:
            return False
        obj = view_object.Object
        obj.EditingForm = True
        obj.Document.recompute()
        try:
            started = super().setEdit(view_object, mode)
        except Exception:
            obj.EditingForm = False
            obj.Document.recompute()
            raise
        if not started:
            # A rejected edit is reported by returning False, and FreeCAD then
            # never calls unsetEdit(): restore the combined Body result here.
            obj.EditingForm = False
            obj.Document.recompute()
        return started

    def unsetEdit(self, view_object, mode):
        result = super().unsetEdit(view_object, mode)
        if mode == 0:
            obj = view_object.Object
            obj.EditingForm = False
            obj.Document.recompute()
        return result


class ViewProviderAdditiveForm(ViewProviderPartDesignForm):
    """Backward-compatible additive Form view provider."""


class ViewProviderSubtractiveForm(ViewProviderPartDesignForm):
    """Subtractive Form view provider."""


def create_partdesign_form(
    body,
    base_feature,
    primitive="Box",
    name=None,
    placement=None,
    operation="Additive",
    path_object=None,
):
    """Create an additive or subtractive Forms primitive inside *body*.

    As for any Part Design feature, the Form is added after the Body's Tip and
    based on the Body's last solid feature up to the Tip. *base_feature* states
    that base: it must be that solid feature, or the Tip, after which the Form
    is added. ``None`` is accepted only in a Body without such a feature."""
    if primitive not in PRIMITIVES:
        raise ValueError(f"Unsupported Part Design Form primitive: {primitive}")
    if operation not in OPERATIONS:
        raise ValueError(f"Unsupported Part Design Form operation: {operation}")
    name = name or f"{operation}Form{primitive}"
    feature_type = f"PartDesign::Feature{operation}Python"
    tip = body.Tip
    obj = body.newObject(feature_type, name)
    # The Body set the base and routed the next solid feature to the new one:
    # another base would drop features from the Body result or make a cycle.
    body_base = obj.BaseFeature
    if base_feature is not body_base and base_feature is not tip:
        document = obj.Document
        body.removeObject(obj)
        document.removeObject(obj.Name)
        if body.Tip is not tip:
            body.Tip = tip
        expected = body_base.Label if body_base is not None else "None"
        raise ValueError(
            f"The base feature of a Form in {body.Label} must be {expected}, "
            "the Body's previous solid feature"
        )
    base_feature = body_base
    label_template = App.Qt.translate("Forms_Create", "%1 Form %2")
    operation_label = (
        App.Qt.translate("Forms_Create", "Additive")
        if operation == "Additive"
        else App.Qt.translate("Forms_Create", "Subtractive")
    )
    primitive_labels = {
        "Box": App.Qt.translate("Forms_Create", "Box"),
        "Cylinder": App.Qt.translate("Forms_Create", "Cylinder"),
        "Sphere": App.Qt.translate("Forms_Create", "Sphere"),
        "Quadball": App.Qt.translate("Forms_Create", "Quadball"),
        "Pipe": App.Qt.translate("Forms_Create", "Pipe"),
        "Face": App.Qt.translate("Forms_Create", "Face"),
        "Torus": App.Qt.translate("Forms_Create", "Torus"),
        "Tube": App.Qt.translate("Forms_Create", "Tube"),
    }
    primitive_label = primitive_labels[primitive]
    obj.Label = label_template.replace("%1", operation_label).replace(
        "%2", primitive_label
    )
    proxy_type = AdditiveFormProxy if operation == "Additive" else SubtractiveFormProxy
    proxy_type(obj, primitive, base_feature, path_object)
    if placement is not None:
        obj.FormPlacement = placement
    if App.GuiUp:
        view_provider_type = (
            ViewProviderAdditiveForm
            if operation == "Additive"
            else ViewProviderSubtractiveForm
        )
        view_provider_type(obj.ViewObject)
    obj.recompute()
    return obj


def create_additive_form(
    body, base_feature, primitive="Box", name=None, placement=None, path_object=None
):
    """Create an additive Forms primitive inside *body*."""
    return create_partdesign_form(
        body, base_feature, primitive, name, placement, "Additive", path_object
    )


def create_subtractive_form(
    body, base_feature, primitive="Box", name=None, placement=None, path_object=None
):
    """Create a subtractive Forms primitive inside *body*."""
    return create_partdesign_form(
        body, base_feature, primitive, name, placement, "Subtractive", path_object
    )


def move_form_to_body(source, body):
    """Replace a standalone Forms primitive with an additive Body feature."""
    if source is None or body is None:
        raise ValueError("A Form and destination Body are required")
    if source.Document is not body.Document:
        raise ValueError("The Form and destination Body must be in the same document")
    parent = source.getParentGeoFeatureGroup()
    if parent is not None and parent.isDerivedFrom("PartDesign::Body"):
        raise ValueError("Only a standalone Form can be moved into a Body")

    primitive = str(getattr(source, "FormType", "")).removeprefix("Forms::")
    if primitive not in PRIMITIVES:
        raise ValueError(f"Unsupported additive Form primitive: {primitive}")

    document = source.Document
    base_feature = body.Tip
    source_name = source.Name
    source_label = source.Label
    source_mode = str(source.CageMode)
    relative_placement = global_placement(body).inverse() * global_placement(source)
    parameter_names = PRIMITIVES[primitive][0].ParameterNames

    copied = {}
    for name in (
        *parameter_names,
        "BRepTolerance",
        "MaxRefinement",
        "Symmetric",
        "SymmetryPlane",
        "VertexSharpness",
        "EdgeSharpness",
        "LocalEdgeInserts",
        "TMeshData",
        "DissolvedEdges",
        "ControlFaces",
        "MatchSupport",
        "MatchBoundary",
        "MatchContinuity",
        "MatchTangentMode",
        "MatchPositionTolerance",
        "MatchAngularTolerance",
        "MatchCurvatureTolerance",
        "MatchParameters",
        "MatchCornerVertices",
        "MatchCornerEdges",
        "SegmentDiameters",
        "SegmentSamples",
    ):
        if name in source.PropertiesList:
            copied[name] = getattr(source, name)
    # Keep the copied properties parametric: e.g. a Length bound to a
    # spreadsheet must still follow it in the Body feature.
    expressions = []
    for path, expression in source.ExpressionEngine:
        name = re.split(r"[.\[]", str(path).lstrip("."))[0]
        if name in copied:
            expressions.append((name, path, expression))
    source_points = [App.Vector(point) for point in source.ControlPoints]
    local_points = [App.Vector(point) for point in getattr(source, "LocalControlPoints", ())]
    view_values = {}
    if App.GuiUp:
        for name in ("ShapeColor", "LineColor", "PointColor", "Transparency"):
            if name in source.ViewObject.PropertiesList:
                view_values[name] = getattr(source.ViewObject, name)

    result = create_additive_form(
        body,
        base_feature,
        primitive,
        name=f"AdditiveForm{primitive}",
        placement=relative_placement,
        path_object=getattr(source, "PathObject", None),
    )
    for name, value in copied.items():
        if name in result.PropertiesList:
            setattr(result, name, value)
    # FreeCAD stores references to the object's own properties relative to
    # it, so these now refer to the new feature, not to the removed source.
    for name, path, expression in expressions:
        if name in result.PropertiesList:
            result.setExpression(path, expression)

    if source_mode == "Editable":
        # Editable cages already contain their final primitive geometry. Bake
        # the standalone object's global placement into Body-local controls.
        result.CageMode = "Editable"
        result.FormPlacement = App.Placement()
        result.ControlPoints = [relative_placement.multVec(point) for point in source_points]
        result.LocalControlPoints = [relative_placement.multVec(point) for point in local_points]
    else:
        result.CageMode = "Parametric"
        result.FormPlacement = relative_placement
    if "ProfileShape" in source.PropertiesList:
        # The exact profile of a Form Face, in Body coordinates like the cage.
        # Baked in the geometry: the feature placement replaces the location
        # of the shape it is given.
        profile = source.ProfileShape.copy()
        if not profile.isNull():
            profile.transformShape(relative_placement.toMatrix(), True)
        result.ProfileShape = profile
        result.ProfileControlPoints = [
            relative_placement.multVec(point) for point in source.ProfileControlPoints
        ]
        result.ProfileControlFaces = list(source.ProfileControlFaces)

    if App.GuiUp:
        for name, value in view_values.items():
            if name in result.ViewObject.PropertiesList:
                setattr(result.ViewObject, name, value)
        import FreeCADGui as Gui

        Gui.Selection.clearSelection()
    document.removeObject(source_name)
    result.Label = source_label
    body.Tip = result
    document.recompute()
    if App.GuiUp:
        Gui.Selection.addSelection(result)
    return result


# Keep the historical scripting imports available after separating implementation modules.
__all__ = [
    "AdditiveFormProxy",
    "MATCH_CORNER_SHARPNESS",
    "OPERATIONS",
    "PRIMITIVES",
    "PartDesignFormProxy",
    "SubtractiveFormProxy",
    "ViewProviderAdditiveForm",
    "ViewProviderPartDesignForm",
    "ViewProviderSubtractiveForm",
    "_apply_match_corner_creases",
    "_apply_match_corner_sharpness",
    "_boundary_edges",
    "_boundary_tangent_planes",
    "_cap_matched_form",
    "_closest_point",
    "_edge_contains_points",
    "_edge_direction_from_vertex",
    "_face_match_parameters",
    "_face_point",
    "_linked_face",
    "_linked_support",
    "_matched_boundary",
    "_neighboring_support_face",
    "_project_to_planes",
    "_surface_tangent_plane",
    "_validate_match_mode",
    "_wire_corner_points",
    "_wire_fraction",
    "_wire_match_parameters",
    "_wire_point",
    "apply_match_constraints",
    "create_additive_form",
    "create_partdesign_form",
    "create_subtractive_form",
    "match_boundary",
    "move_form_to_body",
    "preview_match_shape",
]
