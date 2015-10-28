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

        def IsActive(self): # bad naming, need reconsidering
            if not self.is_active:
                active = False
            elif self.is_active == 'with_document':
                active = FreeCADGui.ActiveDocument is not None
            elif self.is_active == 'with_analysis':
                active = FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None
            elif self.is_active == 'with_results':
                active = FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None and self.results_present()
            elif self.is_active == 'with_part_feature':
                active = FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None and self.part_feature_selected()
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
