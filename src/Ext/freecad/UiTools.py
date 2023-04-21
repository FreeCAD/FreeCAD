# (c) 2021 Werner Mayer LGPL

from PySide import QtUiTools
import FreeCADGui as Gui


class QUiLoader(QtUiTools.QUiLoader):
    """
    This is an extension of Qt's QUiLoader to also create custom widgets
    """
    def __init__(self, arg = None):
        super(QUiLoader, self).__init__(arg)
        self.ui = Gui.PySideUic

    def createWidget(self, className, parent = None, name = ""):
        widget = self.ui.createCustomWidget(className, parent, name)
        if not widget:
            widget = super(QUiLoader, self).createWidget(className, parent, name)
        return widget

