# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'lsf_submission_gui.ui'
#
# Created: Wed Aug 22 08:39:15 2012
#      by: PyQt4 UI code generator 4.9.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_DialogSubmitwithLSF(object):
    def setupUi(self, DialogSubmitwithLSF):
        DialogSubmitwithLSF.setObjectName(_fromUtf8("DialogSubmitwithLSF"))
        DialogSubmitwithLSF.setWindowModality(QtCore.Qt.ApplicationModal)
        DialogSubmitwithLSF.resize(566, 185)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(DialogSubmitwithLSF.sizePolicy().hasHeightForWidth())
        DialogSubmitwithLSF.setSizePolicy(sizePolicy)
        self.layoutWidget = QtGui.QWidget(DialogSubmitwithLSF)
        self.layoutWidget.setGeometry(QtCore.QRect(9, 8, 550, 170))
        self.layoutWidget.setObjectName(_fromUtf8("layoutWidget"))
        self.gridLayout = QtGui.QGridLayout(self.layoutWidget)
        self.gridLayout.setMargin(0)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.verticalLayout = QtGui.QVBoxLayout()
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.label = QtGui.QLabel(self.layoutWidget)
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Arial"))
        font.setPointSize(14)
        font.setBold(True)
        font.setWeight(75)
        self.label.setFont(font)
        self.label.setWordWrap(False)
        self.label.setObjectName(_fromUtf8("label"))
        self.horizontalLayout.addWidget(self.label)
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.numbercpus = QtGui.QSpinBox(self.layoutWidget)
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Arial"))
        font.setPointSize(14)
        font.setBold(True)
        font.setWeight(75)
        self.numbercpus.setFont(font)
        self.numbercpus.setMinimum(1)
        self.numbercpus.setMaximum(8)
        self.numbercpus.setObjectName(_fromUtf8("numbercpus"))
        self.horizontalLayout.addWidget(self.numbercpus)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.horizontalLayout_3 = QtGui.QHBoxLayout()
        self.horizontalLayout_3.setObjectName(_fromUtf8("horizontalLayout_3"))
        self.label_3 = QtGui.QLabel(self.layoutWidget)
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Arial"))
        font.setPointSize(14)
        font.setBold(True)
        font.setWeight(75)
        self.label_3.setFont(font)
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.horizontalLayout_3.addWidget(self.label_3)
        self.progressBar = QtGui.QProgressBar(self.layoutWidget)
        self.progressBar.setProperty("value", 24)
        self.progressBar.setObjectName(_fromUtf8("progressBar"))
        self.horizontalLayout_3.addWidget(self.progressBar)
        self.verticalLayout.addLayout(self.horizontalLayout_3)
        self.gridLayout.addLayout(self.verticalLayout, 0, 0, 2, 1)
        self.verticalLayout_3 = QtGui.QVBoxLayout()
        self.verticalLayout_3.setObjectName(_fromUtf8("verticalLayout_3"))
        self.start_calculation = QtGui.QPushButton(self.layoutWidget)
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Arial"))
        font.setPointSize(14)
        font.setBold(True)
        font.setWeight(75)
        self.start_calculation.setFont(font)
        self.start_calculation.setObjectName(_fromUtf8("start_calculation"))
        self.verticalLayout_3.addWidget(self.start_calculation)
        self.close_dialog = QtGui.QPushButton(self.layoutWidget)
        font = QtGui.QFont()
        font.setFamily(_fromUtf8("Arial"))
        font.setPointSize(14)
        font.setBold(True)
        font.setWeight(75)
        self.close_dialog.setFont(font)
        self.close_dialog.setObjectName(_fromUtf8("close_dialog"))
        self.verticalLayout_3.addWidget(self.close_dialog)
        self.gridLayout.addLayout(self.verticalLayout_3, 0, 1, 1, 1)

        self.retranslateUi(DialogSubmitwithLSF)
        QtCore.QMetaObject.connectSlotsByName(DialogSubmitwithLSF)

    def retranslateUi(self, DialogSubmitwithLSF):
        DialogSubmitwithLSF.setWindowTitle(QtGui.QApplication.translate("DialogSubmitwithLSF", "Launch LSF Jobs", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("DialogSubmitwithLSF", "How many CPUs should be used", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("DialogSubmitwithLSF", "Progress", None, QtGui.QApplication.UnicodeUTF8))
        self.start_calculation.setText(QtGui.QApplication.translate("DialogSubmitwithLSF", "Start Calculation", None, QtGui.QApplication.UnicodeUTF8))
        self.close_dialog.setText(QtGui.QApplication.translate("DialogSubmitwithLSF", "Close", None, QtGui.QApplication.UnicodeUTF8))

