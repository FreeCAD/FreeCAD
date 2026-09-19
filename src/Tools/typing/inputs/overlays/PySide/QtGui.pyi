# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

# FreeCAD's Qt5/Qt6 compatibility module also reexports QtWidgets from QtGui.
try:
    from PySide6.QtGui import *
    from PySide6.QtGui import QColor as QColor
    from PySide6.QtWidgets import *
except ImportError:
    from PySide2.QtGui import *
    from PySide2.QtGui import QColor as QColor
    from PySide2.QtWidgets import *
