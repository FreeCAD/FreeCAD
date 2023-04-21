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
"""Provides GUI tools to convert Draft objects to Sketches and back.

Many Draft objects will be converted to a single non-constrained Sketch.

However, a single sketch with disconnected traces will be converted
into several individual Draft objects.
"""
## @package gui_draft2sketch
# \ingroup draftguitools
# \brief Provides GUI tools to convert Draft objects to Sketches and back.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Draft2Sketch(gui_base_original.Modifier):
    """Gui Command for the Draft2Sketch tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Draft2Sketch',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Draft2Sketch", "Draft to Sketch"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Draft2Sketch", "Convert bidirectionally between Draft objects and Sketches.\nMany Draft objects will be converted into a single non-constrained Sketch.\nHowever, a single sketch with disconnected traces will be converted into several individual Draft objects.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Draft2Sketch, self).Activated(name="Convert Draft/Sketch")
        if not Gui.Selection.getSelection():
            if self.ui:
                self.ui.selectUi(on_close_call=self.finish)
                _msg(translate("draft", "Select an object to convert."))
                self.call = self.view.addEventCallback(
                    "SoEvent",
                    gui_tool_utils.selectObject)
        else:
            self.proceed()

    def proceed(self):
        """Proceed with the command if one object was selected."""
        sel = Gui.Selection.getSelection()
        allSketches = True
        allDraft = True
        Gui.addModule("Draft")
        for obj in sel:
            if obj.isDerivedFrom("Sketcher::SketchObject"):
                allDraft = False
            elif (obj.isDerivedFrom("Part::Part2DObjectPython")
                  or obj.isDerivedFrom("Part::Feature")):
                allSketches = False
            else:
                allDraft = False
                allSketches = False

        if not sel:
            return
        elif allDraft:
            _cmd = "Draft.make_sketch"
            _cmd += "("
            _cmd += "FreeCADGui.Selection.getSelection(), "
            _cmd += "autoconstraints=True"
            _cmd += ")"
            _cmd_list = ['sk = ' + _cmd,
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Convert to Sketch"),
                        _cmd_list)
        elif allSketches:
            n = 0
            _cmd_list = list()
            for o in sel:
                _cmd = "Draft.draftify"
                _cmd += "("
                _cmd += "FreeCAD.ActiveDocument." + o.Name + ", "
                _cmd += "delete=False"
                _cmd += ")"
                _cmd_list.append("df" + str(n) + " = " + _cmd)
                n += 1

            _cmd_list.append('FreeCAD.ActiveDocument.recompute()')
            self.commit(translate("draft", "Convert to Draft"),
                        _cmd_list)
        else:
            _cmd_list = list()
            n = 0
            for obj in sel:
                _cmd_df = "Draft.draftify"
                _cmd_df += "("
                _cmd_df += "FreeCAD.ActiveDocument." + obj.Name + ", "
                _cmd_df += "delete=False"
                _cmd_df += ")"

                _cmd_sk = "Draft.make_sketch"
                _cmd_sk += "("
                _cmd_sk += "FreeCAD.ActiveDocument." + obj.Name + ", "
                _cmd_sk += "autoconstraints=True"
                _cmd_sk += ")"

                if obj.isDerivedFrom("Sketcher::SketchObject"):
                    _cmd_list.append("obj" + str(n) + " = " + _cmd_df)
                elif (obj.isDerivedFrom("Part::Part2DObjectPython")
                      or obj.isDerivedFrom("Part::Feature")):
                    _cmd_list.append("obj" + str(n) + " = " + _cmd_sk)
                #elif obj.isDerivedFrom("Part::Feature"):
                #    # if (len(obj.Shape.Wires) == 1
                #    #     or len(obj.Shape.Edges) == 1):
                #    _cmd_list.append("obj" + str(n) + " = " + _cmd_sk)
                n += 1
            _cmd_list.append('FreeCAD.ActiveDocument.recompute()')
            self.commit(translate("draft", "Convert Draft/Sketch"),
                        _cmd_list)
        self.finish()


Gui.addCommand('Draft_Draft2Sketch', Draft2Sketch())

## @}
