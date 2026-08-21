# SPDX-License-Identifier: LGPL-2.1-or-later

import math


def owning_part(marker):
    if marker is None or marker.Document is None:
        return None

    for obj in marker.Document.Objects:
        try:
            if obj.isDerivedFrom("MbDFEM::MbDPart") and obj.getMassMarker() is marker:
                return obj
        except Exception:
            pass

    return None


def is_mass_marker(obj):
    try:
        return obj is not None and obj.isDerivedFrom("MbDFEM::MbDMassMarker")
    except Exception:
        return False


class MassMarkerTaskPanel:
    def __init__(self, marker):
        import FreeCAD as App
        import FreeCADGui as Gui
        import MatGui
        import Materials
        from PySide import QtCore, QtWidgets

        if not is_mass_marker(marker):
            raise TypeError("MassMarkerTaskPanel requires an MbDMassMarker")

        self.marker = marker
        self.material_manager = Materials.MaterialManager()
        self.form = QtWidgets.QWidget()
        self.form.setWindowTitle("Mass Marker")

        self.mass_edit = QtWidgets.QLineEdit(_format_float(marker.mass))
        self.ix_edit = QtWidgets.QLineEdit(_format_float(marker.principalInertias.x))
        self.iy_edit = QtWidgets.QLineEdit(_format_float(marker.principalInertias.y))
        self.iz_edit = QtWidgets.QLineEdit(_format_float(marker.principalInertias.z))
        self.from_shape_check = QtWidgets.QCheckBox("From shape")
        self.from_shape_check.setChecked(bool(marker.massMarkerFromShape))

        placement = marker.Placement
        self.px_edit = QtWidgets.QLineEdit(_format_float(placement.Base.x))
        self.py_edit = QtWidgets.QLineEdit(_format_float(placement.Base.y))
        self.pz_edit = QtWidgets.QLineEdit(_format_float(placement.Base.z))

        axis = App.Vector(0, 0, 1)
        angle = 0.0
        try:
            axis = placement.Rotation.Axis
            angle = math.degrees(placement.Rotation.Angle)
        except Exception:
            pass

        self.axis_x_edit = QtWidgets.QLineEdit(_format_float(axis.x))
        self.axis_y_edit = QtWidgets.QLineEdit(_format_float(axis.y))
        self.axis_z_edit = QtWidgets.QLineEdit(_format_float(axis.z))
        self.angle_edit = QtWidgets.QLineEdit(_format_float(angle))

        self.material_widget = Gui.UiLoader().createWidget("MatGui::MaterialTreeWidget")
        self.material_tree = MatGui.MaterialTreeWidget(self.material_widget)
        self.material_tree.expanded = True
        self.material_tree.IncludeEmptyFolders = False
        self.material_tree.IncludeEmptyLibraries = False

        material_filter = Materials.MaterialFilter()
        try:
            material_filter.requirePhysical(True)
        except AttributeError:
            pass
        try:
            self.material_tree.setFilter(material_filter)
        except Exception:
            pass

        try:
            self.material_tree.UUID = marker.material.UUID
        except Exception:
            pass

        self.uuid = self.material_tree.UUID
        QtCore.QObject.connect(
            self.material_widget,
            QtCore.SIGNAL("onMaterial(QString)"),
            self._on_material,
        )

        layout = QtWidgets.QVBoxLayout(self.form)
        layout.addLayout(_form_layout([
            ("mass", self.mass_edit),
            ("principal inertia x", self.ix_edit),
            ("principal inertia y", self.iy_edit),
            ("principal inertia z", self.iz_edit),
            ("mass marker from shape", self.from_shape_check),
        ]))
        layout.addLayout(_form_layout([
            ("position x", self.px_edit),
            ("position y", self.py_edit),
            ("position z", self.pz_edit),
            ("rotation axis x", self.axis_x_edit),
            ("rotation axis y", self.axis_y_edit),
            ("rotation axis z", self.axis_z_edit),
            ("rotation angle deg", self.angle_edit),
        ]))
        layout.addWidget(self.material_widget)

        self.from_shape_check.toggled.connect(self._update_enabled_state)
        self._update_enabled_state(self.from_shape_check.isChecked())

    def getStandardButtons(self):
        from PySide import QtGui

        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

    def accept(self):
        import FreeCAD as App
        import FreeCADGui as Gui

        document = self.marker.Document
        if document is None:
            return False

        document.openTransaction("Edit Mass Marker")
        try:
            self._apply_material()
            if self.from_shape_check.isChecked():
                part = owning_part(self.marker)
                if part is None:
                    App.Console.PrintError("Mass marker has no owning MbDPart.\n")
                    document.abortTransaction()
                    return False
                part.populateMassMarkerFromShape()
            else:
                self.marker.Placement = App.Placement(
                    App.Vector(
                        _read_float(self.px_edit, "position x"),
                        _read_float(self.py_edit, "position y"),
                        _read_float(self.pz_edit, "position z"),
                    ),
                    App.Rotation(
                        App.Vector(
                            _read_float(self.axis_x_edit, "rotation axis x"),
                            _read_float(self.axis_y_edit, "rotation axis y"),
                            _read_float(self.axis_z_edit, "rotation axis z"),
                        ),
                        _read_float(self.angle_edit, "rotation angle deg"),
                    ),
                )
                self.marker.mass = _read_float(self.mass_edit, "mass")
                self.marker.principalInertias = App.Vector(
                    _read_float(self.ix_edit, "principal inertia x"),
                    _read_float(self.iy_edit, "principal inertia y"),
                    _read_float(self.iz_edit, "principal inertia z"),
                )
                self.marker.massMarkerFromShape = False

            document.commitTransaction()
        except Exception:
            document.abortTransaction()
            raise

        document.recompute()
        Gui.Control.closeDialog()
        return True

    def reject(self):
        import FreeCADGui as Gui

        Gui.Control.closeDialog()
        return True

    def _apply_material(self):
        self.uuid = self.material_tree.UUID
        if not self.uuid:
            return

        material = self.material_manager.getMaterial(self.uuid)
        self.marker.material = material

    def _on_material(self, uuid):
        self.uuid = uuid

    def _update_enabled_state(self, from_shape):
        for widget in (
            self.mass_edit,
            self.ix_edit,
            self.iy_edit,
            self.iz_edit,
            self.px_edit,
            self.py_edit,
            self.pz_edit,
            self.axis_x_edit,
            self.axis_y_edit,
            self.axis_z_edit,
            self.angle_edit,
        ):
            widget.setEnabled(not from_shape)


def show_mass_marker_task_panel(marker):
    import FreeCADGui as Gui

    Gui.Control.showDialog(MassMarkerTaskPanel(marker))


def _form_layout(rows):
    from PySide import QtWidgets

    layout = QtWidgets.QFormLayout()
    for label, widget in rows:
        layout.addRow(label, widget)
    return layout


def _format_float(value):
    return f"{float(value):.16g}"


def _read_float(widget, label):
    text = widget.text().strip()
    try:
        return float(text)
    except ValueError as exc:
        raise ValueError(f"{label} must be a number") from exc
