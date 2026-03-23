from __future__ import print_function
import sys
from pivy import coin
from pivy.gui import soqt
# import shiboken if you want to use the widget within qt

myWindow = soqt.SoQt.init(sys.argv[0])
print(myWindow)
scene = coin.SoSeparator()
cam = coin.SoPerspectiveCamera()
cam.position = (0, 0, 4)
light = coin.SoLightModel()
light.model = coin.SoLightModel.BASE_COLOR
scene += light, cam, coin.SoCone()
viewer = soqt.SoQtRenderArea(myWindow)
viewer.setSceneGraph(scene)
viewer.setTitle("Examiner Viewer")
viewer.show()
soqt.SoQt.show(myWindow)
soqt.SoQt.mainLoop()