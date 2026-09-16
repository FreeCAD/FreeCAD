# SPDX-License-Identifier: LGPL-2.1-or-later

"""Geometry snapshots in the text format produced by Sketcher's Copy Elements.

Only the geometry-building vocabulary emitted by PythonConverter is accepted.
Files are parsed as data; they cannot import modules or execute Python scripts.
Copied constraints are read for format compatibility. The inserted snapshot is
controlled by its Group handle, rather than the source sketch's dimensions.
"""

import ast
import math
from pathlib import Path

import FreeCAD as App
import Part
import Sketcher

_CONSTRUCTORS = {
    "App.Vector": App.Vector,
    "Part.Point": Part.Point,
    "Part.LineSegment": Part.LineSegment,
    "Part.Circle": Part.Circle,
    "Part.ArcOfCircle": Part.ArcOfCircle,
    "Part.Ellipse": Part.Ellipse,
    "Part.ArcOfEllipse": Part.ArcOfEllipse,
    "Part.Hyperbola": Part.Hyperbola,
    "Part.ArcOfHyperbola": Part.ArcOfHyperbola,
    "Part.Parabola": Part.Parabola,
    "Part.ArcOfParabola": Part.ArcOfParabola,
    "Part.BSplineCurve": Part.BSplineCurve,
    "Sketcher.Constraint": Sketcher.Constraint,
}
_LISTS = {"geoList", "constrGeoList", "constraintList"}
_GEOMETRY_TYPES = tuple(value for name, value in _CONSTRUCTORS.items() if name.startswith("Part."))


_METADATA_PREFIX = "# Sketcher block fixed size: "


def _fixed_size(text):
    for line in text.splitlines():
        if line.startswith(_METADATA_PREFIX):
            value = line[len(_METADATA_PREFIX) :].strip()
            if value not in ("true", "false"):
                raise ValueError("Invalid block fixed size setting")
            return value == "true"
    return False


def metadata(filename):
    """Read per-block insertion defaults; legacy and stock blocks are scalable."""
    return {"fixed_size": _fixed_size(Path(filename).read_text(encoding="utf-8-sig"))}


def read(filename, with_constraints=False):
    """Read a .txt block as Part geometry, including construction flags."""
    text = Path(filename).read_text(encoding="utf-8-sig")
    return _read_text(text, filename, with_constraints)


def _read_text(text, filename, with_constraints=False):
    if not text.startswith("# Copied from sketcher."):
        raise ValueError("Expected text produced by Sketcher's Copy Elements command")
    variables = {"lastGeoId": 0}
    geometry = []
    constraints = []

    def value(node):
        if isinstance(node, ast.Constant) and type(node.value) in (str, int, float, bool):
            if isinstance(node.value, float) and not math.isfinite(node.value):
                raise ValueError("Block coordinates must be finite")
            return node.value
        if isinstance(node, ast.List):
            return [value(item) for item in node.elts]
        if isinstance(node, ast.Name) and node.id in variables:
            return variables[node.id]
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.USub, ast.UAdd)):
            number = value(node.operand)
            if type(number) not in (int, float):
                raise ValueError("Expected a number")
            return -number if isinstance(node.op, ast.USub) else number
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
            left, right = value(node.left), value(node.right)
            if type(left) is int and type(right) is int:
                return left + right
        if isinstance(node, ast.Call) and not node.keywords:
            name = ast.unparse(node.func)
            if name == "len" and len(node.args) == 1:
                if ast.unparse(node.args[0]) in ("ActiveSketch.Geometry", "objectStr.Geometry"):
                    return 0
            if name in _CONSTRUCTORS:
                return _CONSTRUCTORS[name](*(value(arg) for arg in node.args))
        raise ValueError("Unsupported expression in block file")

    for statement in ast.parse(text, filename=str(filename)).body:
        if isinstance(statement, ast.Assign) and len(statement.targets) == 1:
            target = statement.targets[0]
            if isinstance(target, ast.Name) and target.id in _LISTS | {"lastGeoId"}:
                result = value(statement.value)
                if (target.id in _LISTS and isinstance(result, list)) or (
                    target.id == "lastGeoId" and result == 0
                ):
                    variables[target.id] = result
                    continue
        elif isinstance(statement, ast.Delete):
            if all(isinstance(t, ast.Name) and t.id in _LISTS for t in statement.targets):
                for target in statement.targets:
                    variables.pop(target.id, None)
                continue
        elif isinstance(statement, ast.Expr) and isinstance(statement.value, ast.Call):
            call = statement.value
            name = ast.unparse(call.func)
            if call.keywords:
                raise ValueError("Unexpected keyword arguments in block file")
            if name in {f"{key}.append" for key in _LISTS} and len(call.args) == 1:
                variables[name.split(".")[0]].append(value(call.args[0]))
                continue
            if name == "objectStr.addGeometry" and len(call.args) == 2:
                items, construction = (value(arg) for arg in call.args)
                if type(construction) is not bool:
                    raise ValueError("Expected a construction flag")
                for item in items if isinstance(items, list) else [items]:
                    if not isinstance(item, _GEOMETRY_TYPES):
                        raise ValueError("Expected Part geometry")
                    facade = Sketcher.GeometryFacade(item)
                    facade.Construction = construction
                    geometry.append(facade.Geometry)
                continue
            if name == "objectStr.addConstraint" and len(call.args) == 1:
                items = value(call.args[0])
                if all(
                    isinstance(item, Sketcher.Constraint)
                    for item in (items if isinstance(items, list) else [items])
                ):
                    constraints.extend(items if isinstance(items, list) else [items])
                    continue
        raise ValueError("Unsupported statement in block file")
    if not geometry:
        raise ValueError("The block file contains no geometry")
    return (geometry, constraints) if with_constraints else geometry


def insert_geometry(sketch, geometry, filename, fixed_size=False, origin=None, angle=0.0):
    """Import geometry at its native size and location as a file-backed Group."""
    if not geometry:
        raise ValueError("The source file contains no geometry")
    if fixed_size:
        elements = []
        for geo in geometry:
            elements.extend((sketch.addGeometry(geo, Sketcher.GeometryFacade(geo).Construction), 0))
        handle = sketch.addGeometry(Part.Point(origin or App.Vector()), True)
        group = Sketcher.Constraint(
            "Group", [handle, 1] + elements, str(Path(filename).resolve()), False, True, angle
        )
        group.Name = Path(filename).stem
        index = sketch.addConstraint(group)
        return sketch.replaceGroupGeometry(index, geometry)
    bounds = Part.makeCompound([geo.toShape() for geo in geometry]).BoundBox
    height = bounds.XLength < 1e-7
    size = bounds.YLength if height else bounds.XLength
    if not math.isfinite(size) or size < 1e-7:
        raise ValueError("The group width or height must be greater than zero")
    if abs(bounds.ZMin) > 1e-7 or abs(bounds.ZMax) > 1e-7:
        raise ValueError("Group geometry must lie in the sketch XY plane")
    start = (
        App.Vector()
        if Path(filename).suffix.lower() == ".txt"
        else App.Vector(bounds.XMin, bounds.YMin, 0)
    )
    end = start + (App.Vector(0, size, 0) if height else App.Vector(size, 0, 0))
    elements = []
    for geo in geometry:
        index = sketch.addGeometry(geo, Sketcher.GeometryFacade(geo).Construction)
        elements.extend((index, 0))
    handle = sketch.addGeometry(Part.LineSegment(start, end), True)
    group = Sketcher.Constraint(
        "Group", [handle, 0] + elements, str(Path(filename).resolve()), height
    )
    group.Name = Path(filename).stem
    return sketch.addConstraint(group)


def reload(sketch, constraint_index):
    """Reload a text block; the caller owns the undo transaction."""
    group = sketch.Constraints[constraint_index]
    if group.Type != "Group" or not group.File:
        raise ValueError("The selected group has no source file")
    return sketch.replaceGroupGeometry(constraint_index, read(group.File))


# Keep observers alive until their edit documents have been closed.
_editors = {}


def _file_key(filename):
    import os

    return os.path.normcase(str(Path(filename).resolve()))


def edit(sketch, constraint_index):
    """Open the source block in an isolated sketch editor after the menu closes."""
    from PySide import QtCore

    group = sketch.Constraints[constraint_index]
    if group.Type != "Group" or Path(group.File).suffix.lower() != ".txt":
        raise ValueError("The selected constraint is not a text block")
    filename = str(Path(group.File).resolve())
    read(filename)  # Report missing/invalid sources before leaving the original sketch.
    QtCore.QTimer.singleShot(0, lambda: _open_editor(sketch, filename))


def _open_editor(sketch, filename):
    import FreeCADGui as Gui

    key = _file_key(filename)
    if key in _editors:
        session = _editors[key]
        App.setActiveDocument(session.document.Name)
        Gui.activeDocument().setEdit(session.sketch.Name)
        return session
    session = _BlockEditor(sketch, filename)
    _editors[key] = session
    return session


class _BlockEditor:
    def __init__(self, original, filename):
        import FreeCADGui as Gui

        self.filename = filename
        self.original_document = original.Document.Name
        self.original_sketch = original.Name
        self.original_bytes = Path(filename).read_bytes()
        self.fixed_size = _fixed_size(self.original_bytes.decode("utf-8-sig"))
        self.error = None
        self.closing = False
        geometry, constraints = read(filename, with_constraints=True)
        if Gui.activeDocument() and Gui.activeDocument().getInEdit():
            Gui.activeDocument().resetEdit()
        self.document = App.newDocument("BlockEdit")
        self.document.Label = App.Qt.translate("SketcherBlock", "Block Edit")
        self.document.UndoMode = 1
        self.sketch = self.document.addObject("Sketcher::SketchObject", "Block")
        for geo in geometry:
            self.sketch.addGeometry(geo, Sketcher.GeometryFacade(geo).Construction)
        if constraints:
            self.sketch.addConstraint(constraints)
        self.document.recompute()
        Gui.addDocumentObserver(self)
        App.addDocumentObserver(self)
        Gui.activeDocument().setEdit(self.sketch.Name)
        Gui.activeDocument().activeView().viewTop()
        Gui.activeDocument().activeView().fitAll()

    def slotResetEdit(self, view):
        from PySide import QtCore

        if not self.closing and view.Object == self.sketch:
            # Wait for Sketcher's edit transaction and task panel to finish closing.
            QtCore.QTimer.singleShot(0, self.finish)

    def slotDeletedDocument(self, document):
        document = getattr(document, "Document", document)
        if getattr(document, "Name", None) == self.document.Name:
            self.closing = True
            self._detach()

    def _detach(self):
        import FreeCADGui as Gui

        Gui.removeDocumentObserver(self)
        App.removeDocumentObserver(self)
        _editors.pop(_file_key(self.filename), None)

    def finish(self):
        import FreeCADGui as Gui
        from PySide import QtCore, QtWidgets

        if self.closing:
            return
        opened = []
        output = None
        try:
            if Path(self.filename).read_bytes() != self.original_bytes:
                raise ValueError("The block file changed while it was being edited")
            if not self.sketch.Geometry or self.sketch.solve() != 0:
                raise ValueError("The block must contain valid, solved geometry")
            # Reuse Sketcher's Copy Elements serialization without touching the clipboard.
            commands = self.sketch.toPythonCommands()
            text = (
                "# Copied from sketcher.\n"
                + _METADATA_PREFIX
                + ("true" if self.fixed_size else "false")
                + "\n"
                + "\n".join(
                    line.replace("ActiveSketch", "objectStr")
                    for line in commands
                    if "exposeInternalGeometry(" not in line
                )
                + "\n"
            )
            geometry = _read_text(text, self.filename)
            output = QtCore.QSaveFile(self.filename)
            if not output.open(QtCore.QIODevice.WriteOnly):
                raise OSError(output.errorString())
            encoded = text.encode("utf-8")
            if output.write(encoded) != len(encoded):
                raise OSError(output.errorString())
            # Update each sketch from the end: replacing members can renumber constraints.
            for document in list(App.listDocuments().values()):
                if document == self.document:
                    continue
                sketches = [
                    obj
                    for obj in document.Objects
                    if obj.isDerivedFrom("Sketcher::SketchObject")
                    and any(
                        c.Type == "Group"
                        and c.File
                        and _file_key(c.File) == _file_key(self.filename)
                        for c in obj.Constraints
                    )
                ]
                if not sketches:
                    continue
                document.openTransaction(App.Qt.translate("SketcherBlock", "Update Block"))
                opened.append(document)
                for sketch in sketches:
                    index = len(sketch.Constraints) - 1
                    while index >= 0:
                        group = sketch.Constraints[index]
                        if (
                            group.Type == "Group"
                            and group.File
                            and _file_key(group.File) == _file_key(self.filename)
                        ):
                            index = sketch.replaceGroupGeometry(index, geometry)
                        index -= 1
                    if sketch.solve() != 0:
                        raise ValueError("A block instance could not be updated: " + sketch.Label)
                document.recompute()
            if not output.commit():
                raise OSError(output.errorString())
            for document in opened:
                document.commitTransaction()
        except Exception as error:
            for document in reversed(opened):
                document.abortTransaction()
            if output is not None:
                output.cancelWriting()
            self.error = str(error)
            App.Console.PrintError(self.error + "\n")
            QtWidgets.QMessageBox.warning(
                Gui.getMainWindow(),
                App.Qt.translate("SketcherBlock", "Cannot Save Block"),
                App.Qt.translate(
                    "SketcherBlock",
                    "The block editor remains open so the changes can be recovered.",
                )
                + "\n\n"
                + self.error,
            )
            return
        self.closing = True
        self._detach()
        App.closeDocument(self.document.Name)
        original = (
            App.getDocument(self.original_document)
            if self.original_document in App.listDocuments()
            else None
        )
        if original:
            App.setActiveDocument(original.Name)
            if original.getObject(self.original_sketch):
                Gui.activeDocument().setEdit(self.original_sketch)
