# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

try:
    from PySide6.QtNetwork import *
    from PySide6.QtNetwork import QNetworkRequest as QNetworkRequest
except ImportError:
    from PySide2.QtNetwork import *
    from PySide2.QtNetwork import QNetworkRequest as QNetworkRequest
