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

__title__ = "Command Purge Fem Results"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

import FreeCAD
from FemCommands import FemCommands
import FemTools

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore


class _CommandPurgeResults(FemCommands):
    # the Fem_PurgeResults command definition
    def __init__(self):
        super(_CommandPurgeResults, self).__init__()
        self.resources = {'Pixmap': 'fem-purge-results',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_PurgeResults", "Purge results"),
                          'Accel': "S, S",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_PurgeResults", "Purges all results from active analysis")}
        self.is_active = 'with_results'

    def Activated(self):
        fea = FemTools.FemTools()
        fea.reset_all()

        self.hide_meshes_show_parts_constraints()

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_PurgeResults', _CommandPurgeResults())
