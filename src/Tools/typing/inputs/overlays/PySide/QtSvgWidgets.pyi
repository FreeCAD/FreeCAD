# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

# Named imports in stubs are private unless explicitly re-exported with a
# self-alias, so spell out the Qt5 compatibility exports below.
try:
    from PySide6.QtSvgWidgets import *
except ImportError:
    from PySide2.QtSvg import (
        QGraphicsSvgItem as QGraphicsSvgItem,
        QSvgWidget as QSvgWidget,
    )
