from PySide import QtGui, QtCore
from .util import qpixmap_from_svg, qpixmap_from_png


class ShapeButton(QtGui.QToolButton):
    def __init__(self, shape, parent=None):
        super(ShapeButton, self).__init__(parent)
        self.shape = shape

        self.setToolButtonStyle(QtCore.Qt.ToolButtonTextUnderIcon)
        self.setText(shape.label)

        self.setFixedSize(128, 128)
        self.setBaseSize(128, 128)
        self.icon_size = QtCore.QSize(71, 100)
        self.setIconSize(self.icon_size)

        self._update_icon()

    def set_text(self, text):
        self.label.setText(text)

    def _update_icon(self):
        icon_type, icon_bytes = self.shape.get_icon()
        if not icon_type:
            return
        icon_ba = QtCore.QByteArray(icon_bytes)

        if icon_type == "svg":
            icon = qpixmap_from_svg(icon_ba, self.icon_size)
        elif icon_type == "png":
            icon = qpixmap_from_png(icon_ba, self.icon_size)
        else:
            raise NotImplementedError(f"icon type {icon_type} not supported")

        self.setIcon(icon)
