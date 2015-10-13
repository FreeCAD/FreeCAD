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

__title__ = "Command Frequency Analysis"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

import FreeCAD
from FemCommands import FemCommands
from FemTools import FemTools

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui


class _CommandFrequencyAnalysis(FemCommands):
    def __init__(self):
        super(_CommandFrequencyAnalysis, self).__init__()
        self.resources = {'Pixmap': 'fem-frequency-analysis',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Frequency_Analysis", "Run frequency analysis with CalculiX ccx"),
                          'Accel': "R, F",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Frequency_Analysis", "Write .inp file and run frequency analysis with CalculiX ccx")}
        self.is_active = 'with_analysis'

    def Activated(self):
        def load_results(ret_code):
            if ret_code == 0:
                self.fea.load_results()
            else:
                print "CalculiX failed ccx finished with error {}".format(ret_code)

        self.fea = FemTools()
        self.fea.reset_all()
        self.fea.set_analysis_type('frequency')
        message = self.fea.check_prerequisites()
        if message:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", message)
            return
        self.fea.finished.connect(load_results)
        QtCore.QThreadPool.globalInstance().start(self.fea)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_Frequency_Analysis', _CommandFrequencyAnalysis())
