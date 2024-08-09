# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""The BIM ShapeView command"""


import FreeCAD
import FreeCADGui
from draftguitools import gui_shape2dview

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_Shape2DView(gui_shape2dview.Shape2DView):

    def GetResources(self):
        d = super().GetResources()
        d["Pixmap"] = "Arch_BuildingPart_Tree"
        d["MenuText"] = QT_TRANSLATE_NOOP("BIM_Shape2DView", "Section view")
        d['Accel'] = "V, V"
        return d

    def proceed(self):
        """Proceed with the command if one object was selected."""
        # difference from Draft: it sets InPlace to False
        import DraftVecUtils
        if self.call is not None:
            self.end_callbacks(self.call)
        faces = []
        objs = []
        view = FreeCADGui.ActiveDocument.ActiveView
        vec = view.getViewDirection().negative()
        sel = FreeCADGui.Selection.getSelectionEx()
        for s in sel:
            objs.append(s.Object)
            for e in s.SubElementNames:
                if "Face" in e:
                    faces.append(int(e[4:]) - 1)
        # print(objs, faces)
        commitlist = []
        FreeCADGui.addModule("Draft")
        if len(objs) == 1 and faces:
            _cmd = "Draft.make_shape2dview"
            _cmd += "("
            _cmd += "FreeCAD.ActiveDocument." + objs[0].Name + ", "
            _cmd += DraftVecUtils.toString(vec) + ", "
            _cmd += "facenumbers=" + str(faces)
            _cmd += ")"
            commitlist.append("sv = " + _cmd)
            commitlist.append("sv.InPlace = False")
        else:
            n = 0
            for o in objs:
                _cmd = "Draft.make_shape2dview"
                _cmd += "("
                _cmd += "FreeCAD.ActiveDocument." + o.Name + ", "
                _cmd += DraftVecUtils.toString(vec)
                _cmd += ")"
                commitlist.append("sv" + str(n) + " = " + _cmd)
                commitlist.append("sv" + str(n) + ".InPlace = False")
                n += 1
        if commitlist:
            commitlist.append("FreeCAD.ActiveDocument.recompute()")
            self.commit(translate("draft", "Create 2D view"),
                        commitlist)
        self.finish()


class BIM_Shape2DCut(BIM_Shape2DView):

    def GetResources(self):
        d = super().GetResources()
        d["Pixmap"] = "Arch_View_Cut"
        d["MenuText"] = QT_TRANSLATE_NOOP("BIM_Shape2DView", "Section cut")
        d['Accel'] = "V, C"
        return d

    def proceed(self):
        super().proceed()
        FreeCADGui.doCommand("sv.ProjectionMode = \"Cutfaces\"")


FreeCADGui.addCommand("BIM_Shape2DView", BIM_Shape2DView())
FreeCADGui.addCommand("BIM_Shape2DCut", BIM_Shape2DCut())
