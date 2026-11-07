# SPDX-License-Identifier: LGPL-2.1-or-later
"""Part Design commands for additive and subtractive Forms primitives."""

import FreeCAD as App
import FreeCADGui as Gui
import Part
from PySide import QtCore, QtWidgets

PRIMITIVES = ("Box", "Cylinder", "Quadball", "Sphere", "Pipe", "Face", "Torus", "Tube")
OPERATIONS = ("Additive", "Subtractive")
_PENDING_BODY_DROPS = set()
_PRIMITIVE_LABELS = {
    "Box": QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Box"),
    "Cylinder": QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Cylinder"),
    "Sphere": QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Sphere"),
    "Quadball": QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Quadball"),
    "Pipe": QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Pipe"),
    "Face": QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Face"),
    "Torus": QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Torus"),
    "Tube": QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Tube"),
}
_OPERATION_RESOURCES = {
    "Additive": (
        QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Additive Form %1"),
        QtCore.QT_TRANSLATE_NOOP(
            "PartDesign_Form", "Creates an editable Form %1 and adds it to the body"
        ),
        QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Additive Form"),
        QtCore.QT_TRANSLATE_NOOP(
            "PartDesign_Form", "Creates an additive form primitive in the active body"
        ),
    ),
    "Subtractive": (
        QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Subtractive Form %1"),
        QtCore.QT_TRANSLATE_NOOP(
            "PartDesign_Form", "Creates an editable Form %1 and subtracts it from the body"
        ),
        QtCore.QT_TRANSLATE_NOOP("PartDesign_Form", "Subtractive Form"),
        QtCore.QT_TRANSLATE_NOOP(
            "PartDesign_Form", "Creates a subtractive form primitive in the active body"
        ),
    ),
}


def move_form_to_body(source, body):
    """Entry point used by the Body tree drop handler."""
    import Forms

    return Forms.move_form_to_body(source, body)


def schedule_move_form_to_body(source, body):
    """Defer conversion until the tree drop callback releases *source*."""
    if source is None or body is None or source.Document is not body.Document:
        return False
    key = (source.Document.Name, source.Name, body.Name)
    if key in _PENDING_BODY_DROPS:
        return True
    _PENDING_BODY_DROPS.add(key)

    def convert():
        try:
            document = App.getDocument(key[0])
            source_object = document.getObject(key[1])
            body_object = document.getObject(key[2])
            if source_object is None or body_object is None:
                return
            opened = document.getBookedTransactionID() == 0
            if opened:
                document.openTransaction(
                    App.Qt.translate("PartDesign_AdditiveForm", "Move Form into Body")
                )
            try:
                move_form_to_body(source_object, body_object)
            except Exception:
                if opened:
                    document.abortTransaction()
                raise
            if opened:
                document.commitTransaction()
        except Exception as error:
            App.Console.PrintError(f"Move Form into Body: {error}\n")
        finally:
            _PENDING_BODY_DROPS.discard(key)

    QtCore.QTimer.singleShot(0, convert)
    return True


def _primitive_icon(primitive, operation="Additive"):
    """Return the Part Design-colored SVG for one Forms primitive."""
    return f":/icons/PartDesign_{operation}Form{primitive}.svg"


def _selection(body=None):
    """Return ``(body, optional base feature, optional selected face)``."""
    selected_face = None
    selected_body = None
    selections = Gui.Selection.getSelectionEx()
    if len(selections) == 1:
        selected = selections[0]
        obj = selected.Object
        if obj.isDerivedFrom("PartDesign::Body"):
            selected_body = obj
        else:
            selected_body = obj.getParentGeoFeatureGroup()
        if body is None:
            body = selected_body
        if (
            selected_body is body
            and len(selected.SubObjects) == 1
            and getattr(selected.SubObjects[0], "ShapeType", "") == "Face"
        ):
            selected_face = selected.SubObjects[0]
    if body is None and Gui.ActiveDocument is not None:
        body = Gui.ActiveDocument.ActiveView.getActiveObject("pdbody")
    if body is None or not body.isDerivedFrom("PartDesign::Body"):
        return None
    source = body.Tip
    return body, source, selected_face


def _placement(source, face, scale, operation="Additive"):
    if face is not None:
        u_min, u_max, v_min, v_max = face.ParameterRange
        normal = face.normalAt((u_min + u_max) * 0.5, (v_min + v_max) * 0.5)
        offset = scale if operation == "Additive" else -scale * 0.5
        return App.Placement(
            face.CenterOfMass + normal * offset,
            App.Rotation(App.Vector(0, 0, 1), normal),
        )
    if source is None:
        return App.Placement()
    box = source.Shape.BoundBox
    center = App.Vector(
        (box.XMin + box.XMax) * 0.5,
        (box.YMin + box.YMax) * 0.5,
        (box.ZMin + box.ZMax) * 0.5,
    )
    offset = App.Vector(scale * 1.5, 0, 0) if operation == "Additive" else App.Vector()
    return App.Placement(center + offset, App.Rotation())


def _selected_pipe_path(body):
    selected = Gui.Selection.getSelection()
    if len(selected) != 1:
        return None
    path = selected[0]
    shape = getattr(path, "Shape", None)
    if shape is None or shape.isNull() or not shape.Edges or shape.Faces:
        return None
    parent = path.getParentGeoFeatureGroup()
    return path if parent is None or parent is body else None


def _preceding_solid(body, path):
    for feature in reversed(list(body.Group)):
        if feature is path:
            continue
        shape = getattr(feature, "Shape", None)
        if shape is not None and not shape.isNull() and shape.Solids:
            return feature
    return None


def _initialize_size(obj, primitive, size):
    values = {
        "Box": {"Length": size, "Width": size, "Height": size},
        "Cylinder": {"Radius": size * 0.5, "Height": size},
        "Sphere": {"Radius": size * 0.5},
        "Quadball": {"Radius": size * 0.5},
        "Pipe": {"Diameter": max(size * 0.25, 1.0)},
        "Face": {"Length": size, "Width": size},
        "Torus": {"MajorRadius": size * 0.5, "MinorRadius": size * 0.2},
        "Tube": {
            "OuterRadius": size * 0.5,
            "InnerRadius": size * 0.3,
            "Height": size,
        },
    }
    for name, value in values[primitive].items():
        setattr(obj, name, value)


class CommandFormPrimitive:
    def __init__(self, operation, primitive):
        self.Operation = operation
        self.Primitive = primitive

    def GetResources(self):
        menu_template, tooltip_template, _group_text, _group_tooltip = _OPERATION_RESOURCES[
            self.Operation
        ]
        primitive = App.Qt.translate("PartDesign_Form", _PRIMITIVE_LABELS[self.Primitive])
        menu_text = App.Qt.translate("PartDesign_Form", menu_template)
        tooltip = App.Qt.translate("PartDesign_Form", tooltip_template)
        return {
            "Pixmap": _primitive_icon(self.Primitive, self.Operation),
            "MenuText": menu_text.replace("%1", primitive),
            "ToolTip": tooltip.replace("%1", primitive),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return (
            App.ActiveDocument is not None
            and App.ActiveDocument.getBookedTransactionID() == 0
            and not Gui.Control.activeDialog()
        )

    def Activated(self):
        import PartDesignGui

        body = PartDesignGui.getBody()
        selected = _selection(body) if body is not None else None
        if selected is None:
            return
        body, source, face = selected
        path = _selected_pipe_path(body) if self.Primitive == "Pipe" else None
        if self.Primitive == "Pipe":
            if path is None:
                QtWidgets.QMessageBox.warning(
                    Gui.getMainWindow(),
                    App.Qt.translate("PartDesign_Form", "No path selected"),
                    App.Qt.translate(
                        "PartDesign_Form",
                        "Select one sketch, SubShapeBinder, Draft wire, or other "
                        "wire-only object.",
                    ),
                )
                return
            source = _preceding_solid(body, path)
            face = None
        if self.Operation == "Subtractive" and source is None:
            QtWidgets.QMessageBox.warning(
                Gui.getMainWindow(),
                App.Qt.translate("PartDesign_Form", "No previous feature found"),
                App.Qt.translate(
                    "PartDesign_Form",
                    "A subtractive Form requires an existing feature in the body.",
                ),
            )
            return
        size = max(source.Shape.BoundBox.DiagonalLength, 1.0) * 0.25 if source is not None else 10.0
        document = App.ActiveDocument
        transaction_label = self.GetResources()["MenuText"]
        document.openTransaction(transaction_label)
        try:
            import Forms

            factory = (
                Forms.create_additive_form
                if self.Operation == "Additive"
                else Forms.create_subtractive_form
            )
            feature = factory(
                body,
                source,
                self.Primitive,
                placement=(
                    App.Placement()
                    if path is not None
                    else _placement(source, face, size, self.Operation)
                ),
                path_object=path,
            )
            _initialize_size(feature, self.Primitive, size)
            document.recompute()
            if path is not None:
                path.Visibility = False
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(feature)
            feature.ViewObject.Proxy._creation_transaction = True
            Gui.getDocument(document.Name).setEdit(feature, 0)
            from Forms.edit import active_form_session

            if active_form_session(feature) is None:
                raise RuntimeError("Could not open the Forms creation task")
        except Exception:
            if document.getBookedTransactionID() != 0:
                document.abortTransaction()
            raise


class CommandFormGroup:
    def __init__(self, operation):
        self.Operation = operation

    def GetCommands(self):
        return tuple(f"PartDesign_{self.Operation}Form{primitive}" for primitive in PRIMITIVES)

    def GetDefaultCommand(self):
        return 0

    def GetResources(self):
        _menu, _tooltip, group_text, group_tooltip = _OPERATION_RESOURCES[self.Operation]
        return {
            "Pixmap": _primitive_icon("Box", self.Operation),
            "MenuText": App.Qt.translate("PartDesign_Form", group_text),
            "ToolTip": App.Qt.translate("PartDesign_Form", group_tooltip),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return (
            App.ActiveDocument is not None
            and App.ActiveDocument.getBookedTransactionID() == 0
            and not Gui.Control.activeDialog()
        )


class CommandAdditiveFormPrimitive(CommandFormPrimitive):
    def __init__(self, primitive):
        super().__init__("Additive", primitive)


class CommandSubtractiveFormPrimitive(CommandFormPrimitive):
    def __init__(self, primitive):
        super().__init__("Subtractive", primitive)


class CommandAdditiveFormGroup(CommandFormGroup):
    def __init__(self):
        super().__init__("Additive")


class CommandSubtractiveFormGroup(CommandFormGroup):
    def __init__(self):
        super().__init__("Subtractive")


for operation, command_type in (
    ("Additive", CommandAdditiveFormPrimitive),
    ("Subtractive", CommandSubtractiveFormPrimitive),
):
    for primitive in PRIMITIVES:
        Gui.addCommand(f"PartDesign_{operation}Form{primitive}", command_type(primitive))
Gui.addCommand("PartDesign_AdditiveForm", CommandAdditiveFormGroup())
Gui.addCommand("PartDesign_SubtractiveForm", CommandSubtractiveFormGroup())
# Compatibility for existing user toolbars from the initial Forms integration.
Gui.addCommand("PartDesign_FormSurface", CommandAdditiveFormGroup())
