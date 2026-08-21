# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCADGui as Gui

import FreeCAD
import MatGui  # noqa: F401
import PartGui  # noqa: F401
import MbDFEMGui  # noqa: F401
import FreeCADMbDAnimationPanel
import FreeCADMbDSimulationPanel

FreeCAD.__unit_test__ += ["TestMbDFEMGui"]

try:
    Gui.Selection.removeObserver(_animation_parameters_selection_observer)
except Exception:
    pass
_animation_parameters_selection_observer = (
    FreeCADMbDAnimationPanel.AnimationParametersSelectionObserver()
)
Gui.Selection.addObserver(_animation_parameters_selection_observer)

try:
    Gui.Selection.removeObserver(_simulation_parameters_selection_observer)
except Exception:
    pass
_simulation_parameters_selection_observer = (
    FreeCADMbDSimulationPanel.SimulationParametersSelectionObserver()
)
Gui.Selection.addObserver(_simulation_parameters_selection_observer)

def _normalize(vector):
    import FreeCAD as App

    vector = App.Vector(vector.x, vector.y, vector.z)
    if vector.Length == 0:
        return vector
    vector.normalize()
    return vector


def _orthogonal_vector(vector):
    import FreeCAD as App

    if abs(vector.x) < abs(vector.y):
        candidate = App.Vector(1, 0, 0)
    else:
        candidate = App.Vector(0, 1, 0)
    return _normalize(vector.cross(candidate))


def _edge_selection():
    import FreeCADGui as Gui

    candidates = []
    for getter in (
        lambda: Gui.Selection.getSelectionEx("*", 0, True),
        lambda: Gui.Selection.getSelectionEx("*", 1, True),
        lambda: Gui.Selection.getPickedList("*"),
        lambda: [Gui.Selection.getPreselection()],
    ):
        try:
            candidates.extend(getter())
        except Exception:
            pass

    for selected in candidates:
        selection = _selection_from_candidate(selected)
        if selection is not None:
            return selection

    return None


def _selection_from_candidate(selected):
    if selected is None:
        return None
    if len(selected.SubElementNames) != 1:
        return None

    part = selected.Object
    sub_name = selected.SubElementNames[0]
    edge_name = sub_name.rstrip(".").rsplit(".", 1)[-1]
    if not part.isDerivedFrom("MbDFEM::MbDPart") or not edge_name.startswith("Edge"):
        return None

    edge = None
    if len(selected.SubObjects) == 1 and getattr(selected.SubObjects[0], "ShapeType", None) == "Edge":
        edge = selected.SubObjects[0]

    if edge is None and sub_name == edge_name:
        try:
            edge = part.Shape.getElement(edge_name)
        except Exception:
            return None

    if edge is None:
        return None

    if not hasattr(edge, "Curve") or edge.Curve.TypeId != "Part::GeomCircle":
        return None

    return part, sub_name, edge


def _circle_edge_placement(edge):
    import FreeCAD as App

    center = edge.Curve.Location
    z_axis = _normalize(edge.Curve.Axis)
    x_axis = edge.valueAt(edge.FirstParameter) - center
    if x_axis.Length == 0:
        x_axis = _orthogonal_vector(z_axis)
    else:
        x_axis = _normalize(x_axis)
    y_axis = _normalize(z_axis.cross(x_axis))
    x_axis = _normalize(y_axis.cross(z_axis))

    return App.Placement(center, App.Rotation(x_axis, y_axis, z_axis, "ZXY"))


class CreateMbDAssemblyCommand:
    """Command that adds an MbDAssembly object to the active document."""

    def GetResources(self):
        return {
            "MenuText": "MbDAssembly",
            "ToolTip": "Add an MbDAssembly to the active document",
        }

    def IsActive(self):
        return True

    def Activated(self):
        import FreeCAD as App
        import FreeCADGui as Gui

        document = App.ActiveDocument
        if document is None:
            document = App.newDocument("MbDFEM")

        document.openTransaction("Create MbDAssembly")
        try:
            assembly = document.addObject("MbDFEM::MbDAssembly", "MbDAssembly")
            assembly.ensureGravity()
            assembly.ensureSimulationParameters()
            assembly.ensureAnimationParameters()
            document.commitTransaction()
        except Exception:
            document.abortTransaction()
            raise

        document.recompute()
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(assembly)


Gui.addCommand("MbDFEM_CreateMbDAssembly", CreateMbDAssemblyCommand())


class CreateMbDMarkerCommand:
    """Command that adds an MbDMarker from a selected MbDPart circular reference."""

    @staticmethod
    def _normalize(vector):
        import FreeCAD as App

        vector = App.Vector(vector.x, vector.y, vector.z)
        if vector.Length == 0:
            return vector
        vector.normalize()
        return vector

    def _orthogonal_vector(self, vector):
        import FreeCAD as App

        if abs(vector.x) < abs(vector.y):
            candidate = App.Vector(1, 0, 0)
        else:
            candidate = App.Vector(0, 1, 0)
        return self._normalize(vector.cross(candidate))

    def _reference_selection(self):
        import FreeCADGui as Gui

        candidates = []
        for getter in (
            lambda: Gui.Selection.getSelectionEx("*", 0, True),
            lambda: Gui.Selection.getSelectionEx("*", 1, True),
            lambda: Gui.Selection.getPickedList("*"),
            lambda: [Gui.Selection.getPreselection()],
        ):
            try:
                candidates.extend(getter())
            except Exception:
                pass

        for selected in candidates:
            selection = self._selection_from_candidate(selected)
            if selection is not None:
                return selection

        return None

    @staticmethod
    def _selection_from_candidate(selected):
        if selected is None:
            return None

        try:
            part = selected.Object
            sub_element_names = selected.SubElementNames
        except RuntimeError:
            return None

        if len(sub_element_names) != 1:
            return None

        sub_name = sub_element_names[0]
        element_name = sub_name.rstrip(".").rsplit(".", 1)[-1]
        if part is None or not part.isDerivedFrom("MbDFEM::MbDPart"):
            return None

        element = None
        try:
            element = part.Shape.getElement(element_name)
        except Exception:
            pass

        if element is None:
            return None

        if element_name.startswith("Edge"):
            if not hasattr(element, "Curve") or element.Curve.TypeId != "Part::GeomCircle":
                return None
        elif element_name.startswith("Face"):
            if not hasattr(element, "Surface") or element.Surface.TypeId != "Part::GeomCylinder":
                return None
        else:
            return None

        return part, element_name, element

    def _circle_edge_placement(self, edge):
        import FreeCAD as App

        center = edge.Curve.Location
        z_axis = self._normalize(edge.Curve.Axis)
        x_axis = edge.valueAt(edge.FirstParameter) - center
        if x_axis.Length == 0:
            x_axis = self._orthogonal_vector(z_axis)
        else:
            x_axis = self._normalize(x_axis)
        y_axis = self._normalize(z_axis.cross(x_axis))
        x_axis = self._normalize(y_axis.cross(z_axis))

        return App.Placement(center, App.Rotation(x_axis, y_axis, z_axis, "ZXY"))

    def _cylindrical_face_placement(self, face):
        import FreeCAD as App

        surface = face.Surface
        z_axis = self._normalize(surface.Axis)
        if z_axis.Length == 0:
            z_axis = App.Vector(0, 0, 1)

        axis_origin = getattr(surface, "Center", getattr(surface, "Location", App.Vector()))
        face_center = face.CenterOfGravity
        center = axis_origin + z_axis * ((face_center - axis_origin).dot(z_axis))

        x_axis = face_center - center
        if x_axis.Length == 0:
            x_axis = self._orthogonal_vector(z_axis)
        else:
            x_axis = self._normalize(x_axis)
        y_axis = self._normalize(z_axis.cross(x_axis))
        x_axis = self._normalize(y_axis.cross(z_axis))

        return App.Placement(center, App.Rotation(x_axis, y_axis, z_axis, "ZXY"))

    def _reference_placement(self, element):
        if getattr(element, "ShapeType", None) == "Face":
            return self._cylindrical_face_placement(element)
        return self._circle_edge_placement(element)

    @staticmethod
    def _marker_label(part, sub_name):
        return f"MbDMarker ({part.Name}.{sub_name.rstrip('.')})"

    def GetResources(self):
        return {
            "MenuText": "MbDMarker",
            "ToolTip": "Add an MbDMarker referencing the selected circular edge or cylindrical face",
        }

    def IsActive(self):
        import FreeCAD as App

        return App.ActiveDocument is not None

    def Activated(self):
        import FreeCADGui as Gui

        reference_selection = self._reference_selection()
        if reference_selection is None:
            return

        part, sub_name, element = reference_selection
        document = part.Document
        if document is None:
            return

        marker_placement = self._reference_placement(element)

        document.openTransaction("Create MbDMarker")
        try:
            marker = document.addObject("MbDFEM::MbDMarker", "MbDMarker")
            marker.Geometry = (part, [sub_name])
            marker.Label = self._marker_label(part, sub_name)
            marker.Placement = marker_placement
            part.addMarker(marker)
            document.commitTransaction()
        except Exception:
            document.abortTransaction()
            raise

        document.recompute()
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(marker)


Gui.addCommand("MbDFEM_CreateMbDMarker", CreateMbDMarkerCommand())


def _is_mbd_marker(object_):
    try:
        return object_ is not None and object_.isDerivedFrom("MbDFEM::MbDMarker")
    except Exception:
        return False


def _is_mbd_mass_marker(object_):
    try:
        return object_ is not None and object_.isDerivedFrom("MbDFEM::MbDMassMarker")
    except Exception:
        return False


def _marker_from_document_reference(document_name, object_name, sub_name=""):
    import FreeCAD as App

    document = App.getDocument(document_name)
    if document is None:
        return None

    root = document.getObject(object_name)
    if root is None:
        return None
    if _is_mbd_marker(root):
        return root

    if sub_name:
        try:
            resolved = root.getSubObject(sub_name)
            if _is_mbd_marker(resolved):
                return resolved
        except Exception:
            pass

        marker_name = sub_name.rstrip(".").rsplit(".", 1)[-1]
        marker = document.getObject(marker_name)
        if _is_mbd_marker(marker):
            return marker

    return None


def _selected_markers():
    import FreeCAD as App
    import FreeCADGui as Gui

    markers = []
    seen = set()
    document_name = App.ActiveDocument.Name if App.ActiveDocument else "*"

    try:
        selection = Gui.Selection.getSelectionEx(document_name)
    except Exception:
        selection = []

    for selected in selection:
        marker = None
        try:
            if _is_mbd_marker(selected.Object):
                marker = selected.Object
            elif selected.SubElementNames:
                marker = _marker_from_document_reference(
                    selected.Object.Document.Name, selected.Object.Name, selected.SubElementNames[0]
                )
        except Exception:
            marker = None

        if marker is not None and marker.Name not in seen:
            markers.append(marker)
            seen.add(marker.Name)

    return markers


def _selected_mass_markers():
    return [marker for marker in _selected_markers() if _is_mbd_mass_marker(marker)]


def _part_containing_marker(marker):
    if marker is None or marker.Document is None:
        return None

    for object_ in marker.Document.Objects:
        try:
            if object_.isDerivedFrom("MbDFEM::MbDPart"):
                if marker in object_.markers:
                    return object_
                if hasattr(object_, "getMassMarker") and object_.getMassMarker() is marker:
                    return object_
        except Exception:
            pass

    return None


def _assembly_containing_part(part):
    if part is None or part.Document is None:
        return None

    for object_ in part.Document.Objects:
        try:
            if object_.isDerivedFrom("MbDFEM::MbDAssembly"):
                if part in object_.parts or part in object_.fixedparts:
                    return object_
        except Exception:
            pass

    return None


def _assembly_for_markers(markers):
    if len(markers) != 2:
        return None

    parts = [_part_containing_marker(marker) for marker in markers]
    if parts[0] is None or parts[1] is None:
        return None

    assemblies = [_assembly_containing_part(part) for part in parts]
    if assemblies[0] is not None and assemblies[0] == assemblies[1]:
        return assemblies[0]

    return None


class CreateMbDJointTaskPanel:
    """Interactive task panel that creates an MbDJoint from two selected markers."""

    joint_types = [
        "Fixed",
        "Revolute",
        "Prismatic",
        "Cylindrical",
        "Spherical",
        "Universal",
        "Planar",
        "Distance",
        "Gear",
        "RackPinion",
    ]

    def __init__(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets

        self.markers = []
        self._updating_selection = False

        self.form = QtWidgets.QWidget()
        self.form.setWindowTitle("Create MbDJoint")

        layout = QtWidgets.QVBoxLayout(self.form)
        type_layout = QtWidgets.QHBoxLayout()
        type_layout.addWidget(QtWidgets.QLabel("jointType:"))
        self.joint_type_combo = QtWidgets.QComboBox()
        self.joint_type_combo.addItems(self.joint_types)
        type_layout.addWidget(self.joint_type_combo)
        self.marker_i_label = QtWidgets.QLabel()
        self.marker_j_label = QtWidgets.QLabel()
        self.status_label = QtWidgets.QLabel()
        self.status_label.setWordWrap(True)

        layout.addLayout(type_layout)
        layout.addWidget(self.marker_i_label)
        layout.addWidget(self.marker_j_label)
        layout.addWidget(self.status_label)

        self._set_markers(self._selected_markers()[:2], sync_selection=False)
        Gui.Selection.addObserver(self)

    def getStandardButtons(self):
        from PySide import QtGui

        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

    def accept(self):
        import FreeCADGui as Gui

        if not self._ready():
            return False

        marker_i, marker_j = self.markers
        assembly = self._assembly_for_markers(self.markers)
        document = assembly.Document

        document.openTransaction("Create MbDJoint")
        try:
            joint_type = self.joint_type_combo.currentText()
            joint = document.addObject("MbDFEM::MbDJoint", "MbDJoint")
            joint.markerI = marker_i
            joint.markerJ = marker_j
            joint.jointType = joint_type
            joint.Label = f"{joint_type} MbDJoint"
            assembly.addJoint(joint)
            document.commitTransaction()
        except Exception:
            document.abortTransaction()
            raise

        document.recompute()
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(joint)
        self._deactivate()
        return True

    def reject(self):
        self._deactivate()
        return True

    def addSelection(self, document_name, object_name, sub_name, mouse_position):
        marker = self._marker_from_document_reference(document_name, object_name, sub_name)
        if marker is None:
            import FreeCADGui as Gui

            Gui.Selection.removeSelection(document_name, object_name, sub_name)
            return
        if marker in self.markers:
            return
        if len(self.markers) >= 2:
            self._set_markers([self.markers[0], marker])
        else:
            self._set_markers(self.markers + [marker])

    def removeSelection(self, document_name, object_name, sub_name, mouse_position=None):
        marker = self._marker_from_document_reference(document_name, object_name, sub_name)
        if marker in self.markers:
            self._set_markers([selected for selected in self.markers if selected != marker])

    def clearSelection(self, document_name):
        if not self._updating_selection:
            self._set_markers([], sync_selection=False)

    def _deactivate(self):
        import FreeCADGui as Gui

        try:
            Gui.Selection.removeObserver(self)
        except Exception:
            pass
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()

    def _ready(self):
        return len(self.markers) == 2 and self._assembly_for_markers(self.markers) is not None

    def _set_markers(self, markers, sync_selection=True):
        import FreeCADGui as Gui

        self.markers = []
        seen = set()
        for marker in markers:
            if marker is not None and marker.Name not in seen:
                self.markers.append(marker)
                seen.add(marker.Name)

        if sync_selection:
            self._updating_selection = True
            try:
                Gui.Selection.clearSelection()
                for marker in self.markers:
                    Gui.Selection.addSelection(marker)
            finally:
                self._updating_selection = False

        self._update_labels()

    def _update_labels(self):
        marker_i = self.markers[0].Label if len(self.markers) >= 1 else "<select markerI>"
        marker_j = self.markers[1].Label if len(self.markers) >= 2 else "<select markerJ>"

        self.marker_i_label.setText(f"markerI: {marker_i}")
        self.marker_j_label.setText(f"markerJ: {marker_j}")

        if len(self.markers) < 2:
            self.status_label.setText("Select two MbDMarker objects in the tree or canvas.")
        elif self.markers[0] == self.markers[1]:
            self.status_label.setText("Select two different MbDMarker objects.")
        elif self._assembly_for_markers(self.markers) is None:
            self.status_label.setText("The selected markers must belong to parts in the same MbDAssembly.")
        else:
            self.status_label.setText("Ready to create MbDJoint.")

    @staticmethod
    def _is_mbd_marker(object_):
        try:
            return object_ is not None and object_.isDerivedFrom("MbDFEM::MbDMarker")
        except Exception:
            return False

    @classmethod
    def _marker_from_document_reference(cls, document_name, object_name, sub_name=""):
        import FreeCAD as App

        document = App.getDocument(document_name)
        if document is None:
            return None

        root = document.getObject(object_name)
        if root is None:
            return None
        if cls._is_mbd_marker(root):
            return root

        if sub_name:
            try:
                resolved = root.getSubObject(sub_name)
                if cls._is_mbd_marker(resolved):
                    return resolved
            except Exception:
                pass

            marker_name = sub_name.rstrip(".").rsplit(".", 1)[-1]
            marker = document.getObject(marker_name)
            if cls._is_mbd_marker(marker):
                return marker

        return None

    @classmethod
    def _selected_markers(cls):
        import FreeCAD as App
        import FreeCADGui as Gui

        markers = []
        seen = set()
        document_name = App.ActiveDocument.Name if App.ActiveDocument else "*"

        try:
            selection = Gui.Selection.getSelectionEx(document_name)
        except Exception:
            selection = []

        for selected in selection:
            marker = None
            try:
                if cls._is_mbd_marker(selected.Object):
                    marker = selected.Object
                elif selected.SubElementNames:
                    marker = cls._marker_from_document_reference(
                        selected.Object.Document.Name, selected.Object.Name, selected.SubElementNames[0]
                    )
            except Exception:
                marker = None

            if marker is not None and marker.Name not in seen:
                markers.append(marker)
                seen.add(marker.Name)

        return markers

    @staticmethod
    def _part_containing_marker(marker):
        if marker is None or marker.Document is None:
            return None

        for object_ in marker.Document.Objects:
            try:
                if object_.isDerivedFrom("MbDFEM::MbDPart"):
                    if marker in object_.markers:
                        return object_
                    if hasattr(object_, "getMassMarker") and object_.getMassMarker() is marker:
                        return object_
            except Exception:
                pass

        return None

    @staticmethod
    def _assembly_containing_part(part):
        if part is None or part.Document is None:
            return None

        for object_ in part.Document.Objects:
            try:
                if object_.isDerivedFrom("MbDFEM::MbDAssembly"):
                    if part in object_.parts or part in object_.fixedparts:
                        return object_
            except Exception:
                pass

        return None

    @classmethod
    def _assembly_for_markers(cls, markers):
        if len(markers) != 2:
            return None

        parts = [cls._part_containing_marker(marker) for marker in markers]
        if parts[0] is None or parts[1] is None:
            return None

        assemblies = [cls._assembly_containing_part(part) for part in parts]
        if assemblies[0] is not None and assemblies[0] == assemblies[1]:
            return assemblies[0]

        return None


class CreateMbDJointCommand:
    """Command that interactively creates an MbDJoint from two MbDMarkers."""

    def __init__(self, panel_class):
        self.panel_class = panel_class

    def GetResources(self):
        return {
            "MenuText": "MbDJoint",
            "ToolTip": "Create an MbDJoint between two selected MbDMarkers",
        }

    def IsActive(self):
        import FreeCAD as App

        return App.ActiveDocument is not None

    def Activated(self):
        import FreeCADGui as Gui

        panel = self.panel_class()
        Gui.Control.showDialog(panel)


Gui.addCommand("MbDFEM_CreateMbDJoint", CreateMbDJointCommand(CreateMbDJointTaskPanel))


class SetMassMarkerMaterialTaskPanel:
    """Task panel that assigns a physical material to selected MbDMassMarker objects."""

    def __init__(self, markers):
        import FreeCADGui as Gui
        import MatGui
        import Materials
        from PySide import QtCore, QtWidgets

        self.markers = markers
        self.material_manager = Materials.MaterialManager()
        self.form = QtWidgets.QWidget()
        self.form.setWindowTitle("Set Mass Marker Material")

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

        if len(markers) == 1:
            try:
                self.material_tree.UUID = markers[0].material.UUID
            except Exception:
                pass

        self.uuid = self.material_tree.UUID
        QtCore.QObject.connect(
            self.material_widget,
            QtCore.SIGNAL("onMaterial(QString)"),
            self._on_material,
        )

        layout = QtWidgets.QVBoxLayout(self.form)
        layout.addWidget(self.material_widget)

    def getStandardButtons(self):
        from PySide import QtGui

        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

    def accept(self):
        import FreeCAD as App
        import FreeCADGui as Gui

        self.uuid = self.material_tree.UUID
        if not self.uuid:
            App.Console.PrintError("No material selected.\n")
            return False

        try:
            material = self.material_manager.getMaterial(self.uuid)
        except Exception as exc:
            App.Console.PrintError(f"Selected material could not be loaded: {exc}\n")
            return False

        document = self.markers[0].Document if self.markers else App.ActiveDocument
        if document is None:
            return False

        document.openTransaction("Set Mass Marker Material")
        try:
            for marker in self.markers:
                marker.material = material
                part = self._part_containing_marker(marker)
                if part is not None and marker.massMarkerFromShape:
                    part.populateMassMarkerFromShape()
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

    def _on_material(self, uuid):
        self.uuid = uuid

    @staticmethod
    def _part_containing_marker(marker):
        if marker is None or marker.Document is None:
            return None

        for object_ in marker.Document.Objects:
            try:
                if object_.isDerivedFrom("MbDFEM::MbDPart"):
                    if marker in object_.markers:
                        return object_
                    if hasattr(object_, "getMassMarker") and object_.getMassMarker() is marker:
                        return object_
            except Exception:
                pass

        return None


class SetMassMarkerMaterialCommand:
    """Command that assigns material to selected MbDMassMarker objects."""

    def __init__(self, panel_class):
        self.panel_class = panel_class

    def GetResources(self):
        return {
            "MenuText": "Set Mass Marker Material",
            "ToolTip": "Assign material density for selected MbDMassMarker objects",
        }

    def IsActive(self):
        import FreeCAD as App

        return App.ActiveDocument is not None

    def Activated(self):
        import FreeCAD as App
        import FreeCADGui as Gui

        markers = self._selected_mass_markers()
        if not markers:
            App.Console.PrintError("Select one or more MbDMassMarker objects.\n")
            return

        Gui.Control.showDialog(self.panel_class(markers))

    @staticmethod
    def _marker_from_document_reference(document_name, object_name, sub_name=""):
        import FreeCAD as App

        document = App.getDocument(document_name)
        if document is None:
            return None

        root = document.getObject(object_name)
        if root is None:
            return None

        try:
            if root.isDerivedFrom("MbDFEM::MbDMassMarker"):
                return root
        except Exception:
            pass

        if sub_name:
            try:
                resolved = root.getSubObject(sub_name)
                if resolved is not None and resolved.isDerivedFrom("MbDFEM::MbDMassMarker"):
                    return resolved
            except Exception:
                pass

            marker_name = sub_name.rstrip(".").rsplit(".", 1)[-1]
            marker = document.getObject(marker_name)
            try:
                if marker is not None and marker.isDerivedFrom("MbDFEM::MbDMassMarker"):
                    return marker
            except Exception:
                pass

        return None

    @classmethod
    def _selected_mass_markers(cls):
        import FreeCAD as App
        import FreeCADGui as Gui

        markers = []
        seen = set()
        document_name = App.ActiveDocument.Name if App.ActiveDocument else "*"

        try:
            selection = Gui.Selection.getSelectionEx(document_name)
        except Exception:
            selection = []

        for selected in selection:
            marker = None
            try:
                obj = selected.Object
                if obj is not None and obj.isDerivedFrom("MbDFEM::MbDMassMarker"):
                    marker = obj
                elif selected.SubElementNames:
                    marker = cls._marker_from_document_reference(
                        obj.Document.Name, obj.Name, selected.SubElementNames[0]
                    )
            except Exception:
                marker = None

            if marker is not None and marker.Name not in seen:
                markers.append(marker)
                seen.add(marker.Name)

        return markers


Gui.addCommand(
    "MbDFEM_SetMassMarkerMaterial",
    SetMassMarkerMaterialCommand(SetMassMarkerMaterialTaskPanel),
)


class SolveMbDAssemblyTaskPanel:
    """Task panel for simulation parameters and ASMT operations."""

    def __init__(self, assembly):
        import FreeCAD as App
        import FreeCADMbDBackend
        from PySide import QtWidgets

        self.assembly = assembly
        self.parameters = assembly.ensureSimulationParameters()
        self.form = QtWidgets.QWidget()
        self.form.setWindowTitle("Solve MbDAssembly")

        asmt_path = FreeCADMbDBackend.default_asmt_path(assembly)
        solved_path = asmt_path.with_suffix(".solved.asmt")

        layout = QtWidgets.QVBoxLayout(self.form)

        parameters_group = QtWidgets.QGroupBox("Simulation Parameters")
        form_layout = QtWidgets.QFormLayout(parameters_group)

        self.start_time = self._double_spinbox(self.parameters.startTime, -1.0e12, 1.0e12)
        self.end_time = self._double_spinbox(self.parameters.endTime, -1.0e12, 1.0e12)
        self.min_step_size = self._double_spinbox(self.parameters.minStepSize, 0.0, 1.0e12)
        self.max_step_size = self._double_spinbox(self.parameters.maxStepSize, 1.0e-12, 1.0e12)
        self.output_interval = self._double_spinbox(self.parameters.outputInterval, 0.0, 1.0e12)
        self.significant_digits = self._integer_spinbox(
            self.parameters.significantDigits, 1, 16
        )
        self.max_iterations = self._integer_spinbox(self.parameters.maxIterations, 1, 1000000)

        form_layout.addRow("startTime", self.start_time)
        form_layout.addRow("endTime", self.end_time)
        form_layout.addRow("outputInterval", self.output_interval)
        form_layout.addRow("minStepSize", self.min_step_size)
        form_layout.addRow("maxStepSize", self.max_step_size)
        form_layout.addRow("significantDigits", self.significant_digits)
        form_layout.addRow("maxIterations", self.max_iterations)
        layout.addWidget(parameters_group)

        files_group = QtWidgets.QGroupBox("ASMT Files")
        files_layout = QtWidgets.QFormLayout(files_group)
        self.asmt_file = QtWidgets.QLineEdit(str(asmt_path))
        self.solved_asmt_file = QtWidgets.QLineEdit(str(solved_path))
        files_layout.addRow("ASMT", self.asmt_file)
        files_layout.addRow("Solved ASMT", self.solved_asmt_file)
        layout.addWidget(files_group)

        actions_layout = QtWidgets.QHBoxLayout()
        self.export_button = QtWidgets.QPushButton("Export ASMT")
        self.simulate_button = QtWidgets.QPushButton("Simulate ASMT")
        self.import_button = QtWidgets.QPushButton("Import ASMT")
        self.solve_button = QtWidgets.QPushButton("Solve MbDAssembly")
        actions_layout.addWidget(self.export_button)
        actions_layout.addWidget(self.simulate_button)
        actions_layout.addWidget(self.import_button)
        actions_layout.addWidget(self.solve_button)
        layout.addLayout(actions_layout)

        self.status_label = QtWidgets.QLabel()
        self.status_label.setWordWrap(True)
        layout.addWidget(self.status_label)

        self.export_button.clicked.connect(self.export_asmt)
        self.simulate_button.clicked.connect(self.simulate_asmt)
        self.import_button.clicked.connect(self.import_asmt)
        self.solve_button.clicked.connect(self.solve_assembly)

        App.Console.PrintMessage(f"Solving assembly: {assembly.Label}\n")

    @staticmethod
    def _double_spinbox(value, minimum, maximum):
        from PySide import QtWidgets

        field = QtWidgets.QDoubleSpinBox()
        field.setDecimals(12)
        field.setRange(minimum, maximum)
        field.setValue(float(value))
        return field

    @staticmethod
    def _integer_spinbox(value, minimum, maximum):
        from PySide import QtWidgets

        field = QtWidgets.QSpinBox()
        field.setRange(minimum, maximum)
        field.setValue(int(value))
        return field

    def getStandardButtons(self):
        from PySide import QtGui

        return QtGui.QDialogButtonBox.Close

    def accept(self):
        self._apply_parameters()
        return True

    def reject(self):
        return True

    def export_asmt(self):
        import FreeCAD as App
        import FreeCADMbDExporter
        from pathlib import Path

        try:
            self._apply_parameters()
            asmt_path = Path(self.asmt_file.text())
            FreeCADMbDExporter.export_assembly(self.assembly, asmt_path)
            self._set_status(f"Exported ASMT: {asmt_path}")
            App.Console.PrintMessage(f"Exported FreeCADMbD input: {asmt_path}\n")
            return True
        except Exception as exc:
            self._report_error("ASMT export failed", exc)
            return False

    def simulate_asmt(self):
        import FreeCAD as App
        import FreeCADMbDBackend
        from pathlib import Path

        try:
            self._apply_parameters()
            asmt_path = Path(self.asmt_file.text())
            solved_path = Path(self.solved_asmt_file.text())
            backend = FreeCADMbDBackend.FreeCADMbDProcessBackend()
            solved_path, completed = backend.simulate_asmt(asmt_path, solved_path)
            self.solved_asmt_file.setText(str(solved_path))
            self._set_status(f"Simulated ASMT: {solved_path}")
            App.Console.PrintMessage(f"Wrote FreeCADMbD solved assembly: {solved_path}\n")
            if completed.stdout:
                App.Console.PrintMessage(completed.stdout)
            if completed.stderr:
                App.Console.PrintWarning(completed.stderr)
            return True
        except Exception as exc:
            self._report_error("ASMT simulation failed", exc)
            return False

    def import_asmt(self):
        import FreeCAD as App
        import FreeCADGui as Gui
        import FreeCADMbDResults
        from pathlib import Path

        try:
            self._apply_parameters()
            solved_path = Path(self.solved_asmt_file.text())
            imported = FreeCADMbDResults.import_results(self.assembly, solved_path)
            self._set_status(f"Imported ASMT results: {solved_path}")
            App.Console.PrintMessage(
                f"Imported FreeCADMbD results from: {solved_path}\n"
            )
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(self.assembly)
            self.assembly.Document.recompute()
            App.Console.PrintMessage(f"Updated {len(imported)} result objects.\n")
            return True
        except Exception as exc:
            self._report_error("ASMT import failed", exc)
            return False

    def solve_assembly(self):
        if self.export_asmt():
            if self.simulate_asmt():
                self.import_asmt()

    def _apply_parameters(self):
        self.parameters.startTime = self.start_time.value()
        self.parameters.endTime = self.end_time.value()
        self.parameters.maxStepSize = self.max_step_size.value()
        self.parameters.minStepSize = self.min_step_size.value()
        self.parameters.significantDigits = self.significant_digits.value()
        self.parameters.maxIterations = self.max_iterations.value()
        self.parameters.outputInterval = self.output_interval.value()

    def _set_status(self, message):
        import FreeCAD as App

        self.status_label.setText(message)
        App.Console.PrintMessage(message + "\n")

    def _report_error(self, action, exc):
        import FreeCAD as App

        message = f"{action}: {exc}"
        self.status_label.setText(message)
        App.Console.PrintError(message + "\n")


class SolveMbDAssemblyCommand:
    """Command that opens the MbDAssembly solve task panel."""

    def __init__(self, panel_class=None):
        self.panel_class = panel_class or FreeCADMbDSimulationPanel.SimulationTaskPanel

    def GetResources(self):
        return {
            "MenuText": "Solve MbDAssembly",
            "ToolTip": "Open ASMT export, simulation, and import controls",
        }

    def IsActive(self):
        import FreeCAD as App

        return App.ActiveDocument is not None

    @staticmethod
    def activeAssembly():
        import FreeCAD as App
        import FreeCADGui as Gui

        document = App.ActiveDocument
        if document is None:
            return None

        try:
            selection = Gui.Selection.getSelectionEx(document.Name)
        except Exception:
            selection = []

        for selected in selection:
            try:
                obj = selected.Object
                if obj is not None and obj.isDerivedFrom("MbDFEM::MbDAssembly"):
                    return obj
            except Exception:
                pass

        for obj in document.Objects:
            try:
                if obj.isDerivedFrom("MbDFEM::MbDAssembly"):
                    return obj
            except Exception:
                pass

        return None

    def Activated(self):
        import FreeCAD as App
        import FreeCADGui as Gui

        assembly = self.activeAssembly()
        if assembly is None:
            App.Console.PrintError("No MbDAssembly is active or selected.\n")
            return

        Gui.Control.showDialog(self.panel_class(assembly))


Gui.addCommand(
    "MbDFEM_SolveMbDAssembly",
    SolveMbDAssemblyCommand(FreeCADMbDSimulationPanel.SimulationTaskPanel),
)


class MbDFEMWorkbench(Gui.Workbench):
    """Minimal MbDFEM workbench."""

    MenuText = "MbDFEM"
    ToolTip = "MbDFEM workbench"

    def Initialize(self):
        import Part  # noqa: F401
        import PartGui  # noqa: F401
        import MbDFEM  # noqa: F401
        import MbDFEMGui  # noqa: F401

        toolbar_commands = [
            "MbDFEM_CreateMbDAssembly",
            "MbDFEM_CreateMbDMarker",
            "MbDFEM_CreateMbDJoint",
        ]
        menu_commands = [
            *toolbar_commands,
        ]
        self.appendToolbar("MbDFEM", toolbar_commands)
        self.appendMenu("MbDFEM", menu_commands)

    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(MbDFEMWorkbench())
