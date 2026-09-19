# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

try:
    from PySide6.QtCore import *

    # Pyrefly does not currently resolve QTimer through the wildcard re-export,
    # so keep this compatibility export explicit.
    from PySide6.QtCore import QTimer as QTimer
except ImportError:
    from PySide2.QtCore import *

    # Keep the Qt5 compatibility export explicit for the same reason.
    from PySide2.QtCore import QTimer as QTimer

def QT_TRANSLATE_NOOP(context: str, source_text: str, /) -> str: ...
