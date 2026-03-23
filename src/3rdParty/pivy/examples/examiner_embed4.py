#!/usr/bin/env python

###
# Copyright (c) 2002-2007 Systems in Motion
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

##
# Demonstrates embedding of an SoQtExaminerViewer within a simple widget
# hierarchy.

import sys
from random import random

from pivy.coin import *
from pivy.gui.soqt import *

try:
    from pivy.qt.QtWidgets import *
except ImportError:
    from pivy.qt.QtGui import *
from pivy.qt.QtCore import *

class EmbeddedWindow(QMainWindow):
    def __init__(self, *args):
        super(EmbeddedWindow, self).__init__(*args)

        # dummy widget needed for the PyQt stuff
        self.centralWidget = QWidget(self)
        self.mainLayout = QVBoxLayout(self.centralWidget)
        self.examiner = QWidget(self.centralWidget)
        self.mainLayout.addWidget(self.examiner)
        self.hLayout = QHBoxLayout()
        self.groupBox = QGroupBox("Choose axis", self.centralWidget)
        self.hboxlayout1 = QHBoxLayout(self.groupBox)
        self.buttonGroup = QButtonGroup(self.groupBox)
        self.radio_x = QRadioButton("&X", self.groupBox)
        self.radio_y = QRadioButton("&Y", self.groupBox)
        self.radio_z = QRadioButton("&Z", self.groupBox)
        self.buttonGroup.addButton(self.radio_x, 0)
        self.buttonGroup.addButton(self.radio_y, 1)
        self.buttonGroup.addButton(self.radio_z, 2)
        self.hboxlayout1.addWidget(self.radio_x)
        self.hboxlayout1.addWidget(self.radio_y)
        self.hboxlayout1.addWidget(self.radio_z)
        self.hLayout.addWidget(self.groupBox)

        self.controlLayout = QVBoxLayout()
        self.checkbox = QCheckBox("Enable &rotation", self.centralWidget)
        self.checkbox.setDown(False)
        self.button = QPushButton("&Change cone color", self.centralWidget)
        self.controlLayout.addWidget(self.checkbox)
        self.controlLayout.addWidget(self.button)

        self.hLayout.addLayout(self.controlLayout)
        self.mainLayout.addLayout(self.hLayout)

        self.setCentralWidget(self.centralWidget)

        self.radio_x.setChecked(True)

        # construct a simple scenegraph
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

        # N.B.: keep in mind that it is important to keep the examiner
        # viewer as an instance variable by prefixing it with 'self.'
        # otherwise it will fall out of scope and gets deallocated ->
        # no redraws and crashes. 20050727 tamer.

        # add the examinerviewer
        self.exam = SoQtExaminerViewer(self.examiner)
        self.exam.setSceneGraph(root)
        self.exam.setTitle("Embedded viewer")
        self.exam.show()

        self.connect(self.buttonGroup, SIGNAL("buttonClicked(int)"), self.change_axis)
        self.connect(self.button, SIGNAL("clicked()"), self.change_color)
        self.connect(self.checkbox, SIGNAL("clicked()"), self.rotate)

    def change_axis(self, axis):
        self.rotxyz.axis = axis

    def change_color(self):
        self.material.diffuseColor = (random(), random(), random())

    def rotate(self):
        self.gate.enable = not self.gate.enable.getValue()

def main():
    qApp = QApplication([])
    # initialize Qt and SoQt
    SoQt.init(None)

    # set up scrollview window
    vp = EmbeddedWindow()
    # map window
    vp.resize(640, 480)

    # set termination condition
    QObject.connect(qApp, SIGNAL("lastWindowClosed()"), qApp, SLOT("quit()"))

    # start event loop
    SoQt.mainLoop()

if __name__ == '__main__':
    main()
