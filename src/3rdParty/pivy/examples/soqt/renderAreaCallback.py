from __future__ import print_function
import sys
from pivy import coin
from pivy.gui import soqt
from pivy.qt.QtCore import QEvent


def foo(a, event):
    if event.type() == QEvent.MouseButtonPress:
        print(event)  # event is not a QMouseEvent like it was with pyqt4
        print(event.button())


appWindow = soqt.SoQt.init(sys.argv[0])
root = coin.SoSeparator()
myRenderArea = soqt.SoQtRenderArea(appWindow)
myRenderArea.setSceneGraph(root)
myRenderArea.setTitle("My Event Handler")
myRenderArea.setEventCallback(foo, myRenderArea)

myRenderArea.show()
soqt.SoQt.show(appWindow)
soqt.SoQt.mainLoop()
