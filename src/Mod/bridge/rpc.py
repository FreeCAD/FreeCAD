import FreeCAD
import FreeCADGui

import base64
import contextlib
import hashlib
import hmac
import importlib
import importlib.machinery
import io
import ipaddress
import json
import math
import os
import queue
import re
import secrets
import sys
import tempfile
import threading
import time
import traceback
import uuid
from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Any
from xmlrpc.server import SimpleXMLRPCRequestHandler, SimpleXMLRPCServer  # nosec B411 - local authenticated bridge server.

import discovery

QtCore = None


def set_qtcore(qtcore_module):
    global QtCore
    QtCore = qtcore_module


def _resolve_qtcore():
    global QtCore
    if QtCore is not None:
        return QtCore
    try:
        from PySide6 import QtCore as _qc

        QtCore = _qc
        return QtCore
    except Exception:
        pass
    try:
        from PySide2 import QtCore as _qc

        QtCore = _qc
        return QtCore
    except Exception:
        pass
    try:
        from PySide import QtCore as _qc

        QtCore = _qc
        return QtCore
    except Exception:
        pass
    raise RuntimeError("Cannot import QtCore from PySide6, PySide2, or PySide")


def _resolve_qtgui():
    try:
        from PySide6 import QtGui as _qg

        return _qg
    except Exception:
        pass
    try:
        from PySide2 import QtGui as _qg

        return _qg
    except Exception:
        pass
    try:
        from PySide import QtGui as _qg

        return _qg
    except Exception:
        pass
    raise RuntimeError("Cannot import QtGui from PySide6, PySide2, or PySide")


def _resolve_qtwidgets():
    try:
        from PySide6 import QtWidgets as _qw

        return _qw
    except Exception:
        pass
    try:
        from PySide2 import QtWidgets as _qw

        return _qw
    except Exception:
        pass
    try:
        from PySide import QtGui as _qw

        return _qw
    except Exception:
        pass
    raise RuntimeError("Cannot import QtWidgets from PySide6, PySide2, or PySide")


def _resolve_qtsvg():
    try:
        from PySide6 import QtSvg as _qs

        return _qs
    except Exception:
        pass
    try:
        from PySide2 import QtSvg as _qs

        return _qs
    except Exception:
        pass
    try:
        from PySide import QtSvg as _qs

        return _qs
    except Exception:
        pass
    raise RuntimeError("Cannot import QtSvg from PySide6, PySide2, or PySide")


_approval_state = {"auto_approve_session": False}


_viewport_lock_state: dict[str, Any] = {
    "label": None,
    "host": None,
    "input_filter": None,
    "release_timer": None,
    "active": False,
}


def _find_active_3d_widget():
    try:
        main_window = FreeCADGui.getMainWindow()
    except Exception:
        return None
    if main_window is None:
        return None
    QtWidgets = _resolve_qtwidgets()
    mdi = main_window.findChild(QtWidgets.QMdiArea)
    if mdi is None:
        return None
    sub = mdi.activeSubWindow()
    if sub is None:
        return None
    inner = sub.widget()
    return inner if inner is not None else sub


def _build_status_label():
    QtCore = _resolve_qtcore()
    QtWidgets = _resolve_qtwidgets()
    label = QtWidgets.QLabel("VIEWPORT ENGAGED")
    font = label.font()
    font.setBold(True)
    font.setPointSize(max(font.pointSize(), 9))
    label.setFont(font)
    label.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
    label.setStyleSheet(
        "QLabel { color: #ffffff; background-color: #b03030;"
        " padding: 2px 12px; border-radius: 3px; letter-spacing: 1px; }"
    )
    return label


class _ViewportInputFilter:
    def __init__(self, host):
        QtCore = _resolve_qtcore()
        self._QtCore = QtCore
        self._host = host

        blocked_types = {
            QtCore.QEvent.Type.MouseButtonPress,
            QtCore.QEvent.Type.MouseButtonRelease,
            QtCore.QEvent.Type.MouseButtonDblClick,
            QtCore.QEvent.Type.MouseMove,
            QtCore.QEvent.Type.Wheel,
            QtCore.QEvent.Type.KeyPress,
            QtCore.QEvent.Type.KeyRelease,
            QtCore.QEvent.Type.ContextMenu,
            QtCore.QEvent.Type.TabletPress,
            QtCore.QEvent.Type.TabletRelease,
            QtCore.QEvent.Type.TabletMove,
            QtCore.QEvent.Type.TouchBegin,
            QtCore.QEvent.Type.TouchUpdate,
            QtCore.QEvent.Type.TouchEnd,
            QtCore.QEvent.Type.Gesture,
        }
        host_ref = host
        QtWidgets = _resolve_qtwidgets()

        class _Filter(QtCore.QObject):
            def eventFilter(self, obj, event):
                if event.type() not in blocked_types:
                    return False
                if not isinstance(obj, QtWidgets.QWidget):
                    return False
                if host_ref is None:
                    return False
                if obj is host_ref or host_ref.isAncestorOf(obj):
                    return True
                return False

        self._filter = _Filter()

    def install(self):
        QtWidgets = _resolve_qtwidgets()
        app = QtWidgets.QApplication.instance()
        if app is not None:
            app.installEventFilter(self._filter)

    def uninstall(self):
        QtWidgets = _resolve_qtwidgets()
        app = QtWidgets.QApplication.instance()
        if app is not None:
            try:
                app.removeEventFilter(self._filter)
            except Exception:
                pass


_event_pump_active = False


def _wait_qt(milliseconds: int) -> None:
    if milliseconds <= 0:
        return
    global _event_pump_active
    QtCore = _resolve_qtcore()
    QtWidgets = _resolve_qtwidgets()
    app = QtWidgets.QApplication.instance()
    deadline = time.monotonic() + (milliseconds / 1000.0)
    if app is None or _event_pump_active:
        time.sleep(max(0.0, deadline - time.monotonic()))
        return
    _event_pump_active = True
    try:
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            app.processEvents(
                QtCore.QEventLoop.ProcessEventsFlag.ExcludeSocketNotifiers,
                min(50, int(remaining * 1000)),
            )
    finally:
        _event_pump_active = False


def _engage_viewport_lock(notice_ms: int = 0) -> None:
    state = _viewport_lock_state

    if state["release_timer"] is not None:
        try:
            state["release_timer"].stop()
        except Exception:
            pass
        state["release_timer"] = None

    if state["active"]:
        return

    host = _find_active_3d_widget()
    main_window = None
    try:
        main_window = FreeCADGui.getMainWindow()
    except Exception:
        main_window = None

    label = None
    if main_window is not None:
        try:
            status_bar = main_window.statusBar()
            label = _build_status_label()
            status_bar.addWidget(label, 1)
        except Exception:
            label = None

    input_filter = None
    if host is not None:
        input_filter = _ViewportInputFilter(host)
        input_filter.install()

    state["host"] = host
    state["label"] = label
    state["input_filter"] = input_filter
    state["active"] = True

    FreeCADGui.updateGui()
    _wait_qt(notice_ms)


def _release_viewport_lock(linger_ms: int = 600) -> None:
    state = _viewport_lock_state
    if not state["active"]:
        return
    QtCore = _resolve_qtcore()

    def _do_release():
        if state["input_filter"] is not None:
            state["input_filter"].uninstall()
        label = state["label"]
        if label is not None:
            try:
                main_window = FreeCADGui.getMainWindow()
                if main_window is not None:
                    main_window.statusBar().removeWidget(label)
                label.deleteLater()
            except Exception:
                pass
        state["label"] = None
        state["input_filter"] = None
        state["host"] = None
        state["release_timer"] = None
        state["active"] = False

    timer = QtCore.QTimer()
    timer.setSingleShot(True)
    timer.timeout.connect(_do_release)
    state["release_timer"] = timer
    timer.start(max(linger_ms, 0))


@contextlib.contextmanager
def _viewport_lock(notice_ms: int = 0, linger_ms: int = 250):
    _engage_viewport_lock(notice_ms)
    try:
        yield
    finally:
        _release_viewport_lock(linger_ms)


def _show_code_approval_dialog(
    code: str, tag: str = "", reason: str = "", expected_action: str = ""
) -> str:
    QtCore = _resolve_qtcore()
    QtGui = _resolve_qtgui()
    QtWidgets = _resolve_qtwidgets()

    main_window = FreeCADGui.getMainWindow()
    dialog = QtWidgets.QDialog(main_window)
    dialog.setWindowTitle("Cortex XCS")
    dialog.setWindowFlags(
        QtCore.Qt.WindowType.Dialog | QtCore.Qt.WindowType.WindowStaysOnTopHint
    )

    layout = QtWidgets.QVBoxLayout(dialog)
    layout.setContentsMargins(14, 14, 14, 14)
    layout.setSpacing(10)

    heading = QtWidgets.QLabel("Cortex flagged this code as potentially unsafe")
    heading_font = heading.font()
    heading_font.setBold(True)
    heading.setFont(heading_font)
    layout.addWidget(heading)

    subtitle = QtWidgets.QLabel("Review the code below before allowing it to run.")
    subtitle.setWordWrap(True)
    layout.addWidget(subtitle)

    if tag:
        tag_label = QtWidgets.QLabel(f"Reason: {tag}")
        tag_label.setWordWrap(True)
        tag_label.setTextInteractionFlags(
            QtCore.Qt.TextInteractionFlag.TextSelectableByMouse
        )
        tag_label.setStyleSheet("QLabel { color: #e0a030; font-weight: bold; }")
        layout.addWidget(tag_label)

    if reason or expected_action:
        info_box = QtWidgets.QFrame()
        info_box.setStyleSheet(
            "QFrame { background-color: #252525; border: 1px solid #3c3c3c;"
            " border-radius: 4px; }"
        )
        info_layout = QtWidgets.QVBoxLayout(info_box)
        info_layout.setContentsMargins(10, 8, 10, 8)
        info_layout.setSpacing(6)
        if reason:
            reason_label = QtWidgets.QLabel(f"Why: {reason}")
            reason_label.setWordWrap(True)
            reason_label.setTextInteractionFlags(
                QtCore.Qt.TextInteractionFlag.TextSelectableByMouse
            )
            reason_label.setStyleSheet("QLabel { color: #e6e6e6; }")
            info_layout.addWidget(reason_label)
        if expected_action:
            expected_label = QtWidgets.QLabel(f"Expected: {expected_action}")
            expected_label.setWordWrap(True)
            expected_label.setTextInteractionFlags(
                QtCore.Qt.TextInteractionFlag.TextSelectableByMouse
            )
            expected_label.setStyleSheet("QLabel { color: #e6e6e6; }")
            info_layout.addWidget(expected_label)
        layout.addWidget(info_box)

    code_box = QtWidgets.QPlainTextEdit()
    code_box.setPlainText(code)
    code_box.setReadOnly(True)
    code_box.setLineWrapMode(QtWidgets.QPlainTextEdit.LineWrapMode.NoWrap)
    mono = QtGui.QFontDatabase.systemFont(QtGui.QFontDatabase.SystemFont.FixedFont)
    if mono.pointSize() < 10:
        mono.setPointSize(10)
    code_box.setFont(mono)
    code_box.setFixedHeight(170)
    code_box.setMinimumWidth(480)
    code_box.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarPolicy.ScrollBarAsNeeded)
    code_box.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarPolicy.ScrollBarAsNeeded)
    code_box.setStyleSheet(
        "QPlainTextEdit { background-color: #1e1e1e; color: #e6e6e6;"
        " border: 1px solid #3c3c3c; border-radius: 4px; padding: 6px; }"
    )
    layout.addWidget(code_box)

    button_row = QtWidgets.QHBoxLayout()
    button_row.setSpacing(8)
    auto_button = QtWidgets.QPushButton("Auto-approve for this session")
    button_row.addWidget(auto_button)
    button_row.addStretch(1)
    deny_button = QtWidgets.QPushButton("DENY")
    approve_button = QtWidgets.QPushButton("APPROVE")
    approve_button.setDefault(True)
    button_row.addWidget(deny_button)
    button_row.addWidget(approve_button)
    layout.addLayout(button_row)

    outcome = {"decision": "deny"}

    def choose(decision):
        outcome["decision"] = decision
        dialog.accept()

    approve_button.clicked.connect(lambda: choose("approve"))
    deny_button.clicked.connect(lambda: choose("deny"))
    auto_button.clicked.connect(lambda: choose("auto"))

    dialog.adjustSize()
    dialog.setFixedSize(dialog.size())
    dialog.setSizeGripEnabled(False)
    dialog.setWindowFlag(QtCore.Qt.WindowType.MSWindowsFixedSizeDialogHint, True)
    dialog.setWindowFlag(QtCore.Qt.WindowType.WindowMaximizeButtonHint, False)
    anchor = main_window.mapToGlobal(
        QtCore.QPoint(main_window.width(), main_window.height())
    )
    margin = 24
    x = anchor.x() - dialog.width() - margin
    y = anchor.y() - dialog.height() - margin
    dialog.move(max(x, 0), max(y, 0))

    dialog.exec()
    return outcome["decision"]


def _show_file_save_dialog(
    doc_name: str, default_name: str, default_dir: str, on_done
) -> None:
    QtCore = _resolve_qtcore()
    QtWidgets = _resolve_qtwidgets()

    main_window = FreeCADGui.getMainWindow()
    dialog = QtWidgets.QDialog(main_window)
    dialog.setWindowTitle("Parashell — Save Model")
    dialog.setWindowFlags(
        QtCore.Qt.WindowType.Dialog | QtCore.Qt.WindowType.WindowStaysOnTopHint
    )

    layout = QtWidgets.QVBoxLayout(dialog)
    layout.setContentsMargins(14, 14, 14, 14)
    layout.setSpacing(10)

    heading = QtWidgets.QLabel(f"Choose where to save '{doc_name}'")
    heading_font = heading.font()
    heading_font.setBold(True)
    heading.setFont(heading_font)
    layout.addWidget(heading)

    subtitle = QtWidgets.QLabel(
        "Pick a folder and a file name. The .FCStd extension is fixed."
    )
    subtitle.setWordWrap(True)
    layout.addWidget(subtitle)

    state = {"directory": default_dir if os.path.isdir(default_dir or "") else ""}

    location_row = QtWidgets.QHBoxLayout()
    location_row.setSpacing(8)
    location_label = QtWidgets.QLabel("Location:")
    location_value = QtWidgets.QLineEdit(state["directory"])
    location_value.setReadOnly(True)
    location_value.setPlaceholderText("No folder selected")
    location_value.setMinimumWidth(360)
    select_button = QtWidgets.QPushButton("Select Location")
    location_row.addWidget(location_label)
    location_row.addWidget(location_value, 1)
    location_row.addWidget(select_button)
    layout.addLayout(location_row)

    name_row = QtWidgets.QHBoxLayout()
    name_row.setSpacing(8)
    name_label = QtWidgets.QLabel("File name:")
    name_value = QtWidgets.QLineEdit(default_name)
    ext_label = QtWidgets.QLabel(".FCStd")
    ext_font = ext_label.font()
    ext_font.setBold(True)
    ext_label.setFont(ext_font)
    name_row.addWidget(name_label)
    name_row.addWidget(name_value, 1)
    name_row.addWidget(ext_label)
    layout.addLayout(name_row)

    preview = QtWidgets.QLabel("")
    preview.setWordWrap(True)
    preview.setStyleSheet("QLabel { color: #9fb6c4; }")
    layout.addWidget(preview)

    button_row = QtWidgets.QHBoxLayout()
    button_row.setSpacing(8)
    button_row.addStretch(1)
    cancel_button = QtWidgets.QPushButton("Cancel")
    save_button = QtWidgets.QPushButton("Save")
    save_button.setDefault(True)
    button_row.addWidget(cancel_button)
    button_row.addWidget(save_button)
    layout.addLayout(button_row)

    def _clean_name(raw: str) -> str:
        base = (raw or "").strip()
        if base.lower().endswith(".fcstd"):
            base = base[: -len(".FCStd")]
        base = re.sub(r'[\\/:*?"<>|]', "_", base)
        return base

    def _refresh():
        directory = state["directory"]
        name = _clean_name(name_value.text()) or doc_name
        has_dir = bool(directory) and os.path.isdir(directory)
        save_button.setEnabled(has_dir and bool(name))
        if has_dir:
            preview.setText("Will save to: " + os.path.join(directory, f"{name}.FCStd"))
        else:
            preview.setText("Select a folder to continue.")

    def _choose_location():
        picker = QtWidgets.QFileDialog(dialog, "Select save location")
        picker.setFileMode(QtWidgets.QFileDialog.FileMode.Directory)
        picker.setOption(QtWidgets.QFileDialog.Option.ShowDirsOnly, True)
        picker.setOption(QtWidgets.QFileDialog.Option.DontUseNativeDialog, True)
        picker.setDirectory(state["directory"] or os.path.expanduser("~"))

        def _picked(chosen):
            if chosen:
                state["directory"] = chosen
                location_value.setText(chosen)
                _refresh()

        picker.fileSelected.connect(_picked)
        picker.open()

    outcome = {"cancelled": True, "directory": None, "filename": None}

    def _save():
        directory = state["directory"]
        name = _clean_name(name_value.text()) or doc_name
        if not directory or not os.path.isdir(directory) or not name:
            return
        outcome["cancelled"] = False
        outcome["directory"] = directory
        outcome["filename"] = name
        dialog.accept()

    select_button.clicked.connect(_choose_location)
    name_value.textChanged.connect(lambda _=None: _refresh())
    save_button.clicked.connect(_save)
    cancel_button.clicked.connect(dialog.reject)

    _refresh()

    delivered = {"done": False}

    def _on_finished(_result):
        if delivered["done"]:
            return
        delivered["done"] = True
        on_done(dict(outcome))

    dialog.finished.connect(_on_finished)

    dialog.setMinimumWidth(540)
    dialog.adjustSize()
    dialog.setSizeGripEnabled(False)
    anchor = main_window.mapToGlobal(
        QtCore.QPoint(main_window.width(), main_window.height())
    )
    margin = 24
    x = anchor.x() - dialog.width() - margin
    y = anchor.y() - dialog.height() - margin
    dialog.move(max(x, 0), max(y, 0))

    dialog.setModal(True)
    dialog.show()
    dialog.raise_()
    dialog.activateWindow()


def _get_optional_app_type(name):
    value = getattr(FreeCAD, name, None)
    if isinstance(value, type):
        return value
    if isinstance(value, tuple) and all(isinstance(item, type) for item in value):
        return value
    return None


_COLOR_TYPE = _get_optional_app_type("Color")


def _serialize_value(value):
    if isinstance(value, (int, float, str, bool)):
        return value
    elif isinstance(value, FreeCAD.Vector):
        return {"x": value.x, "y": value.y, "z": value.z}
    elif isinstance(value, FreeCAD.Rotation):
        return {
            "Axis": {"x": value.Axis.x, "y": value.Axis.y, "z": value.Axis.z},
            "Angle": value.Angle,
        }
    elif isinstance(value, FreeCAD.Placement):
        return {
            "Base": _serialize_value(value.Base),
            "Rotation": _serialize_value(value.Rotation),
        }
    elif isinstance(value, (list, tuple)):
        return [_serialize_value(v) for v in value]
    elif _COLOR_TYPE is not None and isinstance(value, _COLOR_TYPE):
        return tuple(value)
    else:
        return str(value)


def _serialize_shape(shape):
    if shape is None:
        return None
    out = {}
    for attr, key in (
        ("Volume", "Volume"),
        ("Area", "Area"),
    ):
        try:
            out[key] = float(getattr(shape, attr))
        except Exception:
            pass
    for attr, key in (
        ("Vertexes", "VertexCount"),
        ("Edges", "EdgeCount"),
        ("Faces", "FaceCount"),
    ):
        try:
            out[key] = len(getattr(shape, attr))
        except Exception:
            pass
    return out


def _serialize_view_object(view):
    if view is None:
        return None
    out = {}
    for attr in (
        "ShapeColor",
        "Transparency",
        "Visibility",
        "DisplayMode",
        "LineColor",
        "PointColor",
        "LineWidth",
        "PointSize",
    ):
        try:
            if hasattr(view, attr):
                out[attr] = _serialize_value(getattr(view, attr))
        except Exception:
            continue
    return out


def serialize_object(obj):
    if isinstance(obj, list):
        return [serialize_object(item) for item in obj]
    elif isinstance(obj, FreeCAD.Document):
        return {
            "Name": obj.Name,
            "Label": obj.Label,
            "FileName": obj.FileName,
            "Objects": [serialize_object(child) for child in obj.Objects],
        }
    else:
        result = {
            "Name": obj.Name,
            "Label": obj.Label,
            "TypeId": obj.TypeId,
            "Properties": {},
            "Placement": _serialize_value(getattr(obj, "Placement", None)),
            "Shape": _serialize_shape(getattr(obj, "Shape", None)),
            "ViewObject": {},
        }
        for prop in obj.PropertiesList:
            try:
                result["Properties"][prop] = _serialize_value(getattr(obj, prop))
            except Exception as e:
                result["Properties"][prop] = f"<error: {str(e)}>"
        if hasattr(obj, "ViewObject") and obj.ViewObject is not None:
            result["ViewObject"] = _serialize_view_object(obj.ViewObject)
        if str(getattr(obj, "TypeId", "")).startswith("Sketcher::SketchObject"):
            try:
                result["Sketch"] = {
                    "Attachment": _serialize_sketch_attachment(obj),
                    "GeometryCount": len(list(getattr(obj, "Geometry", []) or [])),
                    "ConstraintCount": len(list(getattr(obj, "Constraints", []) or [])),
                    "Geometry": [
                        _serialize_sketch_geometry(
                            g,
                            i,
                            bool(_safe_get_construction(obj, i)),
                        )
                        for i, g in enumerate(list(getattr(obj, "Geometry", []) or []))
                    ],
                    "Constraints": [
                        _serialize_sketch_constraint(c, i)
                        for i, c in enumerate(
                            list(getattr(obj, "Constraints", []) or [])
                        )
                    ],
                    "OpenVertices": _serialize_open_vertices(obj),
                }
            except Exception as e:
                result["Sketch"] = {"error": f"<sketch serialize error: {e}>"}
        return result


def _safe_get_construction(sketch, i: int) -> bool:
    try:
        return bool(sketch.getConstruction(i))
    except Exception:
        return False


def _serialize_open_vertices(sketch) -> list[dict[str, Any] | None]:
    try:
        if hasattr(sketch, "OpenVertices"):
            return [_vec_to_dict(v) for v in sketch.OpenVertices]
    except Exception:
        pass
    return []


def _insert_part_from_library(relative_path: str) -> None:
    parts_lib_path = os.path.join(FreeCAD.getUserAppDataDir(), "Mod", "parts_library")
    part_path = os.path.join(parts_lib_path, relative_path)
    if not os.path.exists(part_path):
        raise FileNotFoundError(f"Not found: {part_path}")
    FreeCADGui.ActiveDocument.mergeProject(part_path)


_SKETCH_POS_FROM_NAME = {
    "none": 0,
    "any": 0,
    "curve": 0,
    "edge": 0,
    "start": 1,
    "first": 1,
    "begin": 1,
    "end": 2,
    "last": 2,
    "center": 3,
    "mid": 3,
    "midpoint": 3,
}

_SKETCH_POS_NAME = {0: "none", 1: "start", 2: "end", 3: "center"}


def _to_pos_id(value) -> int:
    if value is None:
        return 0
    if isinstance(value, bool):
        return int(value)
    if isinstance(value, int):
        if value not in (0, 1, 2, 3):
            raise ValueError(f"Invalid sketch position id {value}; expected 0..3")
        return value
    if isinstance(value, str):
        key = value.strip().lower()
        if key in _SKETCH_POS_FROM_NAME:
            return _SKETCH_POS_FROM_NAME[key]
        raise ValueError(f"Invalid sketch position name '{value}'")
    raise TypeError(f"Cannot interpret sketch position from {type(value).__name__}")


def _vec_to_dict(v):
    if v is None:
        return None
    return {"x": float(v.x), "y": float(v.y), "z": float(v.z)}


def _vec_from(value):
    if value is None:
        return None
    if isinstance(value, FreeCAD.Vector):
        return FreeCAD.Vector(value.x, value.y, value.z)
    if isinstance(value, dict):
        return FreeCAD.Vector(
            float(value.get("x", 0.0)),
            float(value.get("y", 0.0)),
            float(value.get("z", 0.0)),
        )
    if isinstance(value, (list, tuple)):
        if len(value) < 2:
            raise ValueError(f"Vector requires at least 2 components, got {len(value)}")
        x = float(value[0])
        y = float(value[1])
        z = float(value[2]) if len(value) >= 3 else 0.0
        return FreeCAD.Vector(x, y, z)
    raise TypeError(f"Cannot interpret vector from {type(value).__name__}")


def _angle_to_rad(value, unit: str | None) -> float:
    import math

    f = float(value)
    if unit is None or unit.lower() in ("deg", "degree", "degrees"):
        return math.radians(f)
    if unit.lower() in ("rad", "radian", "radians"):
        return f
    raise ValueError(f"Unknown angle unit '{unit}'")


def _serialize_sketch_geometry(geom, index: int, is_construction: bool):
    cls_name = type(geom).__name__
    base = {
        "index": int(index),
        "type": cls_name,
        "construction": bool(is_construction),
    }
    try:
        if cls_name in ("GeomLineSegment", "LineSegment"):
            base["start_point"] = _vec_to_dict(geom.StartPoint)
            base["end_point"] = _vec_to_dict(geom.EndPoint)
        elif cls_name in ("GeomCircle", "Circle"):
            base["center"] = _vec_to_dict(geom.Center)
            base["radius"] = float(geom.Radius)
            base["normal"] = _vec_to_dict(getattr(geom, "Axis", None))
        elif cls_name in ("GeomArcOfCircle", "ArcOfCircle"):
            import math

            base["center"] = _vec_to_dict(geom.Center)
            base["radius"] = float(geom.Radius)
            base["normal"] = _vec_to_dict(getattr(geom, "Axis", None))
            first_rad = float(geom.FirstParameter)
            last_rad = float(geom.LastParameter)
            base["first_parameter_rad"] = first_rad
            base["last_parameter_rad"] = last_rad
            base["first_parameter_deg"] = math.degrees(first_rad)
            base["last_parameter_deg"] = math.degrees(last_rad)
            try:
                base["start_point"] = _vec_to_dict(geom.StartPoint)
                base["end_point"] = _vec_to_dict(geom.EndPoint)
            except Exception:
                pass
        elif cls_name in ("GeomPoint", "Point"):
            base["position"] = {
                "x": float(geom.X),
                "y": float(geom.Y),
                "z": float(geom.Z),
            }
        elif cls_name in ("GeomEllipse", "Ellipse"):
            base["center"] = _vec_to_dict(geom.Center)
            base["major_radius"] = float(geom.MajorRadius)
            base["minor_radius"] = float(geom.MinorRadius)
            base["normal"] = _vec_to_dict(getattr(geom, "Axis", None))
            base["major_axis_direction"] = _vec_to_dict(getattr(geom, "XAxis", None))
        elif cls_name in ("GeomArcOfEllipse", "ArcOfEllipse"):
            import math

            base["center"] = _vec_to_dict(geom.Center)
            base["major_radius"] = float(geom.MajorRadius)
            base["minor_radius"] = float(geom.MinorRadius)
            base["normal"] = _vec_to_dict(getattr(geom, "Axis", None))
            base["first_parameter_rad"] = float(geom.FirstParameter)
            base["last_parameter_rad"] = float(geom.LastParameter)
            base["first_parameter_deg"] = math.degrees(float(geom.FirstParameter))
            base["last_parameter_deg"] = math.degrees(float(geom.LastParameter))
        elif cls_name in ("GeomBSplineCurve", "BSplineCurve"):
            try:
                base["degree"] = int(geom.Degree)
                base["periodic"] = bool(geom.isPeriodic())
                base["pole_count"] = int(geom.NbPoles)
                base["poles"] = [_vec_to_dict(p) for p in geom.getPoles()]
            except Exception:
                base["details_unavailable"] = True
        else:
            base["repr"] = str(geom)
    except Exception as e:
        base["error"] = f"<serialize error: {e}>"
    return base


def _serialize_sketch_constraint(constr, index: int):
    out = {
        "index": int(index),
        "type": str(getattr(constr, "Type", "")),
        "name": str(getattr(constr, "Name", "") or ""),
    }
    for attr, key in (
        ("First", "first"),
        ("FirstPos", "first_pos_id"),
        ("Second", "second"),
        ("SecondPos", "second_pos_id"),
        ("Third", "third"),
        ("ThirdPos", "third_pos_id"),
    ):
        if hasattr(constr, attr):
            try:
                v = getattr(constr, attr)
                out[key] = int(v)
                if key.endswith("pos_id"):
                    out[key.replace("_id", "_name")] = _SKETCH_POS_NAME.get(
                        int(v), "unknown"
                    )
            except Exception:
                pass
    if hasattr(constr, "Value"):
        try:
            out["value"] = float(constr.Value)
        except Exception:
            try:
                out["value"] = str(constr.Value)
            except Exception:
                pass
    if hasattr(constr, "IsDriving"):
        try:
            out["driving"] = bool(constr.IsDriving)
        except Exception:
            pass
    if hasattr(constr, "IsActive"):
        try:
            out["active"] = bool(constr.IsActive)
        except Exception:
            pass
    return out


def _serialize_sketch_attachment(sketch):
    info = {"map_mode": None, "support": []}
    try:
        if hasattr(sketch, "MapMode"):
            info["map_mode"] = str(sketch.MapMode)
    except Exception:
        pass

    support_attr = None
    for attr in ("AttachmentSupport", "Support"):
        if hasattr(sketch, attr):
            try:
                support_attr = getattr(sketch, attr)
            except Exception:
                continue
            if support_attr is not None:
                break

    if support_attr is None:
        return info

    entries = []
    try:
        for entry in support_attr:
            if entry is None:
                continue
            if isinstance(entry, tuple) and len(entry) == 2:
                ref_obj, sub = entry
                obj_name = getattr(ref_obj, "Name", str(ref_obj))
                if isinstance(sub, (list, tuple)):
                    for s in sub:
                        entries.append({"object": obj_name, "subelement": str(s)})
                else:
                    entries.append(
                        {"object": obj_name, "subelement": str(sub) if sub else ""}
                    )
            else:
                entries.append({"raw": str(entry)})
    except TypeError:
        entries.append({"raw": str(support_attr)})

    info["support"] = entries
    return info


def _serialize_full_sketch(sketch):
    out = {
        "Name": sketch.Name,
        "Label": sketch.Label,
        "TypeId": sketch.TypeId,
        "Placement": _serialize_value(getattr(sketch, "Placement", None)),
        "Attachment": _serialize_sketch_attachment(sketch),
        "GeometryCount": 0,
        "ConstraintCount": 0,
        "Geometry": [],
        "Constraints": [],
        "OpenVertices": [],
    }

    geometry = list(getattr(sketch, "Geometry", []) or [])
    out["GeometryCount"] = len(geometry)
    for i, g in enumerate(geometry):
        try:
            is_construction = bool(sketch.getConstruction(i))
        except Exception:
            is_construction = False
        out["Geometry"].append(_serialize_sketch_geometry(g, i, is_construction))

    constraints = list(getattr(sketch, "Constraints", []) or [])
    out["ConstraintCount"] = len(constraints)
    for i, c in enumerate(constraints):
        out["Constraints"].append(_serialize_sketch_constraint(c, i))

    try:
        if hasattr(sketch, "OpenVertices"):
            out["OpenVertices"] = [_vec_to_dict(v) for v in sketch.OpenVertices]
    except Exception:
        pass

    out["Shape"] = _serialize_shape(getattr(sketch, "Shape", None))
    return out


def _build_part_geometry(geom_dict: dict):
    import Part

    if not isinstance(geom_dict, dict):
        raise TypeError("geometry entry must be a dict")
    gtype = str(geom_dict.get("type", "")).strip()
    if not gtype:
        raise ValueError("geometry entry missing 'type'")

    key = gtype.lower().replace("geom", "")

    if key in ("linesegment", "line"):
        start = _vec_from(
            geom_dict.get("start")
            or geom_dict.get("start_point")
            or geom_dict.get("StartPoint")
        )
        end = _vec_from(
            geom_dict.get("end")
            or geom_dict.get("end_point")
            or geom_dict.get("EndPoint")
        )
        if start is None or end is None:
            raise ValueError("LineSegment requires 'start' and 'end'")
        return Part.LineSegment(start, end)

    if key == "point":
        pos = _vec_from(
            geom_dict.get("position")
            or geom_dict.get("point")
            or geom_dict.get("Position")
        )
        if pos is None:
            raise ValueError("Point requires 'position'")
        return Part.Point(pos)

    if key == "circle":
        center = _vec_from(geom_dict.get("center") or geom_dict.get("Center"))
        radius = geom_dict.get("radius") or geom_dict.get("Radius")
        if center is None or radius is None:
            raise ValueError("Circle requires 'center' and 'radius'")
        normal = _vec_from(
            geom_dict.get("normal") or geom_dict.get("axis") or geom_dict.get("Axis")
        ) or FreeCAD.Vector(0, 0, 1)
        c = Part.Circle()
        c.Center = center
        c.Axis = normal
        c.Radius = float(radius)
        return c

    if key in ("arcofcircle", "arc"):
        center = _vec_from(geom_dict.get("center") or geom_dict.get("Center"))
        radius = geom_dict.get("radius") or geom_dict.get("Radius")
        if center is None or radius is None:
            raise ValueError("ArcOfCircle requires 'center' and 'radius'")
        normal = _vec_from(
            geom_dict.get("normal") or geom_dict.get("axis") or geom_dict.get("Axis")
        ) or FreeCAD.Vector(0, 0, 1)
        unit = geom_dict.get("angle_unit") or geom_dict.get("angle_units")
        first_in = geom_dict.get(
            "start_angle",
            geom_dict.get("first_parameter", geom_dict.get("FirstParameter")),
        )
        last_in = geom_dict.get(
            "end_angle", geom_dict.get("last_parameter", geom_dict.get("LastParameter"))
        )
        if first_in is None or last_in is None:
            raise ValueError(
                "ArcOfCircle requires 'start_angle' and 'end_angle' (or first/last_parameter)"
            )
        first_rad = _angle_to_rad(first_in, unit)
        last_rad = _angle_to_rad(last_in, unit)
        c = Part.Circle()
        c.Center = center
        c.Axis = normal
        c.Radius = float(radius)
        return Part.ArcOfCircle(c, first_rad, last_rad)

    if key == "ellipse":
        center = _vec_from(geom_dict.get("center") or geom_dict.get("Center"))
        major_radius = geom_dict.get("major_radius") or geom_dict.get("MajorRadius")
        minor_radius = geom_dict.get("minor_radius") or geom_dict.get("MinorRadius")
        if center is None or major_radius is None or minor_radius is None:
            raise ValueError(
                "Ellipse requires 'center', 'major_radius', and 'minor_radius'"
            )
        major_dir = _vec_from(
            geom_dict.get("major_axis_direction") or geom_dict.get("major_axis")
        ) or FreeCAD.Vector(1, 0, 0)
        s1 = FreeCAD.Vector(
            center.x + major_dir.x * float(major_radius),
            center.y + major_dir.y * float(major_radius),
            center.z + major_dir.z * float(major_radius),
        )
        normal = _vec_from(
            geom_dict.get("normal") or geom_dict.get("axis")
        ) or FreeCAD.Vector(0, 0, 1)
        ortho = normal.cross(major_dir)
        ortho_len = ortho.Length
        if ortho_len == 0:
            ortho = FreeCAD.Vector(0, 1, 0)
            ortho_len = 1.0
        ortho.normalize()
        s2 = FreeCAD.Vector(
            center.x + ortho.x * float(minor_radius),
            center.y + ortho.y * float(minor_radius),
            center.z + ortho.z * float(minor_radius),
        )
        return Part.Ellipse(s1, s2, center)

    raise ValueError(f"Unsupported geometry type '{gtype}'")


def _build_sketch_constraint(constr_dict: dict):
    import Sketcher

    if not isinstance(constr_dict, dict):
        raise TypeError("constraint entry must be a dict")
    ctype = str(constr_dict.get("type", "")).strip()
    if not ctype:
        raise ValueError("constraint entry missing 'type'")

    name = constr_dict.get("name", "") or ""

    def _need_int(key):
        if key not in constr_dict or constr_dict[key] is None:
            raise ValueError(f"Constraint '{ctype}' requires '{key}'")
        return int(constr_dict[key])

    def _need_float(key):
        if key not in constr_dict or constr_dict[key] is None:
            raise ValueError(f"Constraint '{ctype}' requires '{key}'")
        return float(constr_dict[key])

    def _opt_pos(key, default=0):
        if key not in constr_dict or constr_dict[key] is None:
            return default
        return _to_pos_id(constr_dict[key])

    norm = ctype.lower().replace(" ", "").replace("_", "")

    if norm == "coincident":
        c = Sketcher.Constraint(
            "Coincident",
            _need_int("first"),
            _opt_pos("first_pos", 1),
            _need_int("second"),
            _opt_pos("second_pos", 1),
        )
    elif norm == "horizontal":
        if "first_pos" in constr_dict or "second" in constr_dict:
            c = Sketcher.Constraint(
                "Horizontal",
                _need_int("first"),
                _opt_pos("first_pos", 1),
                _need_int("second"),
                _opt_pos("second_pos", 1),
            )
        else:
            c = Sketcher.Constraint("Horizontal", _need_int("first"))
    elif norm == "vertical":
        if "first_pos" in constr_dict or "second" in constr_dict:
            c = Sketcher.Constraint(
                "Vertical",
                _need_int("first"),
                _opt_pos("first_pos", 1),
                _need_int("second"),
                _opt_pos("second_pos", 1),
            )
        else:
            c = Sketcher.Constraint("Vertical", _need_int("first"))
    elif norm == "equal":
        c = Sketcher.Constraint("Equal", _need_int("first"), _need_int("second"))
    elif norm == "parallel":
        c = Sketcher.Constraint("Parallel", _need_int("first"), _need_int("second"))
    elif norm == "perpendicular":
        c = Sketcher.Constraint(
            "Perpendicular", _need_int("first"), _need_int("second")
        )
    elif norm == "tangent":
        if "first_pos" in constr_dict or "second_pos" in constr_dict:
            c = Sketcher.Constraint(
                "Tangent",
                _need_int("first"),
                _opt_pos("first_pos", 0),
                _need_int("second"),
                _opt_pos("second_pos", 0),
            )
        else:
            c = Sketcher.Constraint("Tangent", _need_int("first"), _need_int("second"))
    elif norm == "distance":
        if "second" in constr_dict and constr_dict["second"] is not None:
            c = Sketcher.Constraint(
                "Distance",
                _need_int("first"),
                _opt_pos("first_pos", 1),
                int(constr_dict["second"]),
                _opt_pos("second_pos", 1),
                _need_float("value"),
            )
        elif "first_pos" in constr_dict and constr_dict["first_pos"] is not None:
            c = Sketcher.Constraint(
                "Distance",
                _need_int("first"),
                _to_pos_id(constr_dict["first_pos"]),
                _need_float("value"),
            )
        else:
            c = Sketcher.Constraint(
                "Distance", _need_int("first"), _need_float("value")
            )
    elif norm == "distancex":
        if "second" in constr_dict and constr_dict["second"] is not None:
            c = Sketcher.Constraint(
                "DistanceX",
                _need_int("first"),
                _opt_pos("first_pos", 1),
                int(constr_dict["second"]),
                _opt_pos("second_pos", 1),
                _need_float("value"),
            )
        else:
            c = Sketcher.Constraint(
                "DistanceX", _need_int("first"), _need_float("value")
            )
    elif norm == "distancey":
        if "second" in constr_dict and constr_dict["second"] is not None:
            c = Sketcher.Constraint(
                "DistanceY",
                _need_int("first"),
                _opt_pos("first_pos", 1),
                int(constr_dict["second"]),
                _opt_pos("second_pos", 1),
                _need_float("value"),
            )
        else:
            c = Sketcher.Constraint(
                "DistanceY", _need_int("first"), _need_float("value")
            )
    elif norm == "radius":
        c = Sketcher.Constraint("Radius", _need_int("first"), _need_float("value"))
    elif norm == "diameter":
        c = Sketcher.Constraint("Diameter", _need_int("first"), _need_float("value"))
    elif norm == "angle":
        unit = constr_dict.get("angle_unit") or constr_dict.get("angle_units") or "deg"
        value_rad = _angle_to_rad(_need_float("value"), unit)
        if "second" in constr_dict and constr_dict["second"] is not None:
            if "first_pos" in constr_dict or "second_pos" in constr_dict:
                c = Sketcher.Constraint(
                    "Angle",
                    _need_int("first"),
                    _opt_pos("first_pos", 0),
                    int(constr_dict["second"]),
                    _opt_pos("second_pos", 0),
                    value_rad,
                )
            else:
                c = Sketcher.Constraint(
                    "Angle", _need_int("first"), int(constr_dict["second"]), value_rad
                )
        else:
            c = Sketcher.Constraint("Angle", _need_int("first"), value_rad)
    elif norm == "pointonobject":
        c = Sketcher.Constraint(
            "PointOnObject",
            _need_int("first"),
            _opt_pos("first_pos", 1),
            _need_int("second"),
        )
    elif norm == "symmetric":
        if "third_pos" in constr_dict and constr_dict.get("third_pos") is not None:
            c = Sketcher.Constraint(
                "Symmetric",
                _need_int("first"),
                _opt_pos("first_pos", 1),
                _need_int("second"),
                _opt_pos("second_pos", 1),
                _need_int("third"),
                _opt_pos("third_pos", 1),
            )
        else:
            c = Sketcher.Constraint(
                "Symmetric",
                _need_int("first"),
                _opt_pos("first_pos", 1),
                _need_int("second"),
                _opt_pos("second_pos", 1),
                _need_int("third"),
            )
    elif norm == "block":
        c = Sketcher.Constraint("Block", _need_int("first"))
    else:
        raise ValueError(f"Unsupported constraint type '{ctype}'")

    if name:
        try:
            c.Name = str(name)
        except Exception:
            pass
    return c


_FAILURE_STATE_TOKENS = {
    "Invalid",
    "Touched",
    "Error",
    "Restore Error",
    "Recompute Error",
    "Restore_Error",
    "Recompute_Error",
}


def _bbox_to_dict(bbox):
    if bbox is None:
        return None
    try:
        return {
            "x_min": float(bbox.XMin),
            "y_min": float(bbox.YMin),
            "z_min": float(bbox.ZMin),
            "x_max": float(bbox.XMax),
            "y_max": float(bbox.YMax),
            "z_max": float(bbox.ZMax),
            "x_length": float(bbox.XLength),
            "y_length": float(bbox.YLength),
            "z_length": float(bbox.ZLength),
        }
    except Exception:
        return None


def _merge_bbox(
    accumulator: dict[str, float] | None, bbox: dict[str, float]
) -> dict[str, float]:
    if accumulator is None:
        merged = {
            "x_min": bbox["x_min"],
            "y_min": bbox["y_min"],
            "z_min": bbox["z_min"],
            "x_max": bbox["x_max"],
            "y_max": bbox["y_max"],
            "z_max": bbox["z_max"],
        }
    else:
        merged = {
            "x_min": min(accumulator["x_min"], bbox["x_min"]),
            "y_min": min(accumulator["y_min"], bbox["y_min"]),
            "z_min": min(accumulator["z_min"], bbox["z_min"]),
            "x_max": max(accumulator["x_max"], bbox["x_max"]),
            "y_max": max(accumulator["y_max"], bbox["y_max"]),
            "z_max": max(accumulator["z_max"], bbox["z_max"]),
        }
    merged["x_length"] = merged["x_max"] - merged["x_min"]
    merged["y_length"] = merged["y_max"] - merged["y_min"]
    merged["z_length"] = merged["z_max"] - merged["z_min"]
    return merged


def _object_health(obj) -> dict[str, Any]:
    state = list(getattr(obj, "State", []) or [])
    must_execute = False
    try:
        if hasattr(obj, "mustExecute"):
            must_execute = bool(obj.mustExecute())
        elif hasattr(obj, "MustExecute"):
            must_execute = bool(getattr(obj, "MustExecute"))
    except Exception:
        must_execute = False

    issues: list[str] = []
    for token in state:
        if token in _FAILURE_STATE_TOKENS and token not in ("Touched",):
            issues.append(f"state:{token}")
    if must_execute:
        issues.append("must_execute")

    info: dict[str, Any] = {
        "name": obj.Name,
        "label": getattr(obj, "Label", obj.Name),
        "type_id": getattr(obj, "TypeId", ""),
        "state": state,
        "must_execute": must_execute,
    }

    shape = getattr(obj, "Shape", None)
    if shape is not None:
        try:
            is_null = bool(shape.isNull())
        except Exception:
            is_null = None
        try:
            is_valid_shape = bool(shape.isValid())
        except Exception:
            is_valid_shape = None
        try:
            is_closed = bool(shape.isClosed())
        except Exception:
            is_closed = None

        try:
            volume = float(shape.Volume)
        except Exception:
            volume = None
        try:
            area = float(shape.Area)
        except Exception:
            area = None
        try:
            vertex_count = len(shape.Vertexes)
        except Exception:
            vertex_count = None
        try:
            edge_count = len(shape.Edges)
        except Exception:
            edge_count = None
        try:
            face_count = len(shape.Faces)
        except Exception:
            face_count = None
        try:
            solid_count = len(shape.Solids)
        except Exception:
            solid_count = None

        try:
            bbox = _bbox_to_dict(shape.BoundBox)
        except Exception:
            bbox = None

        shape_type = type(shape).__name__
        info["shape"] = {
            "type": shape_type,
            "is_null": is_null,
            "is_valid": is_valid_shape,
            "is_closed": is_closed,
            "volume": volume,
            "area": area,
            "vertex_count": vertex_count,
            "edge_count": edge_count,
            "face_count": face_count,
            "solid_count": solid_count,
            "bbox": bbox,
        }

        if is_null is True:
            issues.append("shape:null")
        if is_valid_shape is False:
            issues.append("shape:invalid")
        if (
            vertex_count == 0
            and edge_count == 0
            and face_count == 0
            and solid_count == 0
        ):
            issues.append("shape:empty")
        if (
            shape_type in ("Solid", "Compound")
            and (volume is not None)
            and volume <= 0.0
        ):
            issues.append("shape:zero_volume")
    else:
        info["shape"] = None

    sketch_info = None
    if str(getattr(obj, "TypeId", "")).startswith("Sketcher::SketchObject"):
        sketch_info = {}
        try:
            sketch_info["geometry_count"] = len(
                list(getattr(obj, "Geometry", []) or [])
            )
        except Exception:
            pass
        try:
            sketch_info["constraint_count"] = len(
                list(getattr(obj, "Constraints", []) or [])
            )
        except Exception:
            pass
        for attr, key in (
            ("FullyConstrained", "fully_constrained"),
            ("OpenVertices", "open_vertex_count"),
        ):
            if hasattr(obj, attr):
                try:
                    val = getattr(obj, attr)
                    if attr == "OpenVertices":
                        sketch_info[key] = len(list(val))
                    else:
                        sketch_info[key] = bool(val)
                except Exception:
                    pass
        if hasattr(obj, "MapMode"):
            try:
                sketch_info["map_mode"] = str(obj.MapMode)
            except Exception:
                pass
        info["sketch"] = sketch_info

    info["issues"] = issues
    info["healthy"] = len(issues) == 0
    return info


def _serialize_matrix(matrix) -> list[list[float]] | None:
    try:
        values = list(matrix.A)
    except Exception:
        return None
    if len(values) < 16:
        return None
    return [
        [float(values[0]), float(values[1]), float(values[2]), float(values[3])],
        [float(values[4]), float(values[5]), float(values[6]), float(values[7])],
        [float(values[8]), float(values[9]), float(values[10]), float(values[11])],
        [float(values[12]), float(values[13]), float(values[14]), float(values[15])],
    ]


def _principal_properties(shape) -> dict[str, Any] | None:
    try:
        properties = shape.PrincipalProperties
    except Exception:
        return None
    if not isinstance(properties, dict):
        return None
    result: dict[str, Any] = {}
    for key, value in properties.items():
        if isinstance(value, FreeCAD.Vector):
            result[key] = _vec_to_dict(value)
        elif isinstance(value, bool):
            result[key] = value
        elif isinstance(value, (int, float)):
            result[key] = float(value)
        elif isinstance(value, (list, tuple)):
            serialized = []
            for item in value:
                if isinstance(item, FreeCAD.Vector):
                    serialized.append(_vec_to_dict(item))
                elif isinstance(item, (int, float)):
                    serialized.append(float(item))
                else:
                    serialized.append(str(item))
            result[key] = serialized
        else:
            result[key] = str(value)
    return result


def _geometry_diagnostics(
    shape, run_bop_check: bool, min_edge_length: float, min_face_area: float
) -> dict[str, Any]:
    diagnostics: dict[str, Any] = {"type": type(shape).__name__}
    issues: list[str] = []

    def _safe_bool(fn):
        try:
            return bool(fn())
        except Exception:
            return None

    is_null = _safe_bool(shape.isNull)
    is_valid = _safe_bool(shape.isValid)
    is_closed = _safe_bool(shape.isClosed)
    diagnostics["is_null"] = is_null
    diagnostics["is_valid"] = is_valid
    diagnostics["is_closed"] = is_closed

    tolerance: dict[str, float] = {}
    for label, mode in (("average", 0), ("max", 1), ("min", -1)):
        try:
            tolerance[label] = float(shape.getTolerance(mode))
        except Exception:
            pass
    if tolerance:
        diagnostics["tolerance"] = tolerance

    counts: dict[str, int | None] = {}
    for key, attr in (
        ("solids", "Solids"),
        ("shells", "Shells"),
        ("faces", "Faces"),
        ("wires", "Wires"),
        ("edges", "Edges"),
        ("vertices", "Vertexes"),
    ):
        try:
            counts[key] = len(getattr(shape, attr))
        except Exception:
            counts[key] = None
    diagnostics["counts"] = counts

    invalid_subshapes: list[str] = []
    for kind, attr in (
        ("Solid", "Solids"),
        ("Shell", "Shells"),
        ("Face", "Faces"),
        ("Wire", "Wires"),
        ("Edge", "Edges"),
        ("Vertex", "Vertexes"),
    ):
        try:
            elements = getattr(shape, attr)
        except Exception:
            continue
        for index, element in enumerate(elements):
            try:
                if not element.isValid():
                    invalid_subshapes.append(f"{kind}{index + 1}")
            except Exception:
                invalid_subshapes.append(f"{kind}{index + 1}")
    diagnostics["invalid_subshapes"] = invalid_subshapes

    short_edges: list[dict[str, Any]] = []
    try:
        for index, edge in enumerate(shape.Edges):
            try:
                length = float(edge.Length)
            except Exception:
                continue
            if length < min_edge_length:
                short_edges.append({"edge": f"Edge{index + 1}", "length": length})
    except Exception:
        pass
    diagnostics["short_edges"] = short_edges

    degenerate_faces: list[dict[str, Any]] = []
    try:
        for index, face in enumerate(shape.Faces):
            try:
                area = float(face.Area)
            except Exception:
                continue
            if area < min_face_area:
                degenerate_faces.append({"face": f"Face{index + 1}", "area": area})
    except Exception:
        pass
    diagnostics["degenerate_faces"] = degenerate_faces

    open_shells: list[str] = []
    try:
        for index, shell in enumerate(shape.Shells):
            try:
                if not shell.isClosed():
                    open_shells.append(f"Shell{index + 1}")
            except Exception:
                pass
    except Exception:
        pass
    diagnostics["open_shells"] = open_shells

    try:
        check_result = shape.check(bool(run_bop_check))
        if check_result is None or check_result is True:
            diagnostics["bop_check"] = "ok"
        elif check_result is False:
            diagnostics["bop_check"] = "invalid"
            issues.append("bop:problems")
        else:
            text = str(check_result).strip()
            lowered = text.lower()
            if not text or "valid" in lowered or "no error" in lowered:
                diagnostics["bop_check"] = "ok"
            else:
                diagnostics["bop_check"] = text
                issues.append("bop:problems")
    except Exception as e:
        diagnostics["bop_check"] = str(e).strip() or "invalid"
        issues.append("bop:problems")

    if is_null is True:
        issues.append("shape:null")
    if is_valid is False:
        issues.append("shape:invalid")
    if invalid_subshapes:
        issues.append("subshapes:invalid")
    if short_edges:
        issues.append("edges:short")
    if degenerate_faces:
        issues.append("faces:degenerate")
    if open_shells:
        issues.append("shells:open")

    diagnostics["issues"] = issues
    diagnostics["valid"] = len(issues) == 0
    return diagnostics


def _bbox_overlap(bb1, bb2, gap: float) -> bool:
    try:
        return not (
            bb1.XMin - gap > bb2.XMax
            or bb2.XMin - gap > bb1.XMax
            or bb1.YMin - gap > bb2.YMax
            or bb2.YMin - gap > bb1.YMax
            or bb1.ZMin - gap > bb2.ZMax
            or bb2.ZMin - gap > bb1.ZMax
        )
    except Exception:
        return True


def _shape_sample_points(shape) -> list:
    points: list = []
    seen: set = set()

    def _add(vec) -> None:
        try:
            key = (round(vec.x, 6), round(vec.y, 6), round(vec.z, 6))
        except Exception:
            return
        if key not in seen:
            seen.add(key)
            points.append(FreeCAD.Vector(vec.x, vec.y, vec.z))

    try:
        for vertex in shape.Vertexes:
            _add(vertex.Point)
    except Exception:
        pass
    try:
        for edge in shape.Edges:
            try:
                _add(edge.CenterOfMass)
            except Exception:
                continue
    except Exception:
        pass
    try:
        for face in shape.Faces:
            try:
                _add(face.CenterOfMass)
            except Exception:
                continue
    except Exception:
        pass
    return points


def _points_inside_shape(shape, points: list, tolerance: float) -> list:
    inside: list = []
    try:
        solids = list(shape.Solids)
    except Exception:
        solids = []
    if not solids:
        return inside
    for vec in points:
        for solid in solids:
            try:
                if solid.isInside(vec, tolerance, False):
                    inside.append(vec)
                    break
            except Exception:
                continue
    return inside


def _solid_interior_metrics(shape, samples: int, tolerance: float) -> dict[str, Any]:
    metrics: dict[str, Any] = {
        "solid_count": 0,
        "shells_per_solid": [],
        "extra_shells": 0,
        "volume": None,
        "area": None,
        "bbox": None,
        "bbox_volume": None,
        "fill_ratio": None,
        "grid_points": 0,
        "inside_points": 0,
        "inside_fraction": None,
        "grid_volume_estimate": None,
        "volume_discrepancy": None,
        "axis_samples": [],
        "center_of_mass": None,
        "center_of_mass_inside": None,
        "error": None,
    }

    try:
        solids = list(shape.Solids)
    except Exception:
        solids = []
    metrics["solid_count"] = len(solids)

    extra_shells = 0
    for index, solid in enumerate(solids):
        try:
            shell_count = len(solid.Shells)
        except Exception:
            shell_count = None
        metrics["shells_per_solid"].append({"solid": index + 1, "shells": shell_count})
        if isinstance(shell_count, int) and shell_count > 1:
            extra_shells += shell_count - 1
    metrics["extra_shells"] = extra_shells

    try:
        metrics["volume"] = float(shape.Volume)
    except Exception:
        metrics["volume"] = None
    try:
        metrics["area"] = float(shape.Area)
    except Exception:
        metrics["area"] = None

    if not solids:
        metrics["error"] = "shape has no solids"
        return metrics

    try:
        bbox = shape.BoundBox
        x_min, y_min, z_min = float(bbox.XMin), float(bbox.YMin), float(bbox.ZMin)
        x_max, y_max, z_max = float(bbox.XMax), float(bbox.YMax), float(bbox.ZMax)
        x_len, y_len, z_len = (
            float(bbox.XLength),
            float(bbox.YLength),
            float(bbox.ZLength),
        )
    except Exception as exc:
        metrics["error"] = f"bounding box unavailable: {exc}"
        return metrics

    metrics["bbox"] = _bbox_to_dict(bbox)
    bbox_volume = x_len * y_len * z_len
    metrics["bbox_volume"] = bbox_volume
    if bbox_volume > 0 and metrics["volume"] is not None:
        metrics["fill_ratio"] = metrics["volume"] / bbox_volume

    def _inside(vec) -> bool:
        for solid in solids:
            try:
                if solid.isInside(vec, tolerance, False):
                    return True
            except Exception:
                continue
        return False

    steps = max(2, min(int(samples), 40))
    dx = x_len / steps
    dy = y_len / steps
    dz = z_len / steps

    inside_count = 0
    total = 0
    for i in range(steps):
        cx = x_min + (i + 0.5) * dx
        for j in range(steps):
            cy = y_min + (j + 0.5) * dy
            for k in range(steps):
                cz = z_min + (k + 0.5) * dz
                total += 1
                if _inside(FreeCAD.Vector(cx, cy, cz)):
                    inside_count += 1

    metrics["grid_points"] = total
    metrics["inside_points"] = inside_count
    if total:
        fraction = inside_count / total
        metrics["inside_fraction"] = fraction
        grid_volume = fraction * bbox_volume
        metrics["grid_volume_estimate"] = grid_volume
        occ_volume = metrics["volume"]
        if occ_volume is not None:
            denom = max(abs(occ_volume), abs(grid_volume), 1e-9)
            metrics["volume_discrepancy"] = abs(occ_volume - grid_volume) / denom

    axis_centers = {
        "x": (None, y_min + y_len / 2.0, z_min + z_len / 2.0),
        "y": (x_min + x_len / 2.0, None, z_min + z_len / 2.0),
        "z": (x_min + x_len / 2.0, y_min + y_len / 2.0, None),
    }
    axis_ranges = {"x": (x_min, x_max), "y": (y_min, y_max), "z": (z_min, z_max)}
    axis_steps = max(3, min(steps, 25))
    for axis, center in axis_centers.items():
        lo, hi = axis_ranges[axis]
        span = hi - lo
        axis_inside = 0
        for i in range(axis_steps):
            t = lo + (i + 0.5) * (span / axis_steps)
            if axis == "x":
                point = FreeCAD.Vector(t, center[1], center[2])
            elif axis == "y":
                point = FreeCAD.Vector(center[0], t, center[2])
            else:
                point = FreeCAD.Vector(center[0], center[1], t)
            if _inside(point):
                axis_inside += 1
        metrics["axis_samples"].append(
            {
                "axis": axis,
                "samples": axis_steps,
                "inside": axis_inside,
                "inside_fraction": axis_inside / axis_steps if axis_steps else None,
            }
        )

    try:
        com = shape.CenterOfMass
        metrics["center_of_mass"] = _vec_to_dict(com)
        metrics["center_of_mass_inside"] = _inside(FreeCAD.Vector(com.x, com.y, com.z))
    except Exception:
        metrics["center_of_mass"] = None
        metrics["center_of_mass_inside"] = None

    return metrics


def _link_targets(value) -> list:
    targets = []
    if isinstance(value, FreeCAD.DocumentObject):
        targets.append(value)
    elif isinstance(value, (list, tuple)):
        for item in value:
            if isinstance(item, FreeCAD.DocumentObject):
                targets.append(item)
            elif (
                isinstance(item, (list, tuple))
                and item
                and isinstance(item[0], FreeCAD.DocumentObject)
            ):
                targets.append(item[0])
    return targets


def _dangling_links(objects) -> list[dict[str, Any]]:
    dangling: list[dict[str, Any]] = []
    for obj in objects:
        try:
            property_list = list(obj.PropertiesList)
        except Exception:
            continue
        for prop in property_list:
            try:
                type_id = obj.getTypeIdOfProperty(prop)
            except Exception:
                continue
            if "Link" not in type_id:
                continue
            try:
                value = getattr(obj, prop)
            except Exception:
                continue
            if value is None:
                continue
            for target in _link_targets(value):
                alive = False
                target_name = None
                try:
                    target_name = target.Name
                    owner = target.Document
                    alive = (
                        owner is not None and owner.getObject(target_name) is not None
                    )
                except Exception:
                    alive = False
                if not alive:
                    dangling.append(
                        {
                            "object": obj.Name,
                            "property": prop,
                            "target": target_name,
                        }
                    )
    return dangling


def _dependency_cycles(objects) -> list[list[str]]:
    graph: dict[str, list[str]] = {}
    for obj in objects:
        deps: list[str] = []
        for dependency in getattr(obj, "OutList", []) or []:
            try:
                deps.append(dependency.Name)
            except Exception:
                pass
        graph[obj.Name] = deps

    white, gray, black = 0, 1, 2
    color = {name: white for name in graph}
    stack: list[str] = []
    cycles: list[list[str]] = []

    def visit(node: str) -> None:
        color[node] = gray
        stack.append(node)
        for nxt in graph.get(node, []):
            if nxt not in color:
                continue
            if color[nxt] == gray and nxt in stack:
                index = stack.index(nxt)
                cycles.append(stack[index:] + [nxt])
            elif color[nxt] == white:
                visit(nxt)
        stack.pop()
        color[node] = black

    for name in graph:
        if color[name] == white:
            visit(name)

    seen: set[frozenset] = set()
    unique: list[list[str]] = []
    for cycle in cycles:
        key = frozenset(cycle)
        if key not in seen:
            seen.add(key)
            unique.append(cycle)
    return unique


def _get_parts_list() -> list[str]:
    parts_lib_path = os.path.join(FreeCAD.getUserAppDataDir(), "Mod", "parts_library")
    if not os.path.exists(parts_lib_path):
        return []
    parts = []
    for root, _, files in os.walk(parts_lib_path):
        for file in files:
            if file.endswith(".FCStd"):
                parts.append(os.path.relpath(os.path.join(root, file), parts_lib_path))
    return parts


class _AuthenticatedRPCRequestHandler(SimpleXMLRPCRequestHandler):
    def do_POST(self):
        expected = getattr(self.server, "_auth_token", None)
        if expected is not None:
            provided = self._bearer_token()
            if provided is None or not hmac.compare_digest(provided, expected):
                self.send_response(401)
                self.send_header("Content-Type", "text/plain")
                self.send_header("Content-Length", "0")
                self.end_headers()
                return
        return super().do_POST()

    def _bearer_token(self):
        header = self.headers.get("Authorization", "")
        prefix = "Bearer "
        if header.startswith(prefix):
            value = header[len(prefix) :].strip()
            return value or None
        return None


class LocalOnlyXMLRPCServer(SimpleXMLRPCServer):
    def __init__(self, addr, auth_token=None, **kwargs):
        self._allowed_network = ipaddress.ip_network("127.0.0.0/8")
        self._auth_token = auth_token
        super().__init__(addr, **kwargs)

    def verify_request(self, request, client_address):
        client_ip = client_address[0]
        try:
            addr = ipaddress.ip_address(client_ip)
            if addr in self._allowed_network:
                return True
        except ValueError:
            pass
        FreeCAD.Console.PrintWarning(
            f"Parashell RPC: Rejected connection from {client_ip}\n"
        )
        return False


_gui_task_runner = None
_gui_tasks_running = False


def wake_gui_task_runner():
    runner = _gui_task_runner
    if runner is None:
        return
    try:
        runner.wake.emit()
    except RuntimeError:
        pass


_call_local = threading.local()


class _WakingQueue(queue.Queue):
    def put(self, item, block=True, timeout=None):
        response_slot = queue.Queue(maxsize=1)
        _call_local.response_slot = response_slot
        super().put((item, response_slot), block=block, timeout=timeout)
        wake_gui_task_runner()


class _ResponseRouter:
    def get(self, block=True, timeout=None):
        response_slot = getattr(_call_local, "response_slot", None)
        if response_slot is None:
            raise RuntimeError("No GUI task is pending for the current thread.")
        try:
            return response_slot.get(block=block, timeout=timeout)
        finally:
            _call_local.response_slot = None


rpc_request_queue = _WakingQueue()
rpc_response_queue = _ResponseRouter()
_rpc_dispatch_lock = threading.Lock()


class _DeferredGuiTask:
    def __init__(self, fn):
        self.fn = fn


def process_gui_tasks():
    global _gui_tasks_running
    if _gui_tasks_running:
        return
    _gui_tasks_running = True
    try:
        while not rpc_request_queue.empty():
            task, response_slot = rpc_request_queue.get()
            if isinstance(task, _DeferredGuiTask):
                try:
                    task.fn(response_slot)
                except Exception as e:
                    response_slot.put(f"GUI task error: {e}")
                continue
            try:
                res = task()
            except Exception as e:
                res = f"GUI task error: {e}"
            response_slot.put(res)
    finally:
        _gui_tasks_running = False


def _ensure_gui_task_runner():
    global _gui_task_runner
    if _gui_task_runner is not None:
        return _gui_task_runner
    QtCore = _resolve_qtcore()

    class _GuiTaskRunner(QtCore.QObject):
        wake = QtCore.Signal()

        def __init__(self):
            super().__init__()
            self.wake.connect(process_gui_tasks)
            self._timer = QtCore.QTimer(self)
            self._timer.setInterval(100)
            self._timer.timeout.connect(process_gui_tasks)
            self._timer.start()

    _gui_task_runner = _GuiTaskRunner()
    return _gui_task_runner


@dataclass
class _Object:
    name: str
    type: str | None = None
    analysis: str | None = None
    properties: dict[str, Any] = field(default_factory=dict)


def _set_object_property(doc, obj, properties: dict[str, Any]):
    for prop, val in properties.items():
        try:
            if prop in obj.PropertiesList:
                if prop == "Placement" and isinstance(val, dict):
                    if "Base" in val:
                        pos = val["Base"]
                    elif "Position" in val:
                        pos = val["Position"]
                    else:
                        pos = {}
                    rot = val.get("Rotation", {})
                    placement = FreeCAD.Placement(
                        FreeCAD.Vector(
                            pos.get("x", 0), pos.get("y", 0), pos.get("z", 0)
                        ),
                        FreeCAD.Rotation(
                            FreeCAD.Vector(
                                rot.get("Axis", {}).get("x", 0),
                                rot.get("Axis", {}).get("y", 0),
                                rot.get("Axis", {}).get("z", 1),
                            ),
                            rot.get("Angle", 0),
                        ),
                    )
                    setattr(obj, prop, placement)
                elif isinstance(getattr(obj, prop), FreeCAD.Vector) and isinstance(
                    val, dict
                ):
                    setattr(
                        obj,
                        prop,
                        FreeCAD.Vector(
                            val.get("x", 0), val.get("y", 0), val.get("z", 0)
                        ),
                    )
                elif prop in ["Base", "Tool", "Source", "Profile"] and isinstance(
                    val, str
                ):
                    ref_obj = doc.getObject(val)
                    if ref_obj:
                        setattr(obj, prop, ref_obj)
                    else:
                        raise ValueError(f"Referenced object '{val}' not found.")
                elif prop == "References" and isinstance(val, list):
                    refs = []
                    for ref_name, face in val:
                        ref_obj = doc.getObject(ref_name)
                        if ref_obj:
                            refs.append((ref_obj, face))
                        else:
                            raise ValueError(
                                f"Referenced object '{ref_name}' not found."
                            )
                    setattr(obj, prop, refs)
                else:
                    setattr(obj, prop, val)
            elif prop == "ShapeColor" and isinstance(val, (list, tuple)):
                setattr(
                    obj.ViewObject,
                    prop,
                    (float(val[0]), float(val[1]), float(val[2]), float(val[3])),
                )
            elif prop == "ViewObject" and isinstance(val, dict):
                for k, v in val.items():
                    if k == "ShapeColor":
                        setattr(
                            obj.ViewObject,
                            k,
                            (float(v[0]), float(v[1]), float(v[2]), float(v[3])),
                        )
                    else:
                        setattr(obj.ViewObject, k, v)
            else:
                setattr(obj, prop, val)
        except Exception as e:
            FreeCAD.Console.PrintError(f"Property '{prop}' assignment error: {e}\n")


_VIEW_PRESETS = {
    "Isometric": lambda v: v.viewIsometric(),
    "Front": lambda v: v.viewFront(),
    "Back": lambda v: v.viewRear(),
    "Top": lambda v: v.viewTop(),
    "Bottom": lambda v: v.viewBottom(),
    "Right": lambda v: v.viewRight(),
    "Left": lambda v: v.viewLeft(),
    "Dimetric": lambda v: v.viewDimetric(),
    "Trimetric": lambda v: v.viewTrimetric(),
}


_CAMERA_MODES = {
    "perspective": "Perspective",
    "orthographic": "Orthographic",
    "ortho": "Orthographic",
}


def _get_camera_type(view) -> str | None:
    try:
        return view.getCameraType()
    except Exception:
        return None


def _set_camera_type(view, camera_type: str) -> bool:
    try:
        view.setCameraType(camera_type)
        return True
    except Exception:
        return False


def _disable_view_animation(view) -> bool | None:
    try:
        was_enabled = bool(view.isAnimationEnabled())
    except Exception:
        return None
    if was_enabled:
        try:
            view.setAnimationEnabled(False)
        except Exception:
            return None
    return was_enabled


def _restore_view_animation(view, previous: bool | None) -> None:
    if previous is None:
        return
    try:
        view.setAnimationEnabled(previous)
    except Exception:
        pass


def _disable_navi_cube(view) -> bool | None:
    try:
        viewer = view.getViewer()
        was_enabled = bool(viewer.isEnabledNaviCube())
    except Exception:
        return None
    if was_enabled:
        try:
            viewer.setEnabledNaviCube(False)
        except Exception:
            return None
    return was_enabled


def _restore_navi_cube(view, previous: bool | None) -> None:
    if not previous:
        return
    try:
        view.getViewer().setEnabledNaviCube(True)
    except Exception:
        pass


def _settle_view(view) -> None:
    global _event_pump_active
    try:
        view.stopAnimating()
    except Exception:
        pass
    QtCore = _resolve_qtcore()
    QtWidgets = _resolve_qtwidgets()
    app = QtWidgets.QApplication.instance()
    if _event_pump_active:
        try:
            view.redraw()
        except Exception:
            pass
        return
    _event_pump_active = True
    try:
        for _ in range(3):
            FreeCADGui.updateGui()
            try:
                view.redraw()
            except Exception:
                pass
            if app is not None:
                app.processEvents(
                    QtCore.QEventLoop.ProcessEventsFlag.ExcludeSocketNotifiers, 30
                )
            else:
                time.sleep(0.03)
        FreeCADGui.updateGui()
    finally:
        _event_pump_active = False


_PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
_MIN_PNG_SIZE = 67


def _read_valid_screenshot(tmp_path: str) -> str | None:
    try:
        if not os.path.exists(tmp_path):
            return None
        if os.path.getsize(tmp_path) < _MIN_PNG_SIZE:
            return None
        with open(tmp_path, "rb") as f:
            data = f.read()
    except OSError:
        return None
    if len(data) < _MIN_PNG_SIZE:
        return None
    if not data.startswith(_PNG_SIGNATURE):
        return None
    if not data.rstrip().endswith(b"IEND\xaeB`\x82"):
        return None
    return base64.b64encode(data).decode("utf-8")


_TECHDRAW_PAGE_TYPE = "TechDraw::DrawPage"


def _find_techdraw_page(doc, page_name: str | None):
    pages = [o for o in doc.Objects if getattr(o, "TypeId", "") == _TECHDRAW_PAGE_TYPE]
    if not pages:
        return None
    if page_name:
        for page in pages:
            if page.Name == page_name or page.Label == page_name:
                return page
        return None
    return pages[0]


def _quantity_to_float(value):
    if value is None:
        return None
    inner = getattr(value, "Value", value)
    try:
        return float(inner)
    except (TypeError, ValueError):
        return None


def _serialize_techdraw_view(view) -> dict[str, Any]:
    info: dict[str, Any] = {
        "Name": getattr(view, "Name", None),
        "Label": getattr(view, "Label", None),
        "TypeId": getattr(view, "TypeId", None),
    }
    scale = _quantity_to_float(getattr(view, "Scale", None))
    if scale is not None:
        info["Scale"] = scale
    direction = getattr(view, "Direction", None)
    if direction is not None:
        try:
            info["Direction"] = {
                "x": float(direction.x),
                "y": float(direction.y),
                "z": float(direction.z),
            }
        except Exception:
            pass
    source = getattr(view, "Source", None)
    if source is None:
        source = getattr(view, "SourceView", None)
    if source is not None:
        try:
            if isinstance(source, (list, tuple)):
                info["Source"] = [getattr(s, "Name", str(s)) for s in source if s]
            else:
                info["Source"] = getattr(source, "Name", str(source))
        except Exception:
            pass
    return info


def _serialize_techdraw_page(page) -> dict[str, Any]:
    info: dict[str, Any] = {
        "Name": page.Name,
        "Label": page.Label,
        "TypeId": page.TypeId,
    }
    scale = _quantity_to_float(getattr(page, "Scale", None))
    if scale is not None:
        info["Scale"] = scale
    template = getattr(page, "Template", None)
    if template is not None:
        tinfo: dict[str, Any] = {
            "Name": getattr(template, "Name", None),
            "Label": getattr(template, "Label", None),
        }
        width = _quantity_to_float(getattr(template, "Width", None))
        height = _quantity_to_float(getattr(template, "Height", None))
        if width is not None:
            tinfo["Width"] = width
        if height is not None:
            tinfo["Height"] = height
        if width and height:
            tinfo["Orientation"] = "Landscape" if width >= height else "Portrait"
        template_file = getattr(template, "Template", None)
        if template_file:
            tinfo["TemplateFile"] = str(template_file)
        info["Template"] = tinfo
    views = []
    for view in getattr(page, "Views", []) or []:
        try:
            views.append(_serialize_techdraw_view(view))
        except Exception as e:
            views.append({"error": f"<view serialize error: {e}>"})
    info["Views"] = views
    info["ViewCount"] = len(views)
    return info


def _list_techdraw_pages_gui(doc_name: str | None) -> dict[str, Any]:
    doc = FreeCAD.getDocument(doc_name) if doc_name else FreeCAD.ActiveDocument
    if doc is None:
        return {
            "success": False,
            "error": "No document is open or the named document was not found.",
        }
    pages = [
        _serialize_techdraw_page(o)
        for o in doc.Objects
        if getattr(o, "TypeId", "") == _TECHDRAW_PAGE_TYPE
    ]
    return {"success": True, "document": doc.Name, "pages": pages}


def _ensure_techdraw_page_open(page) -> None:
    view_object = getattr(page, "ViewObject", None)
    if view_object is None:
        return
    try:
        view_object.doubleClicked()
    except Exception:
        pass
    try:
        FreeCADGui.updateGui()
    except Exception:
        pass


def _export_techdraw_svg(page, svg_path: str):
    try:
        import TechDrawGui
    except Exception as e:
        return f"TechDrawGui module is unavailable: {e}"

    last_error: str | None = None
    for attempt in range(2):
        if attempt == 1:
            _ensure_techdraw_page_open(page)
        try:
            TechDrawGui.exportPageAsSvg(page, svg_path)
        except Exception as e:
            last_error = f"exportPageAsSvg failed: {e}"
            continue
        if os.path.exists(svg_path) and os.path.getsize(svg_path) > 0:
            return True
        last_error = "TechDraw produced an empty SVG export."
    return last_error or "TechDraw produced an empty SVG export."


def _qimage_format_argb32(QtGui):
    fmt_scope = getattr(QtGui.QImage, "Format", None)
    if fmt_scope is not None and hasattr(fmt_scope, "Format_ARGB32"):
        return fmt_scope.Format_ARGB32
    return QtGui.QImage.Format_ARGB32


def _set_paint_quality(painter, QtGui) -> None:
    hint_scope = getattr(QtGui.QPainter, "RenderHint", None)
    for name in ("Antialiasing", "TextAntialiasing", "SmoothPixmapTransform"):
        hint = None
        if hint_scope is not None:
            hint = getattr(hint_scope, name, None)
        if hint is None:
            hint = getattr(QtGui.QPainter, name, None)
        if hint is not None:
            try:
                painter.setRenderHint(hint, True)
            except Exception:
                pass


def _compute_techdraw_target_size(
    base_w: float, base_h: float, width: int | None, height: int | None
) -> tuple[int, int]:
    base_w = max(1.0, float(base_w))
    base_h = max(1.0, float(base_h))
    if width and height:
        return max(1, int(width)), max(1, int(height))
    if width and not height:
        out_w = max(1, int(width))
        return out_w, max(1, int(round(out_w * base_h / base_w)))
    if height and not width:
        out_h = max(1, int(height))
        return max(1, int(round(out_h * base_w / base_h))), out_h
    target_long = 1600.0
    max_long = 4000.0
    long_side = max(base_w, base_h)
    scale = target_long / long_side
    if scale < 1.0:
        scale = 1.0
    if long_side * scale > max_long:
        scale = max_long / long_side
    return max(1, int(round(base_w * scale))), max(1, int(round(base_h * scale)))


def _rasterize_svg_to_png(
    svg_path: str, png_path: str, width: int | None, height: int | None
):
    QtSvg = _resolve_qtsvg()
    QtGui = _resolve_qtgui()

    renderer = QtSvg.QSvgRenderer(svg_path)
    if not renderer.isValid():
        return "The exported SVG could not be parsed by the SVG renderer."

    default_size = renderer.defaultSize()
    base_w = default_size.width()
    base_h = default_size.height()
    if base_w <= 0 or base_h <= 0:
        try:
            view_box = renderer.viewBoxF()
            base_w = int(round(view_box.width()))
            base_h = int(round(view_box.height()))
        except Exception:
            base_w = 0
            base_h = 0
    if base_w <= 0 or base_h <= 0:
        return "The exported page reported no usable dimensions."

    out_w, out_h = _compute_techdraw_target_size(base_w, base_h, width, height)

    image = QtGui.QImage(out_w, out_h, _qimage_format_argb32(QtGui))
    image.fill(QtGui.QColor(255, 255, 255))

    painter = QtGui.QPainter(image)
    try:
        _set_paint_quality(painter, QtGui)
        renderer.render(painter)
    finally:
        painter.end()

    if not image.save(png_path, "PNG"):
        return "Failed to encode the rendered page as PNG."
    return True


_VALID_DISPLAY_MODES = (
    "Shaded",
    "Wireframe",
    "Flat Lines",
    "Hidden line",
    "Points",
    "As is",
    "FlatLines",
    "HiddenLine",
)


def _make_object_uid(doc_name: str, obj_name: str) -> str:
    raw = f"obj|{doc_name}|{obj_name}".encode("utf-8")
    return "obj_" + hashlib.sha1(raw, usedforsecurity=False).hexdigest()[:12]


def _make_sub_uid(doc_name: str, obj_name: str, sub_kind: str, sub_index: int) -> str:
    raw = f"sub|{doc_name}|{obj_name}|{sub_kind}|{sub_index}".encode("utf-8")
    return "sub_" + hashlib.sha1(raw, usedforsecurity=False).hexdigest()[:12]


def _resolve_target(doc, identifier: str):
    if not identifier:
        return None, None
    if identifier.startswith("obj_") or identifier.startswith("sub_"):
        for o in doc.Objects:
            if _make_object_uid(doc.Name, o.Name) == identifier:
                return o, None
            if hasattr(o, "Shape") and o.Shape is not None:
                shape = o.Shape
                try:
                    for i in range(len(shape.Faces)):
                        if _make_sub_uid(doc.Name, o.Name, "Face", i + 1) == identifier:
                            return o, f"Face{i + 1}"
                    for i in range(len(shape.Edges)):
                        if _make_sub_uid(doc.Name, o.Name, "Edge", i + 1) == identifier:
                            return o, f"Edge{i + 1}"
                    for i in range(len(shape.Vertexes)):
                        if (
                            _make_sub_uid(doc.Name, o.Name, "Vertex", i + 1)
                            == identifier
                        ):
                            return o, f"Vertex{i + 1}"
                except Exception:
                    pass
        return None, None

    if "#" in identifier:
        obj_part, sub_part = identifier.split("#", 1)
        ref = doc.getObject(obj_part)
        return ref, sub_part if ref else (None, None)
    ref = doc.getObject(identifier)
    return ref, None


def _capture_view_object_state(view_obj) -> dict[str, Any]:
    state: dict[str, Any] = {}
    for attr in (
        "Visibility",
        "DisplayMode",
        "LineColor",
        "ShapeColor",
        "Transparency",
        "LineWidth",
        "PointSize",
        "BoundingBox",
        "Selectable",
        "DrawStyle",
    ):
        try:
            if hasattr(view_obj, attr):
                state[attr] = getattr(view_obj, attr)
        except Exception:
            pass
    return state


def _restore_view_object_state(view_obj, state: dict[str, Any]) -> None:
    for attr, value in state.items():
        try:
            setattr(view_obj, attr, value)
        except Exception:
            continue


def _apply_display_mode(view_obj, mode: str) -> None:
    if mode is None:
        return
    if not hasattr(view_obj, "DisplayMode"):
        return
    candidates = [mode]
    normalized = mode.replace(" ", "").lower()
    aliases = {
        "wireframe": ["Wireframe"],
        "shaded": ["Shaded"],
        "flatlines": ["Flat Lines", "FlatLines"],
        "hiddenline": ["Hidden line", "HiddenLine"],
        "points": ["Points"],
        "asis": ["As is"],
    }
    candidates.extend(aliases.get(normalized, []))

    available: list[str] = []
    try:
        available = [str(m) for m in view_obj.listDisplayModes()]
    except Exception:
        try:
            available = list(getattr(view_obj, "DisplayMode").getEnumerations())
        except Exception:
            available = []

    for c in candidates:
        if not available or c in available:
            try:
                view_obj.DisplayMode = c
                return
            except Exception:
                continue


def _apply_view_overrides(
    doc,
    display_mode: str | None,
    hide: list[str] | None,
    show_only: list[str] | None,
    highlight: list[str] | None,
    highlight_color: tuple[float, float, float] | None,
) -> dict[str, dict[str, Any]]:
    saved: dict[str, dict[str, Any]] = {}
    if doc is None:
        return saved

    def _save(o):
        if o is None or not hasattr(o, "ViewObject") or o.ViewObject is None:
            return
        if o.Name in saved:
            return
        saved[o.Name] = _capture_view_object_state(o.ViewObject)

    show_only_targets: set[str] = set()
    if show_only:
        for ident in show_only:
            ref, _ = _resolve_target(doc, ident)
            if ref is not None:
                show_only_targets.add(ref.Name)

    if show_only_targets:
        for o in doc.Objects:
            if not hasattr(o, "ViewObject") or o.ViewObject is None:
                continue
            _save(o)
            try:
                o.ViewObject.Visibility = o.Name in show_only_targets
            except Exception:
                pass

    if hide:
        for ident in hide:
            ref, _ = _resolve_target(doc, ident)
            if ref is None or not hasattr(ref, "ViewObject") or ref.ViewObject is None:
                continue
            _save(ref)
            try:
                ref.ViewObject.Visibility = False
            except Exception:
                pass

    if display_mode is not None:
        for o in doc.Objects:
            if not hasattr(o, "ViewObject") or o.ViewObject is None:
                continue
            try:
                if not o.ViewObject.Visibility:
                    continue
            except Exception:
                continue
            _save(o)
            _apply_display_mode(o.ViewObject, display_mode)

    if highlight:
        color = highlight_color or (1.0, 0.4, 0.0)
        try:
            FreeCADGui.Selection.clearSelection()
        except Exception:
            pass
        for ident in highlight:
            ref, sub = _resolve_target(doc, ident)
            if ref is None:
                continue
            if hasattr(ref, "ViewObject") and ref.ViewObject is not None:
                _save(ref)
                try:
                    ref.ViewObject.LineColor = color
                except Exception:
                    pass
                try:
                    ref.ViewObject.LineWidth = 4.0
                except Exception:
                    pass
                try:
                    ref.ViewObject.Visibility = True
                except Exception:
                    pass
            try:
                if sub:
                    FreeCADGui.Selection.addSelection(ref, sub)
                else:
                    FreeCADGui.Selection.addSelection(ref)
            except Exception:
                pass

    return saved


def _restore_view_overrides(doc, saved: dict[str, dict[str, Any]]) -> None:
    if not saved or doc is None:
        return
    for name, state in saved.items():
        ref = doc.getObject(name)
        if ref is None or not hasattr(ref, "ViewObject") or ref.ViewObject is None:
            continue
        _restore_view_object_state(ref.ViewObject, state)
    try:
        FreeCADGui.Selection.clearSelection()
    except Exception:
        pass


_MAX_SNAPSHOT_DEPTH = 256


def _serialize_snapshot_node(
    obj,
    doc_name: str,
    include_subelements: bool,
    seen: set[str] | None = None,
    depth: int = 0,
) -> dict[str, Any]:
    type_id = str(getattr(obj, "TypeId", ""))
    node: dict[str, Any] = {
        "uid": _make_object_uid(doc_name, obj.Name),
        "name": obj.Name,
        "label": getattr(obj, "Label", obj.Name),
        "type_id": type_id,
        "category": _feature_category(type_id),
    }
    view = getattr(obj, "ViewObject", None)
    if view is not None:
        try:
            node["visible"] = bool(getattr(view, "Visibility", True))
        except Exception:
            node["visible"] = True
        try:
            if hasattr(view, "DisplayMode"):
                node["display_mode"] = str(view.DisplayMode)
        except Exception:
            pass
    else:
        node["visible"] = True

    shape = getattr(obj, "Shape", None)
    if shape is not None:
        try:
            node["shape_type"] = type(shape).__name__
        except Exception:
            pass
        sub_counts = {}
        try:
            sub_counts["faces"] = len(shape.Faces)
        except Exception:
            pass
        try:
            sub_counts["edges"] = len(shape.Edges)
        except Exception:
            pass
        try:
            sub_counts["vertices"] = len(shape.Vertexes)
        except Exception:
            pass
        if sub_counts:
            node["sub_counts"] = sub_counts

        if include_subelements:
            faces, edges, vertices = [], [], []
            try:
                for i in range(len(shape.Faces)):
                    f = shape.Faces[i]
                    entry: dict[str, Any] = {
                        "uid": _make_sub_uid(doc_name, obj.Name, "Face", i + 1),
                        "ref": f"{obj.Name}#Face{i + 1}",
                        "index": i + 1,
                    }
                    try:
                        entry["area"] = float(f.Area)
                    except Exception:
                        pass
                    try:
                        cog = f.CenterOfMass
                        entry["center"] = {
                            "x": float(cog.x),
                            "y": float(cog.y),
                            "z": float(cog.z),
                        }
                    except Exception:
                        pass
                    faces.append(entry)
            except Exception:
                pass
            try:
                for i in range(len(shape.Edges)):
                    e = shape.Edges[i]
                    entry = {
                        "uid": _make_sub_uid(doc_name, obj.Name, "Edge", i + 1),
                        "ref": f"{obj.Name}#Edge{i + 1}",
                        "index": i + 1,
                    }
                    try:
                        entry["length"] = float(e.Length)
                    except Exception:
                        pass
                    edges.append(entry)
            except Exception:
                pass
            try:
                for i in range(len(shape.Vertexes)):
                    v = shape.Vertexes[i]
                    entry = {
                        "uid": _make_sub_uid(doc_name, obj.Name, "Vertex", i + 1),
                        "ref": f"{obj.Name}#Vertex{i + 1}",
                        "index": i + 1,
                    }
                    try:
                        p = v.Point
                        entry["position"] = {
                            "x": float(p.x),
                            "y": float(p.y),
                            "z": float(p.z),
                        }
                    except Exception:
                        pass
                    vertices.append(entry)
            except Exception:
                pass
            if faces:
                node["faces"] = faces
            if edges:
                node["edges"] = edges
            if vertices:
                node["vertices"] = vertices

    children = list(getattr(obj, "Group", []) or [])
    if children and depth < _MAX_SNAPSHOT_DEPTH:
        if seen is None:
            seen = set()
        seen.add(obj.Name)
        child_nodes = []
        for c in children:
            child_name = getattr(c, "Name", None)
            if child_name is None or child_name in seen:
                continue
            seen.add(child_name)
            child_nodes.append(
                _serialize_snapshot_node(
                    c, doc_name, include_subelements, seen, depth + 1
                )
            )
        if child_nodes:
            node["children"] = child_nodes
    return node


_FEATURE_LINK_PROPS = (
    "BaseFeature",
    "Profile",
    "Sketch",
    "Sketches",
    "Tool",
    "Source",
    "Base",
    "Objects",
    "References",
    "Spine",
    "Path",
    "Sections",
    "Reversed",
    "ReferenceAxis",
    "Refine",
    "ProfileBased",
    "UpToFace",
    "UpToShape",
    "SupportingFace",
    "Support",
    "AttachmentSupport",
    "Originals",
)


def _link_target_names(value) -> list[str]:
    out: list[str] = []
    if value is None:
        return out
    if isinstance(value, FreeCAD.DocumentObject):
        out.append(value.Name)
        return out
    if isinstance(value, (list, tuple)):
        for entry in value:
            out.extend(_link_target_names(entry))
        return out
    if isinstance(value, dict):
        for v in value.values():
            out.extend(_link_target_names(v))
        return out
    return out


def _collect_feature_links(obj) -> list[dict[str, Any]]:
    links: list[dict[str, Any]] = []
    seen: set[tuple[str, str, str]] = set()

    try:
        for prop in obj.PropertiesList:
            try:
                type_id = obj.getTypeIdOfProperty(prop)
            except Exception:
                type_id = ""
            try:
                value = getattr(obj, prop)
            except Exception:
                continue

            is_link_type = (
                "Link" in type_id
                or "PropertyLinkSub" in type_id
                or prop in _FEATURE_LINK_PROPS
            )
            if not is_link_type and not isinstance(value, FreeCAD.DocumentObject):
                if not (
                    isinstance(value, (list, tuple))
                    and value
                    and any(
                        isinstance(x, FreeCAD.DocumentObject)
                        or (
                            isinstance(x, tuple)
                            and x
                            and isinstance(x[0], FreeCAD.DocumentObject)
                        )
                        for x in value
                    )
                ):
                    continue

            if (
                "PropertyLinkSub" in type_id
                and isinstance(value, tuple)
                and len(value) == 2
            ):
                ref_obj, sub = value
                if isinstance(ref_obj, FreeCAD.DocumentObject):
                    sub_list = (
                        list(sub)
                        if isinstance(sub, (list, tuple))
                        else ([sub] if sub else [])
                    )
                    if not sub_list:
                        sub_list = [""]
                    for s in sub_list:
                        key = (prop, ref_obj.Name, str(s) if s else "")
                        if key in seen:
                            continue
                        seen.add(key)
                        links.append(
                            {
                                "property": prop,
                                "target": ref_obj.Name,
                                "subelement": str(s) if s else "",
                            }
                        )
                continue

            if (
                isinstance(value, list)
                and value
                and all(
                    isinstance(e, tuple)
                    and len(e) == 2
                    and isinstance(e[0], FreeCAD.DocumentObject)
                    for e in value
                )
            ):
                for ref_obj, sub in value:
                    sub_list = (
                        list(sub)
                        if isinstance(sub, (list, tuple))
                        else ([sub] if sub else [])
                    )
                    if not sub_list:
                        sub_list = [""]
                    for s in sub_list:
                        key = (prop, ref_obj.Name, str(s) if s else "")
                        if key in seen:
                            continue
                        seen.add(key)
                        links.append(
                            {
                                "property": prop,
                                "target": ref_obj.Name,
                                "subelement": str(s) if s else "",
                            }
                        )
                continue

            for target_name in _link_target_names(value):
                key = (prop, target_name, "")
                if key in seen:
                    continue
                seen.add(key)
                links.append(
                    {
                        "property": prop,
                        "target": target_name,
                        "subelement": "",
                    }
                )
    except Exception:
        pass

    return links


def _serialize_feature_node(obj) -> dict[str, Any]:
    type_id = str(getattr(obj, "TypeId", ""))
    node: dict[str, Any] = {
        "name": obj.Name,
        "label": getattr(obj, "Label", obj.Name),
        "type_id": type_id,
        "category": _feature_category(type_id),
        "links": _collect_feature_links(obj),
        "state": list(getattr(obj, "State", []) or []),
    }
    try:
        if hasattr(obj, "mustExecute"):
            node["must_execute"] = bool(obj.mustExecute())
        elif hasattr(obj, "MustExecute"):
            node["must_execute"] = bool(getattr(obj, "MustExecute"))
    except Exception:
        node["must_execute"] = False

    shape = getattr(obj, "Shape", None)
    if shape is not None:
        try:
            is_null = bool(shape.isNull())
        except Exception:
            is_null = None
        try:
            is_valid = bool(shape.isValid())
        except Exception:
            is_valid = None
        try:
            volume = float(shape.Volume)
        except Exception:
            volume = None
        node["shape_summary"] = {
            "type": type(shape).__name__,
            "is_null": is_null,
            "is_valid": is_valid,
            "volume": volume,
        }

    if type_id.startswith("Sketcher::SketchObject"):
        try:
            node["sketch_summary"] = {
                "geometry_count": len(list(getattr(obj, "Geometry", []) or [])),
                "constraint_count": len(list(getattr(obj, "Constraints", []) or [])),
                "fully_constrained": (
                    bool(getattr(obj, "FullyConstrained", False))
                    if hasattr(obj, "FullyConstrained")
                    else None
                ),
                "map_mode": (
                    str(getattr(obj, "MapMode")) if hasattr(obj, "MapMode") else None
                ),
            }
        except Exception:
            pass

    return node


def _feature_category(type_id: str) -> str:
    if type_id.startswith("PartDesign::Body"):
        return "Body"
    if type_id.startswith("Sketcher::SketchObject"):
        return "Sketch"
    if type_id.startswith("PartDesign::"):
        return "PartDesignFeature"
    if type_id.startswith("Part::"):
        return "PartFeature"
    if (
        "Plane" in type_id
        or "Line" in type_id
        or "Datum" in type_id
        or "ShapeBinder" in type_id
    ):
        return "Datum"
    return "Other"


def _serialize_partdesign_body(body, tracked: set[str]) -> dict[str, Any]:
    tracked.add(body.Name)

    group = list(getattr(body, "Group", []) or [])
    sequence: list[dict[str, Any]] = []
    feature_section: list[dict[str, Any]] = []

    tip = getattr(body, "Tip", None)
    tip_name = tip.Name if isinstance(tip, FreeCAD.DocumentObject) else None

    sketch_to_consumer: dict[str, list[str]] = {}

    for child in group:
        tracked.add(child.Name)
        node = _serialize_feature_node(child)
        node["is_tip"] = child.Name == tip_name
        type_id = str(getattr(child, "TypeId", ""))

        if type_id.startswith("Sketcher::SketchObject"):
            feature_section.append(node)
        elif type_id.startswith("PartDesign::"):
            for link in node.get("links", []):
                if link.get("property") in ("Profile", "Sketch", "Sketches"):
                    sketch_to_consumer.setdefault(link["target"], []).append(child.Name)
            sequence.append(node)
        else:
            feature_section.append(node)

    for sketch_node in feature_section:
        if sketch_node.get("category") == "Sketch":
            consumers = sketch_to_consumer.get(sketch_node["name"], [])
            sketch_node["consumed_by"] = consumers

    feature_chain: list[dict[str, Any]] = []
    name_to_seq = {n["name"]: n for n in sequence}
    for n in sequence:
        previous: list[str] = []
        for link in n.get("links", []):
            if (
                link.get("property") == "BaseFeature"
                and link.get("target") in name_to_seq
            ):
                previous.append(link["target"])
        n["base_features"] = previous
        feature_chain.append(n)

    return {
        "name": body.Name,
        "label": getattr(body, "Label", body.Name),
        "type_id": str(getattr(body, "TypeId", "")),
        "tip": tip_name,
        "feature_count": len(sequence),
        "sketch_count": sum(
            1 for n in feature_section if n.get("category") == "Sketch"
        ),
        "datum_count": sum(
            1
            for n in feature_section
            if n.get("category") not in ("Sketch", "PartDesignFeature")
        ),
        "feature_chain": feature_chain,
        "sketches_and_datums": feature_section,
    }


def _placement_from_dict(value: dict | None) -> "FreeCAD.Placement | None":
    if value is None:
        return None
    if isinstance(value, FreeCAD.Placement):
        return value
    if not isinstance(value, dict):
        return None
    base_dict = value.get("Base") or value.get("Position") or {}
    base = FreeCAD.Vector(
        float(base_dict.get("x", 0.0)),
        float(base_dict.get("y", 0.0)),
        float(base_dict.get("z", 0.0)),
    )
    rot_dict = value.get("Rotation") or {}
    if "Axis" in rot_dict or "Angle" in rot_dict:
        axis = rot_dict.get("Axis", {"x": 0, "y": 0, "z": 1})
        rotation = FreeCAD.Rotation(
            FreeCAD.Vector(
                float(axis.get("x", 0.0)),
                float(axis.get("y", 0.0)),
                float(axis.get("z", 1.0)),
            ),
            float(rot_dict.get("Angle", 0.0)),
        )
    elif {"yaw", "pitch", "roll"} & set(rot_dict.keys()):
        rotation = FreeCAD.Rotation(
            float(rot_dict.get("yaw", 0.0)),
            float(rot_dict.get("pitch", 0.0)),
            float(rot_dict.get("roll", 0.0)),
        )
    else:
        rotation = FreeCAD.Rotation()
    return FreeCAD.Placement(base, rotation)


def _build_ring_shape(
    outer_radius: float, inner_radius: float, thickness: float, tilt_deg: float
) -> "Part.Shape":
    import Part

    if outer_radius <= 0:
        raise ValueError("outer_radius must be > 0")
    if inner_radius < 0:
        raise ValueError("inner_radius must be >= 0")
    if inner_radius >= outer_radius:
        raise ValueError("inner_radius must be < outer_radius")
    if thickness <= 0:
        raise ValueError("thickness must be > 0")

    outer = Part.makeCylinder(float(outer_radius), float(thickness))
    if inner_radius > 0:
        inner = Part.makeCylinder(float(inner_radius), float(thickness))
        ring = outer.cut(inner)
    else:
        ring = outer

    if tilt_deg:
        ring.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), float(tilt_deg)),
        )
    return ring


def _build_strut_shape(p1, p2, radius: float, end_caps: str = "flat") -> "Part.Shape":
    import Part

    v1 = _vec_from(p1)
    v2 = _vec_from(p2)
    direction = v2 - v1
    length = direction.Length
    if length <= 0:
        raise ValueError("strut endpoints must be distinct")
    if radius <= 0:
        raise ValueError("strut radius must be > 0")

    cylinder = Part.makeCylinder(float(radius), length)

    z_axis = FreeCAD.Vector(0, 0, 1)
    rot = FreeCAD.Rotation(z_axis, direction)
    cylinder.Placement = FreeCAD.Placement(v1, rot)

    if end_caps == "round":
        sphere1 = Part.makeSphere(float(radius))
        sphere1.Placement = FreeCAD.Placement(v1, FreeCAD.Rotation())
        sphere2 = Part.makeSphere(float(radius))
        sphere2.Placement = FreeCAD.Placement(v2, FreeCAD.Rotation())
        return cylinder.fuse([sphere1, sphere2]).removeSplitter()

    return cylinder


def _build_gear_shape(
    module: float,
    tooth_count: int,
    thickness: float,
    pressure_angle_deg: float,
    hub_radius: float,
    bore_radius: float,
    helix_angle_deg: float,
) -> "Part.Shape":
    import math
    import Part

    if module <= 0:
        raise ValueError("module must be > 0")
    if tooth_count < 4:
        raise ValueError("tooth_count must be >= 4")
    if thickness <= 0:
        raise ValueError("thickness must be > 0")
    if hub_radius < 0:
        raise ValueError("hub_radius must be >= 0")
    if bore_radius < 0:
        raise ValueError("bore_radius must be >= 0")

    pitch_radius = module * tooth_count / 2.0
    addendum = module
    dedendum = 1.25 * module
    base_radius = pitch_radius * math.cos(math.radians(pressure_angle_deg))
    outer_radius = pitch_radius + addendum
    root_radius = max(pitch_radius - dedendum, 1e-3)

    if bore_radius >= outer_radius:
        raise ValueError("bore_radius must be < outer_radius")
    if hub_radius >= outer_radius:
        raise ValueError("hub_radius must be < outer_radius")

    def _involute_point(r: float) -> tuple[float, float]:
        if r < base_radius:
            r = base_radius
        alpha = math.acos(base_radius / r)
        inv = math.tan(alpha) - alpha
        return r * math.cos(inv), r * math.sin(inv)

    samples = 12
    half_pitch_angle = math.pi / tooth_count
    pitch_inv = math.tan(math.radians(pressure_angle_deg)) - math.radians(
        pressure_angle_deg
    )
    tooth_thickness_angle = (math.pi / (2.0 * tooth_count)) - 2.0 * pitch_inv

    rise = []
    for i in range(samples + 1):
        r = base_radius + (outer_radius - base_radius) * i / samples
        x, y = _involute_point(r)
        rise.append((x, y))

    angle_offset = half_pitch_angle - tooth_thickness_angle / 2.0

    def _rotate(point, angle):
        c = math.cos(angle)
        s = math.sin(angle)
        return point[0] * c - point[1] * s, point[0] * s + point[1] * c

    flank_a = [_rotate(p, -angle_offset) for p in rise]
    flank_b = [_rotate((p[0], -p[1]), angle_offset) for p in rise]

    tooth_points = []
    if root_radius < base_radius:
        root_a_angle = -half_pitch_angle
        tooth_points.append(
            (root_radius * math.cos(root_a_angle), root_radius * math.sin(root_a_angle))
        )
    tooth_points.extend(flank_a)
    tooth_points.extend(reversed(flank_b))
    if root_radius < base_radius:
        root_b_angle = half_pitch_angle
        tooth_points.append(
            (root_radius * math.cos(root_b_angle), root_radius * math.sin(root_b_angle))
        )

    profile_points: list[FreeCAD.Vector] = []
    for tooth_idx in range(tooth_count):
        rot_angle = 2.0 * math.pi * tooth_idx / tooth_count
        for px, py in tooth_points:
            x, y = _rotate((px, py), rot_angle)
            profile_points.append(FreeCAD.Vector(x, y, 0))

    profile_points.append(profile_points[0])

    edges = []
    for i in range(len(profile_points) - 1):
        edges.append(
            Part.LineSegment(profile_points[i], profile_points[i + 1]).toShape()
        )
    wire = Part.Wire(edges)
    if not wire.isClosed():
        raise RuntimeError("Generated gear profile is not a closed wire")

    face = Part.Face(wire)

    if helix_angle_deg:
        helix_rad = math.radians(helix_angle_deg)
        twist = thickness * math.tan(helix_rad) / pitch_radius
        steps = max(8, int(abs(math.degrees(twist)) // 5))
        sections = []
        for i in range(steps + 1):
            t = i / steps
            ring_face = face.copy()
            ring_face.Placement = FreeCAD.Placement(
                FreeCAD.Vector(0, 0, t * thickness),
                FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), math.degrees(t * twist)),
            )
            sections.append(ring_face.OuterWire)
        loft = Part.makeLoft(sections, True, False, False)
        gear_solid = Part.Solid(loft)
    else:
        gear_solid = face.extrude(FreeCAD.Vector(0, 0, float(thickness)))

    if hub_radius > 0:
        hub = Part.makeCylinder(float(hub_radius), float(thickness))
        gear_solid = gear_solid.fuse(hub).removeSplitter()

    if bore_radius > 0:
        bore = Part.makeCylinder(float(bore_radius), float(thickness) + 2.0)
        bore.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0, 0, -1.0), FreeCAD.Rotation()
        )
        gear_solid = gear_solid.cut(bore)

    return gear_solid


def _build_segment_between_points_shape(p1, p2, params: dict[str, Any]) -> "Part.Shape":
    import Part

    v1 = _vec_from(p1)
    v2 = _vec_from(p2)
    direction = v2 - v1
    length = direction.Length
    if length <= 0:
        raise ValueError("segment endpoints must be distinct")

    cross_section = (params.get("cross_section") or "circular").strip().lower()
    if cross_section in ("circle", "circular", "cylinder", "round"):
        radius = float(params.get("radius"))
        if radius <= 0:
            raise ValueError("segment radius must be > 0")
        body = Part.makeCylinder(radius, length)
        end_caps = str(params.get("end_caps", "flat")).lower()
    elif cross_section in ("box", "rectangular", "square"):
        width = float(params.get("width"))
        height = float(params.get("height", width))
        if width <= 0 or height <= 0:
            raise ValueError("segment width and height must be > 0")
        body = Part.makeBox(width, height, length)
        body.Placement = FreeCAD.Placement(
            FreeCAD.Vector(-width / 2.0, -height / 2.0, 0.0),
            FreeCAD.Rotation(),
        )
        end_caps = "flat"
    else:
        raise ValueError(
            f"Unsupported cross_section '{cross_section}'. Use 'circular' or 'box'."
        )

    z_axis = FreeCAD.Vector(0, 0, 1)
    orient = FreeCAD.Rotation(z_axis, direction)
    body.Placement = FreeCAD.Placement(v1, orient).multiply(body.Placement)

    if (
        cross_section in ("circle", "circular", "cylinder", "round")
        and end_caps == "round"
    ):
        sphere1 = Part.makeSphere(float(params.get("radius")))
        sphere1.Placement = FreeCAD.Placement(v1, FreeCAD.Rotation())
        sphere2 = Part.makeSphere(float(params.get("radius")))
        sphere2.Placement = FreeCAD.Placement(v2, FreeCAD.Rotation())
        return body.fuse([sphere1, sphere2]).removeSplitter()

    return body


def _build_hollow_box_shape(
    length: float,
    width: float,
    height: float,
    thickness: float,
    open_top: bool = False,
    open_bottom: bool = False,
) -> "Part.Shape":
    import Part

    if length <= 0 or width <= 0 or height <= 0:
        raise ValueError("length, width, and height must be > 0")
    if thickness <= 0:
        raise ValueError("thickness must be > 0")
    if thickness * 2.0 >= min(length, width):
        raise ValueError("thickness too large for given length/width")

    outer = Part.makeBox(length, width, height)
    inner_h = height + (2.0 if open_top or open_bottom else 0.0)
    inner_z = -1.0 if open_bottom else thickness
    if open_top and open_bottom:
        inner_h = height + 2.0
        inner_z = -1.0
    elif open_top and not open_bottom:
        inner_h = height - thickness + 1.0
        inner_z = thickness
    elif open_bottom and not open_top:
        inner_h = height - thickness + 1.0
        inner_z = -1.0
    else:
        inner_h = height - 2.0 * thickness
        inner_z = thickness

    if inner_h <= 0:
        raise ValueError("thickness too large for given height")

    inner = Part.makeBox(length - 2.0 * thickness, width - 2.0 * thickness, inner_h)
    inner.Placement = FreeCAD.Placement(
        FreeCAD.Vector(thickness, thickness, inner_z),
        FreeCAD.Rotation(),
    )
    return outer.cut(inner)


def _build_hollow_cylinder_shape(
    outer_radius: float,
    height: float,
    thickness: float,
    open_top: bool = True,
    open_bottom: bool = False,
) -> "Part.Shape":
    import Part

    if outer_radius <= 0 or height <= 0 or thickness <= 0:
        raise ValueError("outer_radius, height, and thickness must be > 0")
    if thickness >= outer_radius:
        raise ValueError("thickness must be < outer_radius")

    outer = Part.makeCylinder(outer_radius, height)
    inner_radius = outer_radius - thickness

    if open_top and open_bottom:
        inner_h = height + 2.0
        inner_z = -1.0
    elif open_top and not open_bottom:
        inner_h = height - thickness + 1.0
        inner_z = thickness
    elif open_bottom and not open_top:
        inner_h = height - thickness + 1.0
        inner_z = -1.0
    else:
        inner_h = height - 2.0 * thickness
        if inner_h <= 0:
            raise ValueError("thickness too large for given height")
        inner_z = thickness

    inner = Part.makeCylinder(inner_radius, inner_h)
    inner.Placement = FreeCAD.Placement(
        FreeCAD.Vector(0, 0, inner_z),
        FreeCAD.Rotation(),
    )
    return outer.cut(inner)


def _build_arch_opening_shape(
    width: float,
    height: float,
    depth: float,
    arch_kind: str = "round",
    arch_height: float | None = None,
) -> "Part.Shape":
    import math
    import Part

    if width <= 0 or height <= 0 or depth <= 0:
        raise ValueError("width, height, and depth must be > 0")

    kind = (arch_kind or "round").strip().lower()
    if kind not in ("round", "pointed", "flat"):
        raise ValueError(
            f"arch_kind must be 'round', 'pointed', or 'flat' (got '{arch_kind}')"
        )

    if kind == "round":
        arch_h = width / 2.0
        body_h = height - arch_h
        if body_h <= 0:
            raise ValueError("height must be > width/2 for round arches")
        body = Part.makeBox(width, depth, body_h)
        body.Placement = FreeCAD.Placement(
            FreeCAD.Vector(-width / 2.0, -depth / 2.0, 0.0),
            FreeCAD.Rotation(),
        )
        cyl = Part.makeCylinder(arch_h, depth)
        cyl.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0.0, depth / 2.0, body_h),
            FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90.0),
        )
        return body.fuse(cyl).removeSplitter()

    if kind == "pointed":
        arch_h = float(arch_height) if arch_height is not None else width
        body_h = height - arch_h
        if body_h <= 0:
            raise ValueError("height must be > arch_height for pointed arches")
        body = Part.makeBox(width, depth, body_h)
        body.Placement = FreeCAD.Placement(
            FreeCAD.Vector(-width / 2.0, -depth / 2.0, 0.0),
            FreeCAD.Rotation(),
        )
        bottom_left = FreeCAD.Vector(-width / 2.0, 0.0, body_h)
        bottom_right = FreeCAD.Vector(width / 2.0, 0.0, body_h)
        apex = FreeCAD.Vector(0.0, 0.0, body_h + arch_h)
        e1 = Part.LineSegment(bottom_left, apex).toShape()
        e2 = Part.LineSegment(apex, bottom_right).toShape()
        e3 = Part.LineSegment(bottom_right, bottom_left).toShape()
        wire = Part.Wire([e1, e2, e3])
        face = Part.Face(wire)
        triangle = face.extrude(FreeCAD.Vector(0, depth, 0))
        triangle.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0, -depth / 2.0, 0),
            FreeCAD.Rotation(),
        )
        return body.fuse(triangle).removeSplitter()

    arch_h = float(arch_height) if arch_height is not None else width / 4.0
    body_h = height - arch_h
    if body_h <= 0:
        raise ValueError("height must be > arch_height for flat arches")
    body = Part.makeBox(width, depth, body_h)
    body.Placement = FreeCAD.Placement(
        FreeCAD.Vector(-width / 2.0, -depth / 2.0, 0.0),
        FreeCAD.Rotation(),
    )
    radius_half = (width * width / 4.0 + arch_h * arch_h) / (2.0 * arch_h)
    radius = radius_half
    center_z = body_h + arch_h - radius
    cyl = Part.makeCylinder(radius, depth)
    cyl.Placement = FreeCAD.Placement(
        FreeCAD.Vector(0.0, depth / 2.0, center_z),
        FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90.0),
    )
    half_angle = math.degrees(math.asin((width / 2.0) / radius))
    cap_box_bottom = center_z - radius - 1.0
    cap_box_height = body_h - cap_box_bottom
    cap_box = Part.makeBox(width + 2.0, depth + 2.0, cap_box_height)
    cap_box.Placement = FreeCAD.Placement(
        FreeCAD.Vector(-(width + 2.0) / 2.0, -(depth + 2.0) / 2.0, cap_box_bottom),
        FreeCAD.Rotation(),
    )
    cap = cyl.cut(cap_box)
    _ = half_angle
    return body.fuse(cap).removeSplitter()


def _build_crenellated_wall_shape(
    length: float,
    thickness: float,
    height: float,
    merlon_width: float,
    gap_width: float,
    merlon_height: float,
    start_with_merlon: bool = True,
) -> "Part.Shape":
    import Part

    if length <= 0 or thickness <= 0 or height <= 0:
        raise ValueError("length, thickness, and height must be > 0")
    if merlon_width <= 0 or gap_width < 0:
        raise ValueError("merlon_width must be > 0 and gap_width >= 0")
    if merlon_height <= 0:
        raise ValueError("merlon_height must be > 0")

    base_height = max(height - merlon_height, 0.0)
    base = Part.makeBox(length, thickness, base_height) if base_height > 0 else None

    pieces: list = []
    if base is not None:
        pieces.append(base)

    period = merlon_width + gap_width
    if period <= 0:
        raise ValueError("merlon_width + gap_width must be > 0")

    if start_with_merlon:
        x = 0.0
    else:
        x = gap_width

    while x + 1e-6 < length:
        w = min(merlon_width, length - x)
        if w <= 1e-6:
            break
        merlon = Part.makeBox(w, thickness, merlon_height)
        merlon.Placement = FreeCAD.Placement(
            FreeCAD.Vector(x, 0.0, base_height),
            FreeCAD.Rotation(),
        )
        pieces.append(merlon)
        x += period

    if not pieces:
        raise ValueError("crenellated wall produced no geometry")
    if len(pieces) == 1:
        return pieces[0]
    return pieces[0].fuse(pieces[1:]).removeSplitter()


def _build_pyramid_shape(
    base_points: list,
    apex,
) -> "Part.Shape":
    import Part

    if not base_points or len(base_points) < 3:
        raise ValueError("pyramid base must have >= 3 points")

    apex_v = _vec_from(apex)
    base_vecs = [_vec_from(p) for p in base_points]

    edges = []
    for i in range(len(base_vecs)):
        a = base_vecs[i]
        b = base_vecs[(i + 1) % len(base_vecs)]
        edges.append(Part.LineSegment(a, b).toShape())
    base_wire = Part.Wire(edges)
    if not base_wire.isClosed():
        raise ValueError("pyramid base wire is not closed")
    base_face = Part.Face(base_wire)

    side_faces = [base_face]
    for i in range(len(base_vecs)):
        a = base_vecs[i]
        b = base_vecs[(i + 1) % len(base_vecs)]
        e1 = Part.LineSegment(a, b).toShape()
        e2 = Part.LineSegment(b, apex_v).toShape()
        e3 = Part.LineSegment(apex_v, a).toShape()
        wire = Part.Wire([e1, e2, e3])
        side_faces.append(Part.Face(wire))

    shell = Part.Shell(side_faces)
    return Part.Solid(shell)


def _build_regular_pyramid_shape(
    base_radius: float,
    base_sides: int,
    height: float,
    base_rotation_deg: float = 0.0,
) -> "Part.Shape":
    import math

    if base_radius <= 0 or height <= 0:
        raise ValueError("base_radius and height must be > 0")
    if base_sides < 3:
        raise ValueError("base_sides must be >= 3")
    base_pts = []
    for i in range(base_sides):
        angle = math.radians(base_rotation_deg + 360.0 * i / base_sides)
        base_pts.append(
            [base_radius * math.cos(angle), base_radius * math.sin(angle), 0.0]
        )
    apex = [0.0, 0.0, float(height)]
    return _build_pyramid_shape(base_pts, apex)


def _build_sketch_extrude_shape(
    profile: list[dict[str, Any]], depth: float, direction=None
) -> "Part.Shape":
    import Part

    if not profile:
        raise ValueError("profile must contain at least one geometry entry")
    if depth == 0:
        raise ValueError("depth must be != 0")

    edges = []
    for entry in profile:
        part_geom = _build_part_geometry(entry)
        edges.append(part_geom.toShape())
    wire = Part.Wire(edges)
    if not wire.isClosed():
        raise ValueError(
            "sketch profile is not closed — last edge does not return to the start point"
        )
    face = Part.Face(wire)

    if direction is not None:
        dir_vec = _vec_from(direction)
        if dir_vec.Length == 0:
            raise ValueError("extrude direction cannot be zero")
        dir_vec.normalize()
        return face.extrude(
            FreeCAD.Vector(dir_vec.x * depth, dir_vec.y * depth, dir_vec.z * depth)
        )
    return face.extrude(FreeCAD.Vector(0.0, 0.0, float(depth)))


def _compute_pattern_placements(
    kind: str, count: int, params: dict[str, Any]
) -> list["FreeCAD.Placement"]:
    import math

    placements: list[FreeCAD.Placement] = []
    if count <= 0:
        return placements

    kind_norm = (kind or "").strip().lower()

    if kind_norm in ("circular", "polar"):
        center = _vec_from(params.get("center") or [0, 0, 0])
        axis = _vec_from(params.get("axis") or [0, 0, 1])
        if axis.Length == 0:
            raise ValueError("axis vector cannot be zero")
        axis.normalize()
        total_angle = float(params.get("total_angle_deg", 360.0))
        full_circle = bool(params.get("full_circle", abs(total_angle) >= 360.0 - 1e-6))
        include_first = bool(params.get("include_first", True))

        if full_circle:
            step = 360.0 / count
            indices = range(count) if include_first else range(1, count + 1)
        elif include_first:
            step = total_angle / max(count - 1, 1)
            indices = range(count)
        else:
            step = total_angle / count
            indices = range(1, count + 1)

        for i in indices:
            angle = step * i
            rotation = FreeCAD.Rotation(axis, angle)
            translated_center = FreeCAD.Vector(center.x, center.y, center.z)
            placement = FreeCAD.Placement(
                FreeCAD.Vector(0.0, 0.0, 0.0), rotation, translated_center
            )
            placements.append(placement)
        return placements

    if kind_norm == "linear":
        direction = _vec_from(params.get("direction") or [1, 0, 0])
        if direction.Length == 0:
            raise ValueError("direction vector cannot be zero")
        spacing = float(params.get("spacing", 0.0))
        if spacing == 0.0:
            length = float(params.get("length", 0.0))
            spacing = length / max(count - 1, 1) if length else direction.Length
        direction.normalize()
        for i in range(count):
            offset = direction * (spacing * i)
            placements.append(FreeCAD.Placement(offset, FreeCAD.Rotation()))
        return placements

    if kind_norm == "grid":
        cols = int(params.get("cols", count))
        rows = int(params.get("rows", 1))
        spacing_x = float(params.get("spacing_x", 0.0))
        spacing_y = float(params.get("spacing_y", 0.0))
        u_dir = _vec_from(params.get("u_axis") or [1, 0, 0])
        v_dir = _vec_from(params.get("v_axis") or [0, 1, 0])
        if u_dir.Length == 0 or v_dir.Length == 0:
            raise ValueError("u_axis and v_axis must be non-zero")
        u_dir.normalize()
        v_dir.normalize()
        for r in range(rows):
            for c in range(cols):
                offset = u_dir * (spacing_x * c) + v_dir * (spacing_y * r)
                placements.append(FreeCAD.Placement(offset, FreeCAD.Rotation()))
        return placements

    raise ValueError(
        f"Unknown pattern kind '{kind}'. Use 'circular', 'linear', or 'grid'."
    )


def _bbox_dict_full(bbox) -> dict[str, Any]:
    info = _bbox_to_dict(bbox) or {}
    if not info:
        return info
    info["center"] = {
        "x": (info["x_min"] + info["x_max"]) / 2.0,
        "y": (info["y_min"] + info["y_max"]) / 2.0,
        "z": (info["z_min"] + info["z_max"]) / 2.0,
    }
    info["diagonal"] = float(
        (
            (info["x_max"] - info["x_min"]) ** 2
            + (info["y_max"] - info["y_min"]) ** 2
            + (info["z_max"] - info["z_min"]) ** 2
        )
        ** 0.5
    )
    return info


def _resolve_subshape(obj, sub: str | None):
    if obj is None:
        return None
    shape = getattr(obj, "Shape", None)
    if shape is None:
        return None
    if not sub:
        return shape
    sub_lower = sub.lower()
    try:
        if sub_lower.startswith("face"):
            idx = int(sub[4:])
            return shape.Faces[idx - 1]
        if sub_lower.startswith("edge"):
            idx = int(sub[4:])
            return shape.Edges[idx - 1]
        if sub_lower.startswith("vertex"):
            idx = int(sub[6:])
            return shape.Vertexes[idx - 1]
    except (ValueError, IndexError):
        return None
    return shape


def _point_for_distance(
    doc, identifier: str | dict | list | tuple
) -> tuple[FreeCAD.Vector | None, str]:
    if isinstance(identifier, (list, tuple, dict)) and not isinstance(identifier, str):
        try:
            return _vec_from(identifier), f"point{tuple(_vec_from(identifier))}"
        except Exception:
            return None, str(identifier)

    text = str(identifier)
    ref, sub = _resolve_target(doc, text)
    if ref is None:
        return None, text

    sub_shape = _resolve_subshape(ref, sub)
    if sub_shape is None:
        return None, text

    try:
        if hasattr(sub_shape, "Point"):
            p = sub_shape.Point
            return FreeCAD.Vector(p.x, p.y, p.z), text
    except Exception:
        pass
    try:
        cog = sub_shape.CenterOfMass
        return FreeCAD.Vector(cog.x, cog.y, cog.z), text
    except Exception:
        pass
    try:
        bb = sub_shape.BoundBox
        return FreeCAD.Vector(bb.Center.x, bb.Center.y, bb.Center.z), text
    except Exception:
        return None, text


_SPREADSHEET_TYPE_ID = "Spreadsheet::Sheet"
_CELL_RE = re.compile(r"^([A-Za-z]+)([1-9][0-9]*)$")
_RANGE_RE = re.compile(r"^([A-Za-z]+[1-9][0-9]*):([A-Za-z]+[1-9][0-9]*)$")
_STYLE_OPTIONS = ("replace", "add", "remove")
_STRUCTURE_OPERATIONS = (
    "insert_rows",
    "remove_rows",
    "insert_columns",
    "remove_columns",
)


def _column_to_index(column: str) -> int:
    value = 0
    for char in column.upper():
        value = value * 26 + (ord(char) - ord("A") + 1)
    return value - 1


def _index_to_column(index: int) -> str:
    result = ""
    current = index + 1
    while current > 0:
        current, remainder = divmod(current - 1, 26)
        result = chr(ord("A") + remainder) + result
    return result


def _split_cell(address: str) -> tuple[str, int]:
    match = _CELL_RE.match(str(address).strip())
    if not match:
        raise ValueError(
            f"Invalid cell address '{address}'. Expected a form like 'B3'."
        )
    return match.group(1).upper(), int(match.group(2))


def _normalize_cell(address: str) -> str:
    column, row = _split_cell(address)
    return f"{column}{row}"


def _expand_cell_range(span: str) -> list[str]:
    text = str(span).strip()
    range_match = _RANGE_RE.match(text)
    if not range_match:
        return [_normalize_cell(text)]
    start_col, start_row = _split_cell(range_match.group(1))
    end_col, end_row = _split_cell(range_match.group(2))
    col_start = _column_to_index(start_col)
    col_end = _column_to_index(end_col)
    if col_start > col_end:
        col_start, col_end = col_end, col_start
    if start_row > end_row:
        start_row, end_row = end_row, start_row
    cells: list[str] = []
    for row in range(start_row, end_row + 1):
        for col in range(col_start, col_end + 1):
            cells.append(f"{_index_to_column(col)}{row}")
    return cells


def _color_to_list(color) -> list[float] | None:
    if color is None:
        return None
    try:
        if hasattr(color, "r"):
            values = [color.r, color.g, color.b, color.a]
        else:
            values = list(color)
    except TypeError:
        return None
    if not values:
        return None
    return [round(float(v), 6) for v in values]


def _list_to_color(value) -> tuple:
    if not isinstance(value, (list, tuple)) or len(value) not in (3, 4):
        raise ValueError("Color must be a list of 3 or 4 floats in the range 0..1.")
    channels = [float(v) for v in value]
    if any(c < 0.0 or c > 1.0 for c in channels):
        raise ValueError("Color channels must be between 0.0 and 1.0.")
    if len(channels) == 3:
        channels.append(1.0)
    return tuple(channels)


def _safe_sheet_call(func, *args):
    try:
        return func(*args)
    except Exception:
        return None


def _resolve_spreadsheet(doc, sheet_name):
    sheet = doc.getObject(sheet_name)
    if sheet is None:
        return None, f"Object '{sheet_name}' not found in document '{doc.Name}'."
    if str(getattr(sheet, "TypeId", "")) != _SPREADSHEET_TYPE_ID:
        return None, (
            f"Object '{sheet_name}' is not a {_SPREADSHEET_TYPE_ID} "
            f"(TypeId={getattr(sheet, 'TypeId', 'unknown')})."
        )
    return sheet, None


def _serialize_spreadsheet_cell(sheet, address: str) -> dict[str, Any]:
    entry: dict[str, Any] = {"address": address}
    content = _safe_sheet_call(sheet.getContents, address)
    entry["content"] = content if content is not None else ""
    value = _safe_sheet_call(sheet.get, address)
    if value is None or isinstance(value, (bool, int, float, str)):
        entry["value"] = value
    else:
        entry["value"] = str(value)
    alias = _safe_sheet_call(sheet.getAlias, address)
    if alias:
        entry["alias"] = alias
    display_unit = _safe_sheet_call(sheet.getDisplayUnit, address)
    if display_unit:
        entry["display_unit"] = str(display_unit)
    style = _safe_sheet_call(sheet.getStyle, address)
    if style:
        entry["style"] = sorted(str(s) for s in style)
    alignment = _safe_sheet_call(sheet.getAlignment, address)
    if alignment:
        entry["alignment"] = (
            alignment if isinstance(alignment, str) else [str(a) for a in alignment]
        )
    foreground = _color_to_list(_safe_sheet_call(sheet.getForeground, address))
    if foreground is not None:
        entry["foreground"] = foreground
    background = _color_to_list(_safe_sheet_call(sheet.getBackground, address))
    if background is not None:
        entry["background"] = background
    return entry


_transactions: dict[str, dict[str, Any]] = {}
_active_txn_by_doc: dict[str, str] = {}
_txn_lock = threading.Lock()


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def _uuid7() -> str:
    unix_ms = time.time_ns() // 1_000_000
    rand = os.urandom(10)
    b = bytearray(16)
    b[0] = (unix_ms >> 40) & 0xFF
    b[1] = (unix_ms >> 32) & 0xFF
    b[2] = (unix_ms >> 24) & 0xFF
    b[3] = (unix_ms >> 16) & 0xFF
    b[4] = (unix_ms >> 8) & 0xFF
    b[5] = unix_ms & 0xFF
    b[6] = 0x70 | (rand[0] & 0x0F)
    b[7] = rand[1]
    b[8] = 0x80 | (rand[2] & 0x3F)
    b[9] = rand[3]
    b[10:16] = rand[4:10]
    return str(uuid.UUID(bytes=bytes(b)))


def _object_fingerprint(obj) -> str:
    parts = []
    for prop in sorted(getattr(obj, "PropertiesList", []) or []):
        try:
            parts.append(f"{prop}={getattr(obj, prop)!r}")
        except Exception:
            parts.append(f"{prop}=<unreadable>")
    raw = "|".join(parts).encode("utf-8", "replace")
    return hashlib.sha1(raw, usedforsecurity=False).hexdigest()


def _snapshot_doc(doc) -> dict[str, dict[str, str]]:
    snap: dict[str, dict[str, str]] = {}
    for o in doc.Objects:
        snap[o.Name] = {
            "label": str(getattr(o, "Label", o.Name)),
            "type": str(getattr(o, "TypeId", "")),
            "fingerprint": _object_fingerprint(o),
        }
    return snap


def _diff_snapshots(
    before: dict[str, dict[str, str]], after: dict[str, dict[str, str]]
) -> dict[str, list[dict[str, str]]]:
    add: list[dict[str, str]] = []
    change: list[dict[str, str]] = []
    destroy: list[dict[str, str]] = []
    for name, info in after.items():
        prior = before.get(name)
        if prior is None:
            add.append({"name": name, "label": info["label"], "type": info["type"]})
        elif prior["fingerprint"] != info["fingerprint"]:
            change.append({"name": name, "label": info["label"], "type": info["type"]})
    for name, info in before.items():
        if name not in after:
            destroy.append({"name": name, "label": info["label"], "type": info["type"]})
    return {"add": add, "change": change, "destroy": destroy}


def _txn_public(record: dict[str, Any]) -> dict[str, Any]:
    return {k: v for k, v in record.items() if not k.startswith("_")}


def _txn_summary_counts(diff: dict[str, list[dict[str, str]]]) -> str:
    return (
        f"{len(diff['add'])} to add, "
        f"{len(diff['change'])} to change, "
        f"{len(diff['destroy'])} to destroy"
    )


def _resolve_open_txn(
    transaction_id: str, doc_name: str | None = None
) -> dict[str, Any]:
    record = _transactions.get(transaction_id)
    if record is None:
        raise ValueError(f"Transaction '{transaction_id}' not found.")
    if record["status"] != "open":
        raise ValueError(
            f"Transaction '{transaction_id}' is '{record['status']}', not open; "
            "edits can only be staged on an open transaction."
        )
    if doc_name is not None and record["document"] != doc_name:
        raise ValueError(
            f"Transaction '{transaction_id}' belongs to document "
            f"'{record['document']}', not '{doc_name}'."
        )
    return record


def _record_txn_operation(
    transaction_id: str, doc_name: str, action: str, details: dict[str, Any]
) -> None:
    with _txn_lock:
        record = _resolve_open_txn(transaction_id, doc_name)
        record["operations"].append(
            {
                "seq": len(record["operations"]) + 1,
                "action": action,
                "details": details,
                "at": _now_iso(),
            }
        )


def _txn_result(res: Any) -> dict[str, Any]:
    if isinstance(res, dict) and "__ok__" in res:
        return {"success": True, **res["__ok__"]}
    if isinstance(res, dict) and "__error__" in res:
        return {"success": False, "error": res["__error__"]}
    return {"success": False, "error": str(res)}


def _default_autosave_dir() -> str:
    return os.path.expanduser("~")


def _autosave_document(doc, fallback_dir: str | None = None) -> dict[str, Any]:
    entry = {"name": doc.Name, "saved": False, "path": None, "reason": None}
    current_path = getattr(doc, "FileName", "") or ""
    if current_path:
        try:
            doc.save()
            entry["saved"] = True
            entry["path"] = current_path
        except Exception as e:
            entry["reason"] = f"save failed: {e}"
        return entry
    target_dir = fallback_dir or _default_autosave_dir()
    try:
        os.makedirs(target_dir, exist_ok=True)
        target_path = os.path.join(target_dir, f"{doc.Name}.FCStd")
        doc.saveAs(target_path)
        entry["saved"] = True
        entry["path"] = target_path
    except Exception as e:
        entry["reason"] = f"saveAs failed: {e}"
    return entry


_session_backup_dir: str | None = None


def _ensure_backup_dir() -> str:
    global _session_backup_dir
    if _session_backup_dir is None or not os.path.isdir(_session_backup_dir):
        _session_backup_dir = tempfile.mkdtemp(prefix="parashell_txn_")
    return _session_backup_dir


def _write_doc_backup(doc, tag: str) -> str | None:
    try:
        backup_dir = _ensure_backup_dir()
        safe = re.sub(r"[^A-Za-z0-9_.-]", "_", f"{doc.Name}__{tag}")
        path = os.path.join(backup_dir, f"{safe}.FCStd")
        doc.saveCopy(path)
        return path
    except Exception:
        return None


def _restore_doc_from_backup(doc_name: str, backup_path: str):
    if not backup_path or not os.path.isfile(backup_path):
        raise ValueError("Checkpoint file is missing; cannot roll back.")
    existing = FreeCAD.getDocument(doc_name)
    original_file = ""
    if existing is not None:
        original_file = getattr(existing, "FileName", "") or ""
        FreeCAD.closeDocument(doc_name)
    restored = FreeCAD.openDocument(backup_path)
    if original_file:
        restored.saveAs(original_file)
    restored.recompute()
    return restored


_RPC_METHOD_REGISTRY = {}


def rpc_method(fn):
    _RPC_METHOD_REGISTRY[fn.__name__] = fn
    return fn


class FreeCADRPC:
    TIMEOUT = 30

    def _dispatch(self, method, params):
        if method.startswith("_"):
            raise Exception(f'method "{method}" is not supported')
        func = getattr(self, method, None)
        if not callable(func):
            raise Exception(f'method "{method}" is not supported')
        with _rpc_dispatch_lock:
            return func(*params)


rpc_server_thread = None
rpc_server_instance = None


def _create_rpc_server(host, port, token):
    handler = _AuthenticatedRPCRequestHandler
    try:
        return LocalOnlyXMLRPCServer(
            (host, port),
            auth_token=token,
            allow_none=True,
            logRequests=False,
            requestHandler=handler,
        )
    except OSError:
        return LocalOnlyXMLRPCServer(
            (host, 0),
            auth_token=token,
            allow_none=True,
            logRequests=False,
            requestHandler=handler,
        )


def start_rpc_server(port=9875):
    global rpc_server_thread, rpc_server_instance

    if rpc_server_instance:
        return True

    host = "127.0.0.1"
    token = secrets.token_urlsafe(32)

    server = _create_rpc_server(host, port, token)
    server.register_instance(FreeCADRPC())
    rpc_server_instance = server

    bound_host, bound_port = server.server_address[0], server.server_address[1]

    try:
        discovery.publish(
            host=bound_host,
            port=bound_port,
            token=token,
            pid=os.getpid(),
        )
    except Exception as exc:
        FreeCAD.Console.PrintWarning(
            f"Parashell MCP: Could not publish bridge discovery descriptor: {exc}\n"
        )

    def server_loop():
        FreeCAD.Console.PrintMessage(
            f"Parashell MCP RPC Server started at {bound_host}:{bound_port}\n"
        )
        server.serve_forever()

    rpc_server_thread = threading.Thread(target=server_loop, daemon=True)
    rpc_server_thread.start()
    _ensure_gui_task_runner()
    return True


def stop_rpc_server():
    global rpc_server_instance, rpc_server_thread

    if rpc_server_instance:
        rpc_server_instance.shutdown()
        rpc_server_thread.join()
        rpc_server_instance = None
        rpc_server_thread = None
        discovery.clear()
        FreeCAD.Console.PrintMessage("Parashell MCP RPC Server stopped.\n")


_RPC_METHODS_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "rpc_methods"
)
_rpc_methods_loaded = False


def _load_rpc_methods():
    global _rpc_methods_loaded
    if _rpc_methods_loaded:
        return
    if not os.path.isdir(_RPC_METHODS_DIR):
        raise RuntimeError(f"rpc_methods directory missing: {_RPC_METHODS_DIR}")
    if _RPC_METHODS_DIR not in sys.path:
        sys.path.insert(0, _RPC_METHODS_DIR)
    suffixes = tuple(importlib.machinery.SOURCE_SUFFIXES) + tuple(
        importlib.machinery.EXTENSION_SUFFIXES
    )
    seen = set()
    names = []
    for entry in sorted(os.listdir(_RPC_METHODS_DIR)):
        if entry.startswith("_"):
            continue
        stem = None
        for suffix in suffixes:
            if entry.endswith(suffix):
                stem = entry[: -len(suffix)]
                break
        if stem is None or stem in seen:
            continue
        seen.add(stem)
        names.append(stem)
    if not names:
        raise RuntimeError(f"No rpc method modules found in {_RPC_METHODS_DIR}")
    for name in names:
        importlib.import_module(name)
    for name, fn in _RPC_METHOD_REGISTRY.items():
        setattr(FreeCADRPC, name, fn)
    _rpc_methods_loaded = True


__all__ = [name for name in list(globals()) if not name.startswith("__")]

_load_rpc_methods()
