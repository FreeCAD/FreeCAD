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


class Camera(SceneDetail):
    """Camera(doc): TempoVis plugin for saving and restoring camera."""

    class_id = "SDCamera"

    def __init__(self, doc):
        self.doc = doc
        self.key = "the_cam"

    def _viewer(self):
        gdoc = FreeCADGui.getDocument(self.doc.Name)
        v = gdoc.activeView()
        if not hasattr(v, "getCamera"):
            v = gdoc.mdiViewsOfType("Gui::View3DInventor")[0]
        return v

    def scene_value(self):
        return self._viewer().getCamera()

    def apply_data(self, val):
        self._viewer().setCamera(val)
