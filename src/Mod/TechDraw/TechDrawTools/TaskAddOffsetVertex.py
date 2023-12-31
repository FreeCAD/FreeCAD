# ***************************************************************************
# *   Copyright (c) 2023 edi <edi271@a1.net>                                *
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
"""Provides the TechDraw AddOffsetVertex Task Dialog."""

__title__ = "TechDrawTools.TasAddOffsetVertex"
__author__ = "edi"
__url__ = "https://www.freecad.org"
__version__ = "00.01"
__date__ = "2023/12/04"

import FreeCAD as App
import FreeCADGui as Gui
import TechDraw

from functools import partial

import os

translate = App.Qt.translate

class TaskAddOffsetVertex():
    '''Provides the TechDraw AddOffsetVertex Task Dialog.'''
    def __init__(self,view,vertex):
        self._uiPath = App.getHomePath()
        self._uiPath = os.path.join(self._uiPath, "Mod/TechDraw/TechDrawTools/Gui/TaskAddOffsetVertex.ui")
        self.form = Gui.PySideUic.loadUi(self._uiPath)
        self.form.setWindowTitle(translate("TechDraw_AddOffsetVertex", "Add offset vertex"))
        self.view = view
        self.vertex = vertex

    def accept(self):
        '''slot: OK pressed'''
        point = self.vertex.Point   # this is unscaled and inverted, but is also rotated.
        # unrotate point. Note that since this is already unscaled, we need to set the
        # third parameter to False to avoid an extra descaling.
        point = TechDraw.makeCanonicalPoint(self.view, point, False);
        xOffset = self.form.dSpinBoxX.value()
        yOffset = self.form.dSpinBoxY.value()
        offset = App.Vector(xOffset,yOffset,0)  # the offset is applied to the canonical
                                                # point.  it is an unscaled, unrotated,
                                                # uninverted relative value.
        self.view.makeCosmeticVertex(point+offset)
        Gui.Control.closeDialog()

    def reject(self):
        return True
