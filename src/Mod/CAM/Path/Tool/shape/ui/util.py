from PySide import QtCore, QtGui, QtSvg


def qpixmap_from_png(byte_array, icon_size, ratio=1.0):
    render_size = QtCore.QSize(icon_size.width() * ratio, icon_size.height() * ratio)
    pixmap = QtGui.QPixmap(render_size)
    pixmap.fill(QtGui.Qt.transparent)
    pixmap.loadFromData(byte_array)
    return pixmap.scaled(icon_size, QtCore.Qt.KeepAspectRatio)


def qpixmap_from_svg(byte_array, icon_size, ratio=1.0):
    render_size = QtCore.QSize(icon_size.width() * ratio, icon_size.height() * ratio)

    image = QtGui.QImage(render_size, QtGui.QImage.Format_ARGB32)
    image.fill(QtGui.Qt.transparent)
    painter = QtGui.QPainter(image)

    buffer = QtCore.QBuffer(byte_array)  # PySide6
    buffer.open(QtCore.QIODevice.ReadOnly)
    data = QtCore.QXmlStreamReader(buffer)
    renderer = QtSvg.QSvgRenderer(data)
    renderer.setAspectRatioMode(QtCore.Qt.KeepAspectRatio)
    renderer.render(painter)
    painter.end()

    return QtGui.QPixmap.fromImage(image)


def get_pixmap_from_shape(shape, size, ratio):
    icon_type, icon_bytes = shape.get_icon()
    if not icon_type:
        return None
    icon_ba = QtCore.QByteArray(icon_bytes)

    if icon_type == "svg":
        return qpixmap_from_svg(icon_ba, size, ratio)
    elif icon_type == "png":
        return qpixmap_from_png(icon_ba, size, ratio)

    return None
