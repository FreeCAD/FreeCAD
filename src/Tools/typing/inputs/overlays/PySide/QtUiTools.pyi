# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

try:
    from PySide6.QtUiTools import *
    from PySide6.QtUiTools import QUiLoader as QUiLoader
except ImportError:
    from PySide2.QtUiTools import *
    from PySide2.QtUiTools import QUiLoader as QUiLoader
