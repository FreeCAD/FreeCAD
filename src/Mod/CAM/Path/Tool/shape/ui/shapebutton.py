from PySide import QtGui, QtCore


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
        icon = self.shape.get_icon()
        if icon:
            pixmap = icon.get_qpixmap(self.icon_size)
            self.setIcon(QtGui.QIcon(pixmap))
