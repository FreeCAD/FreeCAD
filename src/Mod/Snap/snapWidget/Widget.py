# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'Widget.ui'
#
# Created: Sun May 15 18:56:59 2011
#      by: PyQt4 UI code generator 4.7.3
#
# WARNING! All changes made in this file will be lost!

# Qt widgets plugin
from PyQt4 import QtCore, QtGui
# FreeCAD modules
import FreeCAD,FreeCADGui
# FreeCADShip modules
from snapUtils import Paths, Translator

class SnapToolbar_UI(object):
    def setupUi(self, widget):
        """ Setup toolbar user interface
        @param widget External built widget to setup.
        """
        self.rootWidget = widget
        widget.setObjectName("Snap toolbar")
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap(Paths.iconsPath() + "/Ico.xpm"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        widget.setWindowIcon(icon)
        # Add tools
        self.isPoint  = QtGui.QCheckBox(widget)
        self.isCenter  = QtGui.QCheckBox(widget)
        self.isMiddle  = QtGui.QCheckBox(widget)
        self.isIntersection  = QtGui.QCheckBox(widget)
        self.isPerpendicular  = QtGui.QCheckBox(widget)
        self.isNear  = QtGui.QCheckBox(widget)
        self.isGrid  = QtGui.QCheckBox(widget)
        self.isExtension  = QtGui.QCheckBox(widget)
        self.OrthoButton  = QtGui.QPushButton(widget)
        self.EnableButton = QtGui.QPushButton(widget)
        # Set ui locale strings
        self.retranslateUi(widget)
        self.adjustSize(widget)
        self.initialize()
        # Event handling
        QtCore.QObject.connect(self.OrthoButton,QtCore.SIGNAL("pressed()"),self.onOrtho)
        QtCore.QObject.connect(self.EnableButton,QtCore.SIGNAL("pressed()"),self.onEnable)
        QtCore.QMetaObject.connectSlotsByName(widget)

    def retranslateUi(self, widget):
        """ Set locale tool strings
        @param widget External built widget to setup.
        """
        widget.setProperty(QtCore.QString("snapToolWidget"), QtCore.QString("Valid"))
        widget.setWindowTitle(Translator.translate("Snap toolbar"))
        self.isPoint.setText(Translator.translate("Point"))
        self.isCenter.setText(Translator.translate("Center"))
        self.isMiddle.setText(Translator.translate("Middle"))
        self.isIntersection.setText(Translator.translate("Intersection"))
        self.isPerpendicular.setText(Translator.translate("Perpendicular"))
        self.isNear.setText(Translator.translate("Near"))
        self.isGrid.setText(Translator.translate("Grid"))
        self.isExtension.setText(Translator.translate("Extension"))
        self.OrthoButton.setText(Translator.translate("Orthogonal"))
        self.EnableButton.setText(Translator.translate("Enabled"))

    def adjustSize(self, widget):
        """ Adjust toolbar to the minimum size.
        @param widget External built widget to setup.
        """
        # Fit tools size
        self.isPoint.adjustSize()
        self.isCenter.adjustSize()
        self.isMiddle.adjustSize()
        self.isIntersection.adjustSize()
        self.isPerpendicular.adjustSize()
        self.isNear.adjustSize()
        self.isGrid.adjustSize()
        self.isExtension.adjustSize()
        # Position it
        self.isPoint.move(0,24)
        xSize = self.isPoint.size().width()
        ySize = self.isPoint.size().height()
        self.isCenter.move(xSize,24)
        xSize = xSize + self.isCenter.size().width()
        ySize = max(ySize, self.isCenter.size().height())
        self.isMiddle.move(xSize,24)
        xSize = xSize + self.isMiddle.size().width()
        ySize = max(ySize, self.isMiddle.size().height())
        self.isIntersection.move(xSize,24)
        xSize = xSize + self.isIntersection.size().width()
        ySize = max(ySize, self.isIntersection.size().height())
        self.isPerpendicular.move(xSize,24)
        xSize = xSize + self.isPerpendicular.size().width()
        ySize = max(ySize, self.isPerpendicular.size().height())
        self.isNear.move(xSize,24)
        xSize = xSize + self.isNear.size().width()
        ySize = max(ySize, self.isNear.size().height())
        self.isGrid.move(xSize,24)
        xSize = xSize + self.isGrid.size().width()
        ySize = max(ySize, self.isGrid.size().height())
        self.isExtension.move(xSize,24)
        xSize = xSize + self.isExtension.size().width()
        ySize = max(ySize, self.isExtension.size().height())
        # Set buttons geometry
        self.OrthoButton.setGeometry(QtCore.QRect(xSize, 24, 112, ySize))
        xSize = xSize + 112
        self.EnableButton.setGeometry(QtCore.QRect(xSize, 24, 112, ySize))
        xSize = xSize + 112
        # Set window geometry
        ySize = ySize+24
        widget.setMinimumSize(QtCore.QSize(xSize, ySize))
        widget.setMaximumSize(QtCore.QSize(xSize, ySize))


    def initialize(self):
        """ Initial values
        """
        self.isPoint.setChecked(True)
        self.isCenter.setChecked(True)
        self.isMiddle.setChecked(False)
        self.isIntersection.setChecked(True)
        self.isPerpendicular.setChecked(False)
        self.isNear.setChecked(False)
        self.isGrid.setChecked(False)
        self.isExtension.setChecked(False)
        # Set orthogonality as disabled
        self.isOrtho = False
        self.OrthoButton.setStyleSheet('QPushButton {color: red}')
        # Set snap at enabled
        self.isEnable = True
        self.EnableButton.setStyleSheet('QPushButton {color: blue}')

    def onOrtho(self):
        """ Activates/desactivates orthogonality snap.
        """
        self.isOrtho = not self.isOrtho
        if self.isOrtho:
            self.OrthoButton.setStyleSheet('QPushButton {color: blue}')
        else:
            self.OrthoButton.setStyleSheet('QPushButton {color: red}')

    def onEnable(self):
        """ Activates/desactivates snap.
        """
        self.isEnable = not self.isEnable
        if self.isEnable:
            self.EnableButton.setStyleSheet('QPushButton {color: blue}')
            self.EnableButton.setText(Translator.translate("Enabled"))
        else:
            self.EnableButton.setStyleSheet('QPushButton {color: red}')
            self.EnableButton.setText(Translator.translate("Disabled"))

    def enabled(self):
        """ Get if snaping is active.
        @return True if snaping is enabled.
        """
        return self.isEnable

    def constrain(self):
        """ Get if constrain is switched on.
        @return True if constrain is enabled.
        """
        return self.isOrtho

    def point(self):
        """ Get if points/vertexes must be snapped.
        @return True if points/vertexes must be snapped.
        """
        return self.isPoint.isChecked()

    def center(self):
        """ Get if arc center must be snapped.
        @return True if arc center must be snapped.
        """
        return self.isCenter.isChecked()

    def middle(self):
        """ Get if middle points must be snapped.
        @return True if middle points must be snapped.
        """
        return self.isMiddle.isChecked()

    def intersection(self):
        """ Get if intersections must be snapped.
        @return True if intersections must be snapped.
        """
        return self.isIntersection.isChecked()

    def perpendicular(self):
        """ Get if perpendicular extensions must be snapped.
        @return True if perpendicular extension must be snapped.
        """
        return self.isPerpendicular.isChecked()

    def near(self):
        """ Get if objects arbitrary points must be snapped.
        @return True if objects arbitrary points must be snapped.
        """
        return self.isNear.isChecked()

    def grid(self):
        """ Get if grid must be snapped.
        @return True if grid must be snapped.
        """
        return self.isGrid.isChecked()

    def extension(self):
        """ Get if extensions must be snapped.
        @return True if extensions must be snapped.
        """
        return self.isExtension.isChecked()
