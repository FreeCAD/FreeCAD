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

import os

translate = App.Qt.translate


class TaskAddOffsetVertex:
    """Provides the TechDraw AddOffsetVertex Task Dialog."""

    def __init__(self, view, vertex):
        self._uiPath = App.getHomePath()
        self._uiPath = os.path.join(
            self._uiPath, "Mod/TechDraw/TechDrawTools/Gui/TaskAddOffsetVertex.ui"
        )
        self.form = Gui.PySideUic.loadUi(self._uiPath)
        self.form.setWindowTitle(translate("TechDraw_AddOffsetVertex", "Offset Vertex"))
        self.view = view
        self.vertex = vertex
        self._previewTag = None
        self.form.dSpinBoxX.valueChanged.connect(self.onOffsetChanged)
        self.form.dSpinBoxY.valueChanged.connect(self.onOffsetChanged)

        sel = Gui.Selection.getSelectionEx()
        if sel and sel[0].SubElementNames:
            sub = sel[0].SubElementNames[0]
            self.form.le_SourceVertex.setText(f"{view.Label}.{sub}")

        App.ActiveDocument.openTransaction("Add offset vertex")

    def onOffsetChanged(self):
        point = TechDraw.makeCanonicalPoint(self.view, self.vertex.Point, False)
        offset = App.Vector(self.form.dSpinBoxX.value(), self.form.dSpinBoxY.value(), 0)
        if self._previewTag:
            self.view.removeCosmeticVertex(self._previewTag)
        self._previewTag = self.view.makeCosmeticVertex(point + offset)
        self.view.requestPaint()

    def accept(self):
        if not self._previewTag:
            point = TechDraw.makeCanonicalPoint(self.view, self.vertex.Point, False)
            offset = App.Vector(self.form.dSpinBoxX.value(), self.form.dSpinBoxY.value(), 0)
            self.view.makeCosmeticVertex(point + offset)
        self.view.requestPaint()
        Gui.Control.closeDialog()
        App.ActiveDocument.commitTransaction()

    def reject(self):
        if self._previewTag:
            self.view.removeCosmeticVertex(self._previewTag)
            self.view.requestPaint()
        App.ActiveDocument.abortTransaction()
        return True
