import sys

from pivy.qt import QtWidgets
from pivy import coin, quarter


def main():
    app = QtWidgets.QApplication(sys.argv)

    root = coin.SoSeparator()
    mat = coin.SoMaterial()
    mat.diffuseColor.setValue(coin.SbColor(0.8, 1, 0.8))
    mat.specularColor.setValue(coin.SbColor(1, 1, 1))
    mat.shininess.setValue(1.0)
    mat.transparency.setValue(0.9)
    root.addChild(mat)
    root.addChild(coin.SoSphere())
    root.addChild(coin.SoCube())

    viewer = quarter.QuarterWidget()
    viewer.setSceneGraph(root)

    viewer.setWindowTitle("minimal")
    viewer.show()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
