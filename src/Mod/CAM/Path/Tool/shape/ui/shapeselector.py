# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
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
from typing import Optional
import FreeCADGui
from functools import partial
from PySide import QtGui
from ...camassets import cam_assets
from .. import ToolBitShape
from .flowlayout import FlowLayout
from .shapebutton import ShapeButton


class ShapeSelector:
    def __init__(self):
        self.shape = None
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ShapeSelector.ui")

        self.form.buttonBox.clicked.connect(self.form.close)

        self.flows = {}

        self.update_shapes()
        self.form.toolBox.setCurrentIndex(0)

    def _add_shape_group(self, toolbox):
        if toolbox in self.flows:
            return self.flows[toolbox]
        flow = FlowLayout(toolbox, orientation=QtGui.Qt.Horizontal)
        flow.widthChanged.connect(lambda x: toolbox.setMinimumWidth(x))
        self.flows[toolbox] = flow
        return flow

    def _add_shapes(self, toolbox, shapes):
        flow = self._add_shape_group(toolbox)

        # Remove all shapes first.
        for i in reversed(range(flow.count())):
            flow.itemAt(i).widget().setParent(None)

        # Add all shapes.
        for shape in sorted(shapes, key=lambda x: x.label):
            button = ShapeButton(shape)
            flow.addWidget(button)
            cb = partial(self.on_shape_button_clicked, shape)
            button.clicked.connect(cb)

    def update_shapes(self):
        # Retrieve each shape asset
        builtin = cam_assets.fetch(asset_type="toolbitshape", store="builtin")
        builtin = {c.id: c for c in builtin}
        custom = cam_assets.fetch(asset_type="toolbitshape", store="local")
        for shape in custom:
            builtin.pop(shape.id, None)
        self._add_shapes(self.form.standardTools, builtin.values())
        self._add_shapes(self.form.customTools, custom)

    def on_shape_button_clicked(self, shape):
        self.shape = shape
        self.form.close()

    def show(self) -> Optional[ToolBitShape]:
        self.form.exec()
        return self.shape
