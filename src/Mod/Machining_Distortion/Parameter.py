# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'Parameter.ui'
#
# Created: Fri Dec 17 12:28:02 2010
#      by: PyQt4 UI code generator 4.7.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_DialogParameter(object):
    def setupUi(self, DialogParameter):
        DialogParameter.setObjectName("DialogParameter")
        DialogParameter.setWindowModality(QtCore.Qt.ApplicationModal)
        DialogParameter.resize(178, 136)
        self.verticalLayout = QtGui.QVBoxLayout(DialogParameter)
        self.verticalLayout.setObjectName("verticalLayout")
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.label = QtGui.QLabel(DialogParameter)
        self.label.setObjectName("label")
        self.horizontalLayout.addWidget(self.label)
        self.ValueStart = QtGui.QDoubleSpinBox(DialogParameter)
        self.ValueStart.setMinimum(-99.0)
        self.ValueStart.setMaximum(0.0)
        self.ValueStart.setProperty("value", -5.0)
        self.ValueStart.setObjectName("ValueStart")
        self.horizontalLayout.addWidget(self.ValueStart)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.horizontalLayout_2 = QtGui.QHBoxLayout()
        self.horizontalLayout_2.setObjectName("horizontalLayout_2")
        self.label_2 = QtGui.QLabel(DialogParameter)
        self.label_2.setObjectName("label_2")
        self.horizontalLayout_2.addWidget(self.label_2)
        self.ValueEnd = QtGui.QDoubleSpinBox(DialogParameter)
        self.ValueEnd.setProperty("value", 5.0)
        self.ValueEnd.setObjectName("ValueEnd")
        self.horizontalLayout_2.addWidget(self.ValueEnd)
        self.verticalLayout.addLayout(self.horizontalLayout_2)
        self.line = QtGui.QFrame(DialogParameter)
        self.line.setFrameShape(QtGui.QFrame.HLine)
        self.line.setFrameShadow(QtGui.QFrame.Sunken)
        self.line.setObjectName("line")
        self.verticalLayout.addWidget(self.line)
        self.horizontalLayout_3 = QtGui.QHBoxLayout()
        self.horizontalLayout_3.setObjectName("horizontalLayout_3")
        self.label_3 = QtGui.QLabel(DialogParameter)
        self.label_3.setObjectName("label_3")
        self.horizontalLayout_3.addWidget(self.label_3)
        self.ValueSize = QtGui.QDoubleSpinBox(DialogParameter)
        self.ValueSize.setProperty("value", 1.0)
        self.ValueSize.setObjectName("ValueSize")
        self.horizontalLayout_3.addWidget(self.ValueSize)
        self.verticalLayout.addLayout(self.horizontalLayout_3)
        self.buttonBox = QtGui.QDialogButtonBox(DialogParameter)
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.verticalLayout.addWidget(self.buttonBox)

        self.retranslateUi(DialogParameter)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), DialogParameter.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), DialogParameter.reject)
        QtCore.QMetaObject.connectSlotsByName(DialogParameter)

    def retranslateUi(self, DialogParameter):
        DialogParameter.setWindowTitle(QtGui.QApplication.translate("DialogParameter", "Dialog", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("DialogParameter", "Start value:", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("DialogParameter", "End value:", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("DialogParameter", "Step size:", None, QtGui.QApplication.UnicodeUTF8))

