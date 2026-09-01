# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

try:
    from PySide6.QtWidgets import *
    from PySide6.QtWidgets import QMainWindow as QMainWindow, QWidget as QWidget
except ImportError:
    from PySide2.QtWidgets import *
    from PySide2.QtWidgets import QMainWindow as QMainWindow, QWidget as QWidget
