#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
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

__title__ = "Command Quick Analysis"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

import FreeCAD
from FemTools import FemTools
from FemCommands import FemCommands

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui


class _CommandQuickAnalysis(FemCommands):
    def __init__(self):
        super(_CommandQuickAnalysis, self).__init__()
        self.resources = {'Pixmap': 'fem-quick-analysis',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Quick_Analysis", "Run CalculiX ccx"),
                          'Accel': "R, C",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Quick_Analysis", "Write .inp file and run CalculiX ccx")}
        self.is_active = 'with_solver'

    def Activated(self):
        def load_results(ret_code):
            if ret_code == 0:
                self.fea.load_results()
                self.show_results_on_mesh()
                self.hide_parts_constraints_show_meshes()

            else:
                print ("CalculiX failed ccx finished with error {}".format(ret_code))

        self.fea = FemTools()
        self.fea.reset_all()
        message = self.fea.check_prerequisites()
        if message:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", message)
            return
        self.fea.finished.connect(load_results)
        QtCore.QThreadPool.globalInstance().start(self.fea)

    def show_results_on_mesh(self):
        #FIXME proprer mesh refreshing as per FreeCAD.FEM_dialog settings required
        # or confirmation that it's safe to call restore_result_dialog
        #FIXME if an analysis has multiple results (frequence) the first result object found is restored
        import _TaskPanelResultControl
        tp = _TaskPanelResultControl._TaskPanelResultControl()
        tp.restore_result_dialog()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_Quick_Analysis', _CommandQuickAnalysis())
