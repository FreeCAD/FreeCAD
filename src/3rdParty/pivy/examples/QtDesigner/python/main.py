#!/usr/bin/env python

import sys
from pivy.gui.soqt import SoQt
from pivy.qt.QtGui import QApplication
from pivy.qt.QtCore import QObject
from pivy.qt.QtCore import SIGNAL
from pivy.qt.QtCore import SLOT
from mainwindow import MainWindow

if __name__ == "__main__":

    SoQt.init(None)
    app = QApplication(sys.argv)
    window = MainWindow()
    QObject.connect(app, SIGNAL("lastWindowClosed()"), app, SLOT("quit()"))
    window.show()
    sys.exit(app.exec_())
