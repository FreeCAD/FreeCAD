# SPDX-License-Identifier: LGPL-2.1-or-later

"""BIM command for interactive face extrusion."""

import FreeCADGui
from PySide.QtCore import QT_TRANSLATE_NOOP

from draftguitools.gui_trimex import ExtrudeFace


class BimExtrudeFace(ExtrudeFace):
    """BIM face extrusion command."""

    def GetResources(self):
        return {
            "Pixmap": "BIM_ExtrudeFace",
            "MenuText": QT_TRANSLATE_NOOP("BIM_ExtrudeFace", "Extrude Face"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_ExtrudeFace", "Extrudes a selected face into a solid"
            ),
        }


FreeCADGui.addCommand("BIM_ExtrudeFace", BimExtrudeFace())
