from typing import Optional
import FreeCADGui
from functools import partial
from PySide import QtGui
from ..registry import SHAPE_REGISTRY
from .. import ToolBitShape
from .flowlayout import FlowLayout
from .shapebutton import ShapeButton

class ShapeSelector():
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
        shapes = set(SHAPE_REGISTRY.get_shapes())
        builtin = set(s for s in shapes if s.is_builtin)
        self._add_shapes(self.form.standardTools, builtin)
        self._add_shapes(self.form.customTools, shapes - builtin)

    def on_shape_button_clicked(self, shape):
        self.shape = shape
        self.form.close()

    def show(self) -> Optional[ToolBitShape]:
        self.form.exec()
        return self.shape
