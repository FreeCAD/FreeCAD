# Gera uma folha de contato dos ícones em vários tamanhos (para revisão).
import os
import sys

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
from PySide6 import QtCore, QtGui, QtSvg

app = QtGui.QGuiApplication.instance() or QtGui.QGuiApplication([])


def folha(icons, out, sizes=(64, 32, 18), cols=8, label=True):
    """icons: lista de (nome, svg)."""
    cw = sum(sizes) + 10 * len(sizes) + 20
    ch = max(sizes) + (22 if label else 10)
    rows = (len(icons) + cols - 1) // cols
    img = QtGui.QImage(cols * cw, rows * ch + 10, QtGui.QImage.Format_RGB32)
    img.fill(QtGui.QColor("#FFFFFF"))
    p = QtGui.QPainter(img)
    p.setRenderHint(QtGui.QPainter.Antialiasing)
    f = p.font()
    f.setPixelSize(10)
    p.setFont(f)
    for i, (name, svg) in enumerate(icons):
        x = (i % cols) * cw + 10
        y = (i // cols) * ch + 6
        r = QtSvg.QSvgRenderer(QtCore.QByteArray(svg.encode("utf-8")))
        xx = x
        for s in sizes:
            r.render(p, QtCore.QRectF(xx, y + (max(sizes) - s) / 2, s, s))
            xx += s + 10
        if label:
            p.setPen(QtGui.QColor("#556"))
            p.drawText(QtCore.QRect(x - 6, y + max(sizes) + 2, cw - 8, 14), QtCore.Qt.AlignLeft, name[:30])
    p.end()
    img.save(out)
