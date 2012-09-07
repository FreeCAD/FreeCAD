# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'postprocess.ui'
#
# Created: Mon Aug 20 15:18:09 2012
#      by: PyQt4 UI code generator 4.8.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class Ui_dialog(object):
    def setupUi(self, dialog):
        dialog.setObjectName(_fromUtf8("dialog"))
        dialog.resize(425, 240)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(dialog.sizePolicy().hasHeightForWidth())
        dialog.setSizePolicy(sizePolicy)
        dialog.setMinimumSize(QtCore.QSize(0, 0))
        self.gridLayout_2 = QtGui.QGridLayout(dialog)
        self.gridLayout_2.setObjectName(_fromUtf8("gridLayout_2"))
        self.buttonBox = QtGui.QDialogButtonBox(dialog)
        self.buttonBox.setOrientation(QtCore.Qt.Vertical)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.gridLayout_2.addWidget(self.buttonBox, 0, 3, 1, 1)
        self.button_select_results_folder = QtGui.QPushButton(dialog)
        self.button_select_results_folder.setObjectName(_fromUtf8("button_select_results_folder"))
        self.gridLayout_2.addWidget(self.button_select_results_folder, 3, 0, 1, 1)
        self.button_start_postprocessing = QtGui.QPushButton(dialog)
        self.button_start_postprocessing.setEnabled(False)
        self.button_start_postprocessing.setMinimumSize(QtCore.QSize(0, 23))
        self.button_start_postprocessing.setObjectName(_fromUtf8("button_start_postprocessing"))
        self.gridLayout_2.addWidget(self.button_start_postprocessing, 3, 1, 1, 1)
        self.groupBox_2 = QtGui.QGroupBox(dialog)
        self.groupBox_2.setObjectName(_fromUtf8("groupBox_2"))
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.groupBox_2)
        self.verticalLayout_2.setObjectName(_fromUtf8("verticalLayout_2"))
        self.check_abs_disp_x = QtGui.QRadioButton(self.groupBox_2)
        self.check_abs_disp_x.setObjectName(_fromUtf8("check_abs_disp_x"))
        self.verticalLayout_2.addWidget(self.check_abs_disp_x)
        spacerItem = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout_2.addItem(spacerItem)
        self.check_abs_disp_y = QtGui.QRadioButton(self.groupBox_2)
        self.check_abs_disp_y.setObjectName(_fromUtf8("check_abs_disp_y"))
        self.verticalLayout_2.addWidget(self.check_abs_disp_y)
        spacerItem1 = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout_2.addItem(spacerItem1)
        self.check_abs_disp_z = QtGui.QRadioButton(self.groupBox_2)
        self.check_abs_disp_z.setObjectName(_fromUtf8("check_abs_disp_z"))
        self.verticalLayout_2.addWidget(self.check_abs_disp_z)
        self.gridLayout_2.addWidget(self.groupBox_2, 0, 0, 1, 1)
        self.groupBox_3 = QtGui.QGroupBox(dialog)
        self.groupBox_3.setObjectName(_fromUtf8("groupBox_3"))
        self.verticalLayout_3 = QtGui.QVBoxLayout(self.groupBox_3)
        self.verticalLayout_3.setObjectName(_fromUtf8("verticalLayout_3"))
        self.check_rot_x = QtGui.QRadioButton(self.groupBox_3)
        self.check_rot_x.setObjectName(_fromUtf8("check_rot_x"))
        self.verticalLayout_3.addWidget(self.check_rot_x)
        spacerItem2 = QtGui.QSpacerItem(20, 33, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout_3.addItem(spacerItem2)
        self.check_rot_y = QtGui.QRadioButton(self.groupBox_3)
        self.check_rot_y.setObjectName(_fromUtf8("check_rot_y"))
        self.verticalLayout_3.addWidget(self.check_rot_y)
        spacerItem3 = QtGui.QSpacerItem(20, 34, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout_3.addItem(spacerItem3)
        self.check_rot_z = QtGui.QRadioButton(self.groupBox_3)
        self.check_rot_z.setObjectName(_fromUtf8("check_rot_z"))
        self.verticalLayout_3.addWidget(self.check_rot_z)
        self.gridLayout_2.addWidget(self.groupBox_3, 0, 1, 1, 1)

        self.retranslateUi(dialog)
        QtCore.QMetaObject.connectSlotsByName(dialog)

    def retranslateUi(self, dialog):
        dialog.setWindowTitle(QtGui.QApplication.translate("dialog", "Machining Distortion Prediction", None, QtGui.QApplication.UnicodeUTF8))
        self.button_select_results_folder.setText(QtGui.QApplication.translate("dialog", "Select Results Folder", None, QtGui.QApplication.UnicodeUTF8))
        self.button_start_postprocessing.setText(QtGui.QApplication.translate("dialog", "Start Postprocessing", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox_2.setTitle(QtGui.QApplication.translate("dialog", "Select Z-Axis", None, QtGui.QApplication.UnicodeUTF8))
        self.check_abs_disp_x.setText(QtGui.QApplication.translate("dialog", "Absolute Displacement X", None, QtGui.QApplication.UnicodeUTF8))
        self.check_abs_disp_y.setText(QtGui.QApplication.translate("dialog", "Absolute Displacement Y", None, QtGui.QApplication.UnicodeUTF8))
        self.check_abs_disp_z.setText(QtGui.QApplication.translate("dialog", "Absolute Displacement Z", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox_3.setTitle(QtGui.QApplication.translate("dialog", "Select Y-Axis", None, QtGui.QApplication.UnicodeUTF8))
        self.check_rot_x.setText(QtGui.QApplication.translate("dialog", "Rotation around X-Axis", None, QtGui.QApplication.UnicodeUTF8))
        self.check_rot_y.setText(QtGui.QApplication.translate("dialog", "Rotation around Y-Axis", None, QtGui.QApplication.UnicodeUTF8))
        self.check_rot_z.setText(QtGui.QApplication.translate("dialog", "Rotation around Z-Axis", None, QtGui.QApplication.UnicodeUTF8))

