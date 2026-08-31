# SPDX-License-Identifier: LGPL-2.1-or-later

"""Qt event processing and synchronization primitives for GUI tests."""

from __future__ import annotations

from collections.abc import Callable
import time
from typing import Any, TypeAlias

import FreeCAD

try:
    import FreeCADGui

    GUI_MODULE_AVAILABLE = True
except ImportError:
    FreeCADGui = None
    GUI_MODULE_AVAILABLE = False

try:
    from PySide import QtCore, QtGui, QtWidgets
except ImportError:
    try:
        from PySide6 import QtCore, QtGui, QtWidgets
    except ImportError:
        QtCore = None
        QtGui = None
        QtWidgets = None


Predicate: TypeAlias = Callable[[], bool]
"""A condition polled by :func:`wait_until`."""


def gui_available() -> bool:
    """Return whether a usable FreeCAD main window is available."""
    if not GUI_MODULE_AVAILABLE:
        return False

    try:
        return FreeCAD.GuiUp and FreeCADGui.getMainWindow() is not None
    except (AttributeError, RuntimeError):
        return False


def qt_application() -> Any | None:
    """Return the active Qt application, if the Qt bindings are available."""
    if QtWidgets is None:
        return None

    return QtWidgets.QApplication.instance()


def pump(timeout_ms: int = 50) -> None:
    """Process GUI events for approximately ``timeout_ms`` milliseconds.

    This is useful when a test deliberately wants to give an animation or a
    known asynchronous operation time to run. Prefer :func:`wait_until` when
    the test can express a state that proves the operation completed.
    """
    if QtCore is None or qt_application() is None:
        return

    loop = QtCore.QEventLoop()
    QtCore.QTimer.singleShot(max(0, timeout_ms), loop.quit)
    exec_method = getattr(loop, "exec", None) or loop.exec_
    exec_method()


def flush_gui(timeout_ms: int = 0) -> None:
    """Flush pending Qt and FreeCAD GUI work.

    If ``timeout_ms`` is non-zero, continue processing events for roughly
    that long after the immediate flush.
    """
    if not gui_available():
        return

    app = qt_application()
    if app is not None:
        app.processEvents()

    FreeCADGui.updateGui()

    if timeout_ms:
        pump(timeout_ms)


def wait_until(
    predicate: Predicate,
    timeout_ms: int = 1000,
    step_ms: int = 10,
) -> bool:
    """Poll ``predicate`` while processing GUI events until it becomes true.

    The predicate is evaluated immediately, then again after each event-loop
    iteration. The final predicate evaluation is returned when the timeout is
    reached, so a state change that happens at the boundary is not lost.
    """
    timeout_ms = max(0, timeout_ms)
    step_ms = max(1, step_ms)
    deadline = time.monotonic() + timeout_ms / 1000.0

    while True:
        if predicate():
            return True

        remaining = deadline - time.monotonic()
        if remaining <= 0:
            return bool(predicate())

        flush_gui(min(step_ms, max(1, int(remaining * 1000))))
