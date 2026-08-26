# SPDX-License-Identifier: LGPL-2.1-or-later

"""GUI bootstrap for the cad-x assistant.

Registers the assistant panel command and docks the panel once the main
window is ready.  The dock's toggle action appears under View > Panels as
``CadX Assistant``.
"""

from __future__ import annotations

import FreeCAD as App
import FreeCADGui as Gui


def _warn(message: str) -> None:
    App.Console.PrintWarning(f"cad-x: {message}\n")


try:
    import CadXGui

    CadXGui.ensure_commands_registered()
except Exception as exc:
    _warn(f"GUI bootstrap failed: {exc}")
