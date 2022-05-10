# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'expanded_view.ui'
##
## Created by: Qt User Interface Compiler version 5.15.1
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *


class Ui_ExpandedView(object):
    def setupUi(self, ExpandedView):
        if not ExpandedView.objectName():
            ExpandedView.setObjectName(u"ExpandedView")
        ExpandedView.resize(807, 141)
        sizePolicy = QSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(ExpandedView.sizePolicy().hasHeightForWidth())
        ExpandedView.setSizePolicy(sizePolicy)
        self.horizontalLayout_2 = QHBoxLayout(ExpandedView)
        self.horizontalLayout_2.setSpacing(2)
        self.horizontalLayout_2.setObjectName(u"horizontalLayout_2")
        self.horizontalLayout_2.setSizeConstraint(QLayout.SetNoConstraint)
        self.horizontalLayout_2.setContentsMargins(2, 0, 2, 0)
        self.labelIcon = QLabel(ExpandedView)
        self.labelIcon.setObjectName(u"labelIcon")
        sizePolicy1 = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        sizePolicy1.setHorizontalStretch(0)
        sizePolicy1.setVerticalStretch(0)
        sizePolicy1.setHeightForWidth(self.labelIcon.sizePolicy().hasHeightForWidth())
        self.labelIcon.setSizePolicy(sizePolicy1)
        self.labelIcon.setMinimumSize(QSize(48, 48))
        self.labelIcon.setMaximumSize(QSize(48, 48))
        self.labelIcon.setBaseSize(QSize(48, 48))

        self.horizontalLayout_2.addWidget(self.labelIcon)

        self.horizontalSpacer = QSpacerItem(
            8, 20, QSizePolicy.Fixed, QSizePolicy.Minimum
        )

        self.horizontalLayout_2.addItem(self.horizontalSpacer)

        self.verticalLayout = QVBoxLayout()
        self.verticalLayout.setSpacing(3)
        self.verticalLayout.setObjectName(u"verticalLayout")
        self.horizontalLayout = QHBoxLayout()
        self.horizontalLayout.setSpacing(10)
        self.horizontalLayout.setObjectName(u"horizontalLayout")
        self.labelPackageName = QLabel(ExpandedView)
        self.labelPackageName.setObjectName(u"labelPackageName")

        self.horizontalLayout.addWidget(self.labelPackageName)

        self.labelVersion = QLabel(ExpandedView)
        self.labelVersion.setObjectName(u"labelVersion")
        self.labelVersion.setTextFormat(Qt.RichText)
        sizePolicy2 = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
        sizePolicy2.setHorizontalStretch(0)
        sizePolicy2.setVerticalStretch(0)
        sizePolicy2.setHeightForWidth(
            self.labelVersion.sizePolicy().hasHeightForWidth()
        )
        self.labelVersion.setSizePolicy(sizePolicy2)

        self.horizontalLayout.addWidget(self.labelVersion)

        self.labelTags = QLabel(ExpandedView)
        self.labelTags.setObjectName(u"labelTags")

        self.horizontalLayout.addWidget(self.labelTags)

        self.horizontalSpacer_2 = QSpacerItem(
            40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum
        )

        self.horizontalLayout.addItem(self.horizontalSpacer_2)

        self.verticalLayout.addLayout(self.horizontalLayout)

        self.labelDescription = QLabel(ExpandedView)
        self.labelDescription.setObjectName(u"labelDescription")
        sizePolicy.setHeightForWidth(
            self.labelDescription.sizePolicy().hasHeightForWidth()
        )
        self.labelDescription.setSizePolicy(sizePolicy)
        self.labelDescription.setTextFormat(Qt.PlainText)
        self.labelDescription.setAlignment(Qt.AlignLeading | Qt.AlignLeft | Qt.AlignTop)
        self.labelDescription.setWordWrap(True)

        self.verticalLayout.addWidget(self.labelDescription)

        self.labelMaintainer = QLabel(ExpandedView)
        self.labelMaintainer.setObjectName(u"labelMaintainer")
        sizePolicy2.setHeightForWidth(
            self.labelMaintainer.sizePolicy().hasHeightForWidth()
        )
        self.labelMaintainer.setSizePolicy(sizePolicy2)
        self.labelMaintainer.setAlignment(Qt.AlignLeading | Qt.AlignLeft | Qt.AlignTop)
        self.labelMaintainer.setWordWrap(False)

        self.verticalLayout.addWidget(self.labelMaintainer)

        self.horizontalLayout_2.addLayout(self.verticalLayout)

        self.labelStatus = QLabel(ExpandedView)
        self.labelStatus.setObjectName(u"labelStatus")
        self.labelStatus.setTextFormat(Qt.RichText)
        self.labelStatus.setAlignment(
            Qt.AlignRight | Qt.AlignTrailing | Qt.AlignVCenter
        )

        self.horizontalLayout_2.addWidget(self.labelStatus)

        self.retranslateUi(ExpandedView)

        QMetaObject.connectSlotsByName(ExpandedView)

    # setupUi

    def retranslateUi(self, ExpandedView):
        ExpandedView.setWindowTitle(
            QCoreApplication.translate("ExpandedView", u"Form", None)
        )
        self.labelIcon.setText(
            QCoreApplication.translate("ExpandedView", u"Icon", None)
        )
        self.labelPackageName.setText(
            QCoreApplication.translate("ExpandedView", u"<h1>Package Name</h1>", None)
        )
        self.labelVersion.setText(
            QCoreApplication.translate("ExpandedView", u"Version", None)
        )
        self.labelTags.setText(
            QCoreApplication.translate("ExpandedView", u"(tags)", None)
        )
        self.labelDescription.setText(
            QCoreApplication.translate("ExpandedView", u"Description", None)
        )
        self.labelMaintainer.setText(
            QCoreApplication.translate("ExpandedView", u"Maintainer", None)
        )
        self.labelStatus.setText(
            QCoreApplication.translate("ExpandedView", u"UpdateAvailable", None)
        )

    # retranslateUi
