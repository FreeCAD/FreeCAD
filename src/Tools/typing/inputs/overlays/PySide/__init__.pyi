# SPDX-License-Identifier: LGPL-2.1-or-later

"""The compatibility package installed by FreeCAD's PySide setup."""

from __future__ import annotations

try:
    from PySide6 import __version__ as __version__, __version_info__ as __version_info__
except ImportError:
    from PySide2 import __version__ as __version__, __version_info__ as __version_info__
