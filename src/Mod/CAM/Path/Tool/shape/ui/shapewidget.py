from PySide import QtGui, QtCore


class ShapeWidget(QtGui.QWidget):
    def __init__(self, shape, parent=None):
        super(ShapeWidget, self).__init__(parent)
        self.layout = QtGui.QVBoxLayout(self)
        self.layout.setAlignment(QtCore.Qt.AlignHCenter)

        self.shape = shape
        ratio = self.devicePixelRatioF()
        self.icon_size = QtCore.QSize(200 * ratio, 235 * ratio)
        self.icon_widget = QtGui.QLabel()
        self.layout.addWidget(self.icon_widget)

        self._update_icon()

    def _update_icon(self):
        icon = self.shape.get_icon()
        if icon:
            pixmap = icon.get_qpixmap(self.icon_size)
            self.icon_widget.setPixmap(pixmap)
