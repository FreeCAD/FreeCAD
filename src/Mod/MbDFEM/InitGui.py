# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCADGui as Gui

import FreeCAD
import PartGui  # noqa: F401
import MbDFEMGui  # noqa: F401

FreeCAD.__unit_test__ += ["TestMbDFEMGui"]

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

        try:
            sub_objects = selected.SubObjects
        except RuntimeError:
            sub_objects = []

        if len(sub_objects) == 1:
            sub_object = sub_objects[0]
            try:
                if getattr(sub_object, "ShapeType", None) in ("Edge", "Face"):
                    element = sub_object
            except RuntimeError:
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


def _part_containing_marker(marker):
    if marker is None or marker.Document is None:
        return None

    for object_ in marker.Document.Objects:
        try:
            if object_.isDerivedFrom("MbDFEM::MbDPart") and marker in object_.markers:
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
                if object_.isDerivedFrom("MbDFEM::MbDPart") and marker in object_.markers:
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


class SolveMbDAssemblyCommand:
    """Command that exports an MbDAssembly and solves it with FreeCADMbD."""

    def GetResources(self):
        return {
            "MenuText": "Solve MbDAssembly",
            "ToolTip": "Export the active MbDAssembly, run FreeCADMbD, and import result frames",
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

        import FreeCADMbDBackend

        assembly = self.activeAssembly()
        if assembly is None:
            App.Console.PrintError("No MbDAssembly is active or selected.\n")
            return

        document = assembly.Document
        document.openTransaction("Solve MbDAssembly")
        try:
            result = FreeCADMbDBackend.FreeCADMbDProcessBackend().solve(assembly)
            document.commitTransaction()
        except Exception as exc:
            document.abortTransaction()
            App.Console.PrintError(f"FreeCADMbD solve failed: {exc}\n")
            raise

        App.Console.PrintMessage(f"Exported FreeCADMbD input: {result.asmt_file}\n")
        if result.result_file:
            App.Console.PrintMessage(f"Imported FreeCADMbD results: {result.result_file}\n")
        else:
            App.Console.PrintWarning("FreeCADMbD produced no .results.json file to import.\n")
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(assembly)


Gui.addCommand("MbDFEM_SolveMbDAssembly", SolveMbDAssemblyCommand())


class MbDFEMWorkbench(Gui.Workbench):
    """Minimal MbDFEM workbench."""

    MenuText = "MbDFEM"
    ToolTip = "MbDFEM workbench"

    def Initialize(self):
        import Part  # noqa: F401
        import PartGui  # noqa: F401
        import MbDFEM  # noqa: F401
        import MbDFEMGui  # noqa: F401

        commands = [
            "MbDFEM_CreateMbDAssembly",
            "MbDFEM_CreateMbDMarker",
            "MbDFEM_CreateMbDJoint",
            "MbDFEM_SolveMbDAssembly",
        ]
        self.appendToolbar("MbDFEM", commands)
        self.appendMenu("MbDFEM", commands)

    def ContextMenu(self, recipient):
        self.appendContextMenu("", ["MbDFEM_CreateMbDMarker", "MbDFEM_CreateMbDJoint"])

    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(MbDFEMWorkbench())
