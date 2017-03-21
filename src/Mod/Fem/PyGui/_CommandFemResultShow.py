# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
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

__title__ = "Command Show Result"
__author__ = "Juergen Riegel, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandFemResultShow
#  \ingroup FEM
#  \brief FreeCAD Command show results for FEM workbench

from .FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore


class _CommandFemResultShow(FemCommands):
    "the FEM_ResultShow command definition"
    def __init__(self):
        super(_CommandFemResultShow, self).__init__()
        self.resources = {'Pixmap': 'fem-result',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_ResultShow", "Show result"),
                          'Accel': "S, R",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_ResultShow", "Shows and visualizes selected result data")}
        self.is_active = 'with_selresult'

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if sel[0].isDerivedFrom("Fem::FemResultObject"):
                result_object = sel[0]
                result_object.ViewObject.startEditing()


FreeCADGui.addCommand('FEM_ResultShow', _CommandFemResultShow())
