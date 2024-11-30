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
import FreeCAD as App
import FreeCADGui as Gui


class ClipPlane(SceneDetail):
    """ClipPlane(document, enable = None, placement = None, offset = 0.0):
    TempoVis plugin for applying clipping plane to whole project except edit root.
    enable can be 0 for disable, 1 for enable, and -1 for toggle
    (FIXME: toggle value support is a hack for while we can't read out the current state)."""

    class_id = "SDClipPlane"
    key = ""

    def __init__(self, document, enable=None, placement=None, offset=0.0):
        self.doc = document
        if enable is not None:
            if placement is not None and offset != 0.0:
                placement = placement.copy()
                dir = placement.Rotation.multVec(App.Vector(0, 0, 1))
                placement.Base = placement.Base + dir * offset
            self.data = (enable, placement)

    def scene_value(self):
        return (
            0,
            None,
        )  # hack. For until there is a way to easily query the plane, this should be good enough.

    def apply_data(self, val):
        enable, pla = val
        if enable != 0:
            self._viewer().toggleClippingPlane(enable, pla=pla)
        else:
            self._viewer().toggleClippingPlane(enable)

    def _viewer(self):
        if self.doc is None:
            gdoc = Gui.editDocument()
        else:
            gdoc = Gui.getDocument(self.doc.Name)
        v = gdoc.activeView()
        if not hasattr(v, "toggleClippingPlane"):
            v = gdoc.mdiViewsOfType("Gui::View3DInventor")[0]
        return v
