# SPDX-License-Identifier: LGPL-2.1-or-later

"""Widget and property-editor snapshots for both FreeCAD themes."""

import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

import FreeCAD

from VisualTestSupport import compare_images


_PAGES = (
    "buttons", "text", "numbers", "themed-spinboxes", "labels", "views", "containers",
    "property-states", "property-scalars", "property-geometry", "property-collections",
    "property-editors-states", "property-editors-scalars",
    "property-editors-geometry", "property-editors-collections",
)


def _properties(doc, page):
    target = doc.addObject("App::VarSet", "Target")
    target.Label = "Linked object"
    matrix = FreeCAD.Matrix()
    matrix.move(FreeCAD.Vector(12.5, 25, -5))
    rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 45)
    return {
        "property-scalars": (
            ("Bool", "Boolean", True), ("Integer", "Integer", 42),
            ("IntegerConstraint", "IntegerConstraint", 42), ("Percent", "Percent", 40),
            ("Float", "Float", 12.5), ("FloatConstraint", "FloatConstraint", 12.5),
            ("Precision", "Precision", 0.001), ("Quantity", "Quantity", 12.5),
            ("Length", "Length", 12.5), ("Angle", "Angle", 45),
            ("String", "String", "Sample text"), ("Font", "Font", "Noto Sans"),
            ("Enumeration", "Enumeration", ["First", "Second", "Third"]),
            ("Color", "Color", (0.2, 0.5, 0.9)),
            ("File", "File", "/models/sample.FCStd"), ("Path", "Path", "/models"),
            ("FileIncluded", "IncludedFile", ""),
        ),
        "property-geometry": (
            ("Vector", "Vector", (1, 2, 3)),
            ("VectorDistance", "VectorDistance", (12.5, 25, -5)),
            ("Position", "Position", (12.5, 25, -5)), ("Direction", "Direction", (0, 0, 1)),
            ("Matrix", "Matrix", matrix), ("Rotation", "Rotation", rotation),
            ("Placement", "Placement", FreeCAD.Placement(FreeCAD.Vector(12.5, 25, -5), rotation)),
        ),
        "property-collections": (
            ("StringList", "Strings", ["First", "Second", "Third"]),
            ("IntegerList", "Integers", [1, 2, 42]), ("FloatList", "Floats", [1.25, 2.5, 12.5]),
            ("VectorList", "Vectors", [(1, 2, 3), (12.5, 25, -5)]),
            ("Map", "Map", {"Name": "Sample", "Size": "12.5"}),
            ("Material", "Material", FreeCAD.Material()),
            ("MaterialList", "Materials", [FreeCAD.Material()]),
            ("Link", "Link", target), ("LinkList", "Links", [target]),
            ("LinkSub", "SubLink", target), ("LinkSubList", "SubLinks", [(target, [""])]),
        ),
    }[page]


def _values(doc, page):
    obj = doc.addObject("App::VarSet", "Values")
    names = []
    if page == "property-states":
        for group, prefix, mode in (("Editable", "Editable", 0), ("Read-only", "Locked", 1),
                                    ("Expression", "Bound", 0),
                                    ("Read-only expression", "LockedBound", 1)):
            for kind, suffix, value, expression in (
                ("Bool", "False", False, "0"), ("Bool", "True", True, "1"),
                ("Integer", "Integer", 42, "42"), ("Float", "Float", 12.5, "12.5"),
                ("String", "String", "Sample text", "<<Sample text>>"),
            ):
                name = prefix + suffix
                obj.addProperty("App::Property" + kind, name, group)
                setattr(obj, name, value)
                if "Bound" in prefix:
                    obj.setExpression(name, expression)
                obj.setEditorMode(name, mode)
                names.append(name)
        return obj, names

    if page.startswith("property-"):
        for kind, suffix, value in _properties(doc, page):
            for prefix, mode in (("Normal", 0), ("ReadOnly", 1)):
                name = prefix + suffix
                obj.addProperty("App::Property" + kind, name, "Values")
                if kind == "Quantity":
                    setattr(obj, name, FreeCAD.Units.Unit("mm"))
                setattr(obj, name, value)
                if kind == "Enumeration":
                    setattr(obj, name, 1)
                if kind == "Material":
                    obj.setPropertyStatus(name, "MaterialEdit")
                obj.setEditorMode(name, mode)
                names.append(name)
        return obj, names

    for kind, name, value in (("Length", "Length", 12.5), ("Length", "BoundLength", 12.5),
                              ("Integer", "BoundInteger", 42),
                              ("String", "BoundString", "Sample text"), ("Map", "Map", {})):
        obj.addProperty("App::Property" + kind, name)
        setattr(obj, name, value)
    for name, expression in (("BoundLength", "Length"), ("BoundInteger", "42"),
                             ("BoundString", "<<Sample text>>")):
        obj.setExpression(name, expression)
    return obj, names


class ComponentSnapshotTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not FreeCAD.GuiUp:
            raise unittest.SkipTest("Use FreeCAD -t; GUI widgets are required")
        if os.environ.get("CI") and not sys.platform.startswith("linux"):
            raise unittest.SkipTest("Visual snapshots are only supported on Linux CI")

        from PySide import QtCore, QtGui, QtWidgets
        import QtUnitGui

        cls.fixture = QtUnitGui
        cls.output = Path(os.environ.get("FC_VISUAL_OUT_DIR")
                          or tempfile.mkdtemp(prefix="freecad-component-zoo-"))
        cls.output.mkdir(parents=True, exist_ok=True)
        root = next(parent for parent in Path(__file__).resolve().parents
                    if (parent / "tests/visual/fonts/NotoSans-Regular.ttf").is_file())
        relative = Path("tests/visual/baselines") / Path(__file__).stem
        default = next((parent / relative for parent in Path(__file__).resolve().parents
                        if (parent / relative).is_dir()), root / relative)
        cls.baselines = Path(os.environ.get("FC_VISUAL_BASELINE_DIR", default))
        cls.mode = os.environ.get("FC_VISUAL_MODE", "compare")
        if cls.mode not in ("compare", "record", "update"):
            raise ValueError(f"Unknown FC_VISUAL_MODE: {cls.mode}")
        cls.snapshots = {}

        app = QtWidgets.QApplication
        cls.addClassCleanup(app.setStyle, app.style().objectName())
        app.setStyle("Fusion")
        cls.addClassCleanup(QtCore.QLocale.setDefault, QtCore.QLocale())
        QtCore.QLocale.setDefault(QtCore.QLocale.c())
        cls.addClassCleanup(FreeCAD.Units.setSchema, FreeCAD.Units.getSchema())
        FreeCAD.Units.setSchema(0)
        font_id = QtGui.QFontDatabase.addApplicationFont(
            str(root / "tests/visual/fonts/NotoSans-Regular.ttf")
        )
        families = QtGui.QFontDatabase.applicationFontFamilies(font_id)
        if not families:
            raise AssertionError("Cannot load visual test font")
        cls.addClassCleanup(QtGui.QFontDatabase.removeApplicationFont, font_id)
        cls.addClassCleanup(app.setFont, app.font())
        font = QtGui.QFont(families[0])
        font.setPixelSize(14)
        app.setFont(font)
        cls.addClassCleanup(app.setCursorFlashTime, app.cursorFlashTime())
        app.setCursorFlashTime(0)

    @classmethod
    def tearDownClass(cls):
        report = {
            "label": os.environ.get("FC_VISUAL_LABEL", ""),
            "snapshots": cls.snapshots,
            "success": len(cls.snapshots) == len(_PAGES) * 2
            and all(item["passed"] for item in cls.snapshots.values()),
        }
        (cls.output / "results.json").write_text(json.dumps(report, indent=2) + "\n")

    def test_snapshots(self):
        for theme in ("dark", "light"):
            images = self._render(theme, _PAGES)
            for page in _PAGES:
                with self.subTest(snapshot=f"{page}-{theme}"):
                    self._snapshot(page, theme, images[page])

    def _snapshot(self, page, theme, image):
        name = f"{page}-{theme}"
        result = {"file": f"actual/{name}.png", "passed": False}
        self.snapshots[name] = result
        actual = self.output / result["file"]
        actual.parent.mkdir(parents=True, exist_ok=True)
        self.assertTrue(image.save(str(actual)), f"Cannot save {actual}")
        for folder in ("expected", "diff"):
            (self.output / folder / actual.name).unlink(missing_ok=True)

        if self.mode in ("record", "update"):
            if os.environ.get("FC_VISUAL_UPDATE_BASELINE", "").lower() in ("1", "true"):
                self.baselines.mkdir(parents=True, exist_ok=True)
                (self.baselines / actual.name).write_bytes(actual.read_bytes())
            result["passed"] = True
            return

        baseline = self.baselines / actual.name
        self.assertTrue(baseline.is_file(), f"Missing baseline: {baseline}")
        expected = self.output / "expected" / actual.name
        expected.parent.mkdir(parents=True, exist_ok=True)
        expected.write_bytes(baseline.read_bytes())
        passed, message = compare_images(
            expected, actual, self.output / "diff" / actual.name,
            tolerance=2, ignore_alpha=False, max_mismatched_pixels=0,
        )
        result.update(passed=passed, message=message)
        self.assertTrue(passed, f"{name}: {message}")

    def _render(self, theme, pages):
        documents = []
        samples = []
        try:
            for page in pages:
                doc = FreeCAD.newDocument("ComponentZoo")
                documents.append(doc.Name)
                obj, names = _values(doc, page.replace("property-editors-", "property-"))
                doc.recompute()
                samples.append((page, obj, names))
            return self.fixture.componentZoo(theme, samples)
        finally:
            for name in reversed(documents):
                FreeCAD.closeDocument(name)

    def test_fixture_cleanup(self):
        from PySide.QtWidgets import QApplication

        def preferences():
            root = FreeCAD.ParamGet("User parameter:BaseApp/Preferences")
            window = root.GetGroup("MainWindow").GetContents() or []
            keys = {"Theme", "StyleSheet", "ThemeStyleParametersFile", "TiledBackground"}
            return (
                sorted(entry for entry in window if entry[1] in keys),
                sorted(root.GetGroup("Themes").GetContents() or []),
                sorted(root.GetGroup("View").GetContents() or []),
            )

        previous = preferences()
        app = QApplication.instance()
        documents = set(FreeCAD.listDocuments())
        palette = app.palette()
        stylesheet = app.styleSheet()
        self._render("dark", ("property-states",))
        with self.assertRaisesRegex(ValueError, "Unknown component zoo page"):
            self._render("dark", ("unknown",))

        self.assertEqual(set(FreeCAD.listDocuments()), documents)
        self.assertEqual(app.palette(), palette)
        self.assertEqual(app.styleSheet(), stylesheet)
        self.assertEqual(preferences(), previous)
