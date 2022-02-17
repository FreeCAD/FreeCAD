# ***************************************************************************
# *   (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>                  *
# *   (c) 2009, 2010 Ken Cline <cline@frii.com>                             *
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides GUI tools to project an object into a 2D plane.

This creates a 2D shape in the 3D view itself. This projection
can be further used to create a technical drawing using
the TechDraw Workbench.
"""
## @package gui_shape2dview
# \ingroup draftguitools
# \brief Provides GUI tools to project an object into a 2D plane.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import DraftVecUtils
import Draft_rc
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils

from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Shape2DView(gui_base_original.Modifier):
    """Gui Command for the Shape2DView tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_2DShapeView',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Shape2DView", "Shape 2D view"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Shape2DView", "Creates a 2D projection of the selected objects on the XY plane.\nThe initial projection direction is the negative of the current active view direction.\nYou can select individual faces to project, or the entire solid, and also include hidden lines.\nThese projections can be used to create technical drawings with the TechDraw Workbench.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Shape2DView, self).Activated(name="Project 2D view")
        if not Gui.Selection.getSelection():
            if self.ui:
                self.ui.selectUi(on_close_call=self.finish)
                _msg(translate("draft", "Select an object to project"))
                self.call = self.view.addEventCallback(
                    "SoEvent",
                    gui_tool_utils.selectObject)
        else:
            self.proceed()

    def proceed(self):
        """Proceed with the command if one object was selected."""
        faces = []
        objs = []
        vec = Gui.ActiveDocument.ActiveView.getViewDirection().negative()
        sel = Gui.Selection.getSelectionEx()
        for s in sel:
            objs.append(s.Object)
            for e in s.SubElementNames:
                if "Face" in e:
                    faces.append(int(e[4:]) - 1)
        # print(objs, faces)
        commitlist = []
        Gui.addModule("Draft")
        if len(objs) == 1 and faces:
            _cmd = "Draft.make_shape2dview"
            _cmd += "("
            _cmd += "FreeCAD.ActiveDocument." + objs[0].Name + ", "
            _cmd += DraftVecUtils.toString(vec) + ", "
            _cmd += "facenumbers=" + str(faces)
            _cmd += ")"
            commitlist.append("sv = " + _cmd)
        else:
            n = 0
            for o in objs:
                _cmd = "Draft.make_shape2dview"
                _cmd += "("
                _cmd += "FreeCAD.ActiveDocument." + o.Name + ", "
                _cmd += DraftVecUtils.toString(vec)
                _cmd += ")"
                commitlist.append("sv" + str(n) + " = " + _cmd)
                n += 1
        if commitlist:
            commitlist.append("FreeCAD.ActiveDocument.recompute()")
            self.commit(translate("draft", "Create 2D view"),
                        commitlist)
        self.finish()


Gui.addCommand('Draft_Shape2DView', Shape2DView())

## @}
