# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

try:
    from PySide6.QtSvg import *
    from PySide6.QtSvg import QSvgRenderer as QSvgRenderer
except ImportError:
    from PySide2.QtSvg import *
    from PySide2.QtSvg import QSvgRenderer as QSvgRenderer
