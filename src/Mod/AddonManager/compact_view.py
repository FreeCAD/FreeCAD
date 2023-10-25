# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'compact_view.ui'
##
## Created by: Qt User Interface Compiler version 5.15.1
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide.QtCore import *
from PySide.QtGui import *
from PySide.QtWidgets import *


class Ui_CompactView(object):
    def setupUi(self, CompactView):
        if not CompactView.objectName():
            CompactView.setObjectName("CompactView")
        CompactView.resize(489, 16)
        sizePolicy = QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(CompactView.sizePolicy().hasHeightForWidth())
        CompactView.setSizePolicy(sizePolicy)
        self.horizontalLayout_2 = QHBoxLayout(CompactView)
        self.horizontalLayout_2.setObjectName("horizontalLayout_2")
        self.horizontalLayout_2.setSizeConstraint(QLayout.SetNoConstraint)
        self.horizontalLayout_2.setContentsMargins(3, 0, 9, 0)
        self.labelIcon = QLabel(CompactView)
        self.labelIcon.setObjectName("labelIcon")
        sizePolicy1 = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        sizePolicy1.setHorizontalStretch(0)
        sizePolicy1.setVerticalStretch(0)
        sizePolicy1.setHeightForWidth(self.labelIcon.sizePolicy().hasHeightForWidth())
        self.labelIcon.setSizePolicy(sizePolicy1)
        self.labelIcon.setMinimumSize(QSize(16, 16))
        self.labelIcon.setBaseSize(QSize(16, 16))

        self.horizontalLayout_2.addWidget(self.labelIcon)

        self.labelPackageName = QLabel(CompactView)
        self.labelPackageName.setObjectName("labelPackageName")

        self.horizontalLayout_2.addWidget(self.labelPackageName)

        self.labelVersion = QLabel(CompactView)
        self.labelVersion.setObjectName("labelVersion")

        self.horizontalLayout_2.addWidget(self.labelVersion)

        self.labelDescription = QLabel(CompactView)
        self.labelDescription.setObjectName("labelDescription")
        sizePolicy2 = QSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.Preferred)
        sizePolicy2.setHorizontalStretch(0)
        sizePolicy2.setVerticalStretch(0)
        sizePolicy2.setHeightForWidth(self.labelDescription.sizePolicy().hasHeightForWidth())
        self.labelDescription.setSizePolicy(sizePolicy2)
        self.labelDescription.setTextFormat(Qt.PlainText)
        self.labelDescription.setWordWrap(False)
        self.labelDescription.setTextInteractionFlags(Qt.TextSelectableByMouse)

        self.horizontalLayout_2.addWidget(self.labelDescription)

        self.labelStatus = QLabel(CompactView)
        self.labelStatus.setObjectName("labelStatus")

        self.horizontalLayout_2.addWidget(self.labelStatus)

        self.retranslateUi(CompactView)

        QMetaObject.connectSlotsByName(CompactView)

    # setupUi

    def retranslateUi(self, CompactView):
        #        CompactView.setWindowTitle(QCoreApplication.translate("CompactView", "Form", None))
        self.labelIcon.setText(QCoreApplication.translate("CompactView", "Icon", None))
        self.labelPackageName.setText(
            QCoreApplication.translate("CompactView", "<b>Package Name</b>", None)
        )
        self.labelVersion.setText(QCoreApplication.translate("CompactView", "Version", None))
        self.labelDescription.setText(
            QCoreApplication.translate("CompactView", "Description", None)
        )
        self.labelStatus.setText(
            QCoreApplication.translate("CompactView", "Update Available", None)
        )

    # retranslateUi
