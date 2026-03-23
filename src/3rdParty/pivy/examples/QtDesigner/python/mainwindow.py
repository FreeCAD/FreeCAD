#!/usr/bin/env python

from random import random
from pivy.coin import *
from pivy.gui.soqt import SoQtExaminerViewer
from ui_test import Ui_MainWindow
from pivy.qt.QtGui import QWidget
from pivy.qt.QtGui import QMainWindow
from pivy.qt.QtGui import QButtonGroup
from pivy.qt.QtCore import QObject
from pivy.qt.QtCore import SIGNAL

class MainWindow(QMainWindow, Ui_MainWindow):
    def __init__(self, parent = None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)
        self.setupSoQt()
        self.buttonGroup = QButtonGroup(self.groupBox)
        self.buttonGroup.addButton(self.button_x, 0)
        self.buttonGroup.addButton(self.button_y, 1)
        self.buttonGroup.addButton(self.button_z, 2)
        self.connect(self.buttonGroup, SIGNAL("buttonClicked(int)"), self.change_axis)
        self.connect(self.button, SIGNAL("clicked()"), self.change_color)
        self.connect(self.checkbox, SIGNAL("clicked()"), self.rotate)

    def change_axis(self, axis):
        self.rotxyz.axis = axis

    def change_color(self):
        self.material.diffuseColor = (random(), random(), random())

    def rotate(self):
        self.gate.enable = not self.gate.enable.getValue()

    def setupSoQt(self):
        root = SoSeparator()
        self.rotxyz = SoRotationXYZ()
        self.gate = SoGate(SoMFFloat.getClassTypeId())
        self.elapsedTime = SoElapsedTime()
        self.gate.enable = False
        self.gate.input.connectFrom(self.elapsedTime.timeOut)
        self.rotxyz.angle.connectFrom(self.gate.output)        
        self.material = SoMaterial()
        self.material.diffuseColor = (0.0, 1.0, 1.0)
        self.cone = SoCone()
        root.addChild(self.rotxyz)
        root.addChild(self.material)
        root.addChild(self.cone)

        self.exam = SoQtExaminerViewer(self.examiner)
        self.exam.setSceneGraph(root)
        #self.exam.show()
