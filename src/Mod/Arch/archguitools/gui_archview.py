#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
#*   Copyright (c) 2020 Carlo Pavan                                        *
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
"""Provide the Arch_Wall command."""
## @package gui_wall
# \ingroup ARCH
# \brief Provide the Arch_Wall command used in Arch to create an Arch Wall.

import FreeCAD as App
import FreeCADGui as Gui
import Draft
import archmake.make_arch_view as make_arch_view
from PySide import QtCore,QtGui

# ---------------------------------------------------------------------------
# this is just a very rough implementation to test the objects
# ---------------------------------------------------------------------------

class Arch_View:

    "the Arch Wall command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_SectionPlane_Tree.svg',
                'MenuText': "Arch View EXPERIMENTAL",
                'ToolTip': "EXPERIMENTAL\nCreate an Arch View object"}

    def IsActive(self):

        return not App.ActiveDocument is None

    def Activated(self):
        make_arch_view.make_arch_view()



Gui.addCommand('Arch_View', Arch_View())