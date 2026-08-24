"""Small Qt compatibility layer for FreeCAD 1.1."""
from __future__ import annotations


def load_qt():
    try:
        from PySide import QtCore, QtGui

        QtWidgets = QtGui
        return QtCore, QtGui, QtWidgets
    except ImportError:
        from PySide2 import QtCore, QtGui, QtWidgets

        return QtCore, QtGui, QtWidgets


def message_box(parent, title: str, message: str, *, critical: bool = False):
    _QtCore, _QtGui, QtWidgets = load_qt()
    if critical:
        return QtWidgets.QMessageBox.critical(parent, title, message)
    return QtWidgets.QMessageBox.information(parent, title, message)
