# /***************************************************************************
# *   Copyright (c) 2019 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/

from Show.SceneDetail import SceneDetail

import FreeCADGui


class Workbench(SceneDetail):
    """Workbench(wb = None): Plugin for TempoVis for changing active workbench.
    wb: string, a name of a workbench (e.g. 'SketcherWorkbench')"""

    class_id = "SDWorkbench"
    mild_restore = True

    def __init__(self, wb=None):
        self.key = "workbench"
        if wb is not None:
            self.data = wb

    def scene_value(self):
        return FreeCADGui.activeWorkbench().name()

    def apply_data(self, val):
        FreeCADGui.activateWorkbench(val)
