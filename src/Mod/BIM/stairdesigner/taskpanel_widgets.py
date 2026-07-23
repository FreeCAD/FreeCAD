# SPDX-License-Identifier: LGPL-2.1-or-later

"""Shared Stair Designer task-panel widgets and editors."""

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui

translate = FreeCAD.Qt.translate


def _load_task_form(resource, owner, widget_names):
    """Load a Designer form with an explicit, Python-owned outer widget.

    ``PySideUic.loadUi()`` ignores its nominal ``base`` argument in FreeCAD's
    implementation. Use ``UiLoader.load()`` directly so the loaded root is
    created with *form* as its Qt and Shiboken parent from the beginning.
    """

    form = QtGui.QWidget()
    content = FreeCADGui.UiLoader().load(resource, form)
    if content is None:
        raise RuntimeError(f"Cannot load task form {resource}")
    layout = QtGui.QVBoxLayout(form)
    layout.setContentsMargins(0, 0, 0, 0)
    layout.setSpacing(0)
    layout.addWidget(content)
    form.setWindowTitle(content.windowTitle())
    form.ui = content
    owner.__dict__.setdefault("_ui_forms", []).append((form, content))
    for name in widget_names:
        setattr(owner, name, getattr(content, name))
    return form


def _value(quantity):
    return float(quantity.Value) if hasattr(quantity, "Value") else float(quantity)


def _length_spin(value=0.0, minimum=0.0, maximum=1000000.0):
    spin = QtGui.QDoubleSpinBox()
    spin.setDecimals(2)
    spin.setRange(minimum, maximum)
    spin.setSuffix(" mm")
    spin.setValue(value)
    return spin


def _float_spin(value=0.0, minimum=0.0, maximum=1000.0, decimals=3):
    spin = QtGui.QDoubleSpinBox()
    spin.setDecimals(decimals)
    spin.setRange(minimum, maximum)
    spin.setValue(value)
    return spin


def _percent_spin(value=50):
    spin = QtGui.QSpinBox()
    spin.setRange(0, 100)
    spin.setSuffix(" %")
    spin.setValue(int(round(_value(value))))
    return spin


class _FlightTreeWidget(QtGui.QTreeWidget):
    """Flight tree that owns and consumes its Delete-key action."""

    def __init__(self, delete_callback, parent=None):
        super().__init__(parent)
        self._delete_callback = delete_callback

    def keyPressEvent(self, event):
        if event.key() == QtCore.Qt.Key_Delete:
            self._delete_callback()
            event.accept()
            return
        super().keyPressEvent(event)


class _CurrentPageTabWidget(QtGui.QTabWidget):
    """A tab widget whose height follows only its current page."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.currentChanged.connect(self._current_page_changed)

    def _hint_for_current_page(self, minimum=False):
        base_hint = (
            super().minimumSizeHint()
            if minimum
            else super().sizeHint()
        )
        current = self.currentWidget()
        if current is None:
            return base_hint

        page_hints = []
        for index in range(self.count()):
            page = self.widget(index)
            page_hints.append(
                page.minimumSizeHint() if minimum else page.sizeHint()
            )
        if not page_hints:
            return base_hint

        current_hint = (
            current.minimumSizeHint() if minimum else current.sizeHint()
        )
        tallest_page = max(hint.height() for hint in page_hints)
        chrome_height = max(0, base_hint.height() - tallest_page)
        return QtCore.QSize(
            base_hint.width(),
            current_hint.height() + chrome_height,
        )

    def sizeHint(self):
        return self._hint_for_current_page()

    def minimumSizeHint(self):
        return self._hint_for_current_page(minimum=True)

    def _current_page_changed(self, _index):
        self.updateGeometry()
        parent = self.parentWidget()
        if parent is not None:
            parent.updateGeometry()
