
# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM postprocessing line plot ViewProvider for the document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package view_post_lineplot
#  \ingroup FEM
#  \brief view provider for post line plot object

import FreeCAD
import FreeCADGui

import FemGui
from PySide import QtGui


class VPPostLinePlot:
    """
    A View Provider for the Post LinePlot object
    """

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/FEM_PostLineplot.svg"

     def setEdit(self, vobj, mode):
        # make sure we see what we edit
        vobj.show()

        # build up the task panel
        #taskd = task_post_glyphfilter._TaskPanel(vobj)

        #show it
        #FreeCADGui.Control.showDialog(taskd)

        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None
