from PySide import QtGui, QtCore
from .util import get_pixmap_from_shape


class ShapeWidget(QtGui.QWidget):
    def __init__(self, shape, parent=None):
        super(ShapeWidget, self).__init__(parent)
        self.layout = QtGui.QVBoxLayout(self)
        self.layout.setAlignment(QtCore.Qt.AlignHCenter)

        self.shape = shape
        self.icon_size = QtCore.QSize(200, 235)
        self.icon_widget = QtGui.QLabel()
        self.layout.addWidget(self.icon_widget)

        self._update_icon()

    def _update_icon(self):
        ratio = self.devicePixelRatioF()
        pixmap = get_pixmap_from_shape(self.shape, self.icon_size, ratio)
        if pixmap:
            self.icon_widget.setPixmap(pixmap)
