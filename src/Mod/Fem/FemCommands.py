#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - FreeCAD Developers                               *
#*   Author (c) 2015 - Przemo Fiszt < przemo@firszt.eu>                    *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__ = "Fem Commands"
__author__ = "Przemo Firszt"
__url__ = "http://www.freecadweb.org"

import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtCore


class FemCommands(object):
        def __init__(self):
            self.resources = {'Pixmap': 'fem-frequency-analysis',
                              'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Command", "Default Fem Command MenuText"),
                              'Accel': "",
                              'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Command", "Default Fem Command ToolTip")}
            #FIXME add option description
            self.is_active = None

        def GetResources(self):
            return self.resources

        def IsActive(self):
            if not self.is_active:
                active = False
            elif self.is_active == 'with_document':
                active = FreeCADGui.ActiveDocument is not None
            elif self.is_active == 'with_analysis':
                active = FemGui.getActiveAnalysis() is not None and self.active_analysis_in_active_doc()
            elif self.is_active == 'with_results':
                active = FemGui.getActiveAnalysis() is not None and self.active_analysis_in_active_doc() and self.results_present()
            elif self.is_active == 'with_part_feature':
                active = FreeCADGui.ActiveDocument is not None and self.part_feature_selected()
            elif self.is_active == 'with_solver':
                active = FemGui.getActiveAnalysis() is not None and self.active_analysis_in_active_doc() and self.solver_selected()
            elif self.is_active == 'with_analysis_without_solver':
                active = FemGui.getActiveAnalysis() is not None and self.active_analysis_in_active_doc() and not self.analysis_has_solver()
            return active

        def results_present(self):
            results = False
            analysis_members = FemGui.getActiveAnalysis().Member
            for o in analysis_members:
                if o.isDerivedFrom('Fem::FemResultObject'):
                    results = True
            return results

        def part_feature_selected(self):
            sel = FreeCADGui.Selection.getSelection()
            if len(sel) == 1 and sel[0].isDerivedFrom("Part::Feature"):
                return True
            else:
                return False

        def active_analysis_in_active_doc(self):
            return FemGui.getActiveAnalysis().Document is FreeCAD.ActiveDocument

        def solver_selected(self):
            sel = FreeCADGui.Selection.getSelection()
            if len(sel) == 1 and sel[0].isDerivedFrom("Fem::FemSolverObjectPython"):
                return True
            else:
                return False

        def analysis_has_solver(self):
            solver = False
            analysis_members = FemGui.getActiveAnalysis().Member
            for o in analysis_members:
                if o.isDerivedFrom("Fem::FemSolverObjectPython"):
                    solver = True
            if solver is True:
                return True
            else:
                return False

        def hide_parts_constraints_show_meshes(self):
            if FreeCAD.GuiUp:
                for acnstrmesh in FreeCAD.activeDocument().Objects:
                    #if "Constraint" in acnstrmesh.TypeId:
                    #    acnstrmesh.ViewObject.Visibility = False
                    if "Mesh" in acnstrmesh.TypeId:
                        aparttoshow = acnstrmesh.Name.replace("_Mesh", "")
                        for apart in FreeCAD.activeDocument().Objects:
                            if aparttoshow == apart.Name:
                                apart.ViewObject.Visibility = False
                        acnstrmesh.ViewObject.Visibility = True  # OvG: Hide constraints and parts and show meshes

        def hide_meshes_show_parts_constraints(self):
            if FreeCAD.GuiUp:
                for acnstrmesh in FreeCAD.activeDocument().Objects:
                    if "Constraint" in acnstrmesh.TypeId:
                        acnstrmesh.ViewObject.Visibility = True
                    if "Mesh" in acnstrmesh.TypeId:
                        aparttoshow = acnstrmesh.Name.replace("_Mesh", "")
                        for apart in FreeCAD.activeDocument().Objects:
                            if aparttoshow == apart.Name:
                                apart.ViewObject.Visibility = True
                        acnstrmesh.ViewObject.Visibility = False  # OvG: Hide meshes and show constraints and meshed part e.g. on purging results
