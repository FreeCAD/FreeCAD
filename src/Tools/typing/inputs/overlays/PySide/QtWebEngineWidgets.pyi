# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

try:
    from PySide6.QtWebEngineWidgets import *
    from PySide6.QtWebEngineCore import QWebEnginePage as QWebEnginePage
except ImportError:
    from PySide2.QtWebEngineWidgets import *
