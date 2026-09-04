# SPDX-License-Identifier: LGPL-2.1-or-later

"""BIM command for interactive face extrusion."""

import FreeCADGui

from draftguitools.gui_trimex import ExtrudeFace

FreeCADGui.addCommand("BIM_ExtrudeFace", ExtrudeFace())
