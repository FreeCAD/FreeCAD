# SPDX-License-Identifier: LGPL-2.1-or-later

# -*- coding: utf8 -*-
# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD Draft Workbench - GUI part"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "https://www.freecad.org"

## @package DraftGui
#  \ingroup DRAFT
#  \brief GUI elements and utilities of the Draft workbench
#
#  This module provides GUI tools for the Draft workbench, such as
#  toolbars and task panels, and Qt-dependent utilities such as
#  a delayed (todo) commit system

"""This is the GUI part of the Draft module.
Report to Draft.py for info
"""

import os
import sys
import time
import math
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui
import PySide.QtWidgets as QtWidgets

import FreeCAD
import FreeCADGui
import Draft
import DraftVecUtils
import WorkingPlane
from draftutils import params
from draftutils import utils
from draftutils.todo import todo
from draftutils.translate import translate
from draftutils.units import display_external


def _get_incmd_shortcut(itm):
    return params.get_param("inCommandShortcut" + itm).upper()


# ---------------------------------------------------------------------------
# Customized widgets
# ---------------------------------------------------------------------------


class DraftBaseWidget(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

    def eventFilter(self, widget, event):
        if event.type() == QtCore.QEvent.KeyPress and event.text().upper() == _get_incmd_shortcut(
            "CycleSnap"
        ):
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.cycleSnapObject()
            return True
        return super().eventFilter(widget, event)


class DraftDockWidget(DraftBaseWidget):
    """custom Widget that emits a resized() signal when resized"""

    def __init__(self, parent=None):
        super().__init__(parent)

    def resizeEvent(self, event):
        self.emit(QtCore.SIGNAL("resized()"))

    def changeEvent(self, event):
        if event.type() == QtCore.QEvent.LanguageChange:
            self.emit(QtCore.SIGNAL("retranslate()"))
        else:
            super().changeEvent(event)


class DraftLineEdit(QtWidgets.QLineEdit):
    """custom QLineEdit widget that has the power to catch Escape keypress"""

    def __init__(self, parent=None):
        super().__init__(parent)

    def keyPressEvent(self, event):
        if event.key() == QtCore.Qt.Key_Escape:
            self.emit(QtCore.SIGNAL("escaped()"))
        elif event.key() == QtCore.Qt.Key_Up:
            self.emit(QtCore.SIGNAL("up()"))
        elif event.key() == QtCore.Qt.Key_Down:
            self.emit(QtCore.SIGNAL("down()"))
        else:
            super().keyPressEvent(event)


class DraftTaskPanel:
    def __init__(self, widget, extra=None):
        if extra:
            if isinstance(extra, list):
                self.form = [widget] + extra
            else:
                self.form = [widget, extra]
        else:
            self.form = widget

    def getStandardButtons(self):
        return QtWidgets.QDialogButtonBox.Close

    def accept(self):
        if hasattr(FreeCADGui, "draftToolBar"):
            return FreeCADGui.draftToolBar.validatePoint()
        else:
            if FreeCADGui.ActiveDocument:
                FreeCADGui.ActiveDocument.resetEdit()
            return True

    def reject(self):
        # https://github.com/FreeCAD/FreeCAD/issues/17027
        # Function can be called multiple times if Esc is pressed during mouse
        # move. We need to prevent multiple calls to draftToolBar.escape():
        if not FreeCADGui.draftToolBar.isTaskOn:
            return
        FreeCADGui.draftToolBar.isTaskOn = False
        FreeCADGui.draftToolBar.escape()
        if FreeCADGui.ActiveDocument:
            FreeCADGui.ActiveDocument.resetEdit()
        return True

    def isAllowedAlterDocument(self):
        return False


class DraftToolBar:
    """The Draft Task panel UI
    Draft Toolbar is the main ui of the Draft Module. Once displayed as a
    toolbar, now it define the ui of the Task Panel.
    Toolbar become obsolete due to lack of manteinence and was disabled
    by default in February 2020.
    Draft Ui Commands call and get information such as point coordinates,
    subcommands activation, continue mode, etc. from Task Panel Ui
    """

    def __init__(self):
        self.tray = None
        self.sourceCmd = None
        self.mouse = True
        self.mouse_delay_input_start = time.time()
        self.cancel = None
        self.pointcallback = None

        # OBSOLETE BUT STILL USED BY SOME ADDONS AND MACROS
        self.color = QtGui.QColor(
            utils.rgba_to_argb(params.get_param_view("DefaultShapeLineColor"))
        )
        self.facecolor = QtGui.QColor(
            utils.rgba_to_argb(params.get_param_view("DefaultShapeColor"))
        )
        self.linewidth = params.get_param_view("DefaultShapeLineWidth")
        self.fontsize = params.get_param("textheight")

        self.paramconstr = utils.rgba_to_argb(params.get_param("constructioncolor"))
        self.constrMode = False
        self.continueMode = False
        self.chainedMode = False
        self.relativeMode = True
        self.globalMode = False
        self.state = None
        self.textbuffer = []
        self.crossedViews = []
        self.isTaskOn = False
        self.makeFaceMode = True
        self.mask = None
        self.alock = False
        self.x = 0  # coord of the point as displayed in the task panel (global/local and relative/absolute)
        self.y = 0  # idem
        self.z = 0  # idem
        self.new_point = None  # global point value
        self.last_point = None  # idem
        self.lvalue = 0
        self.pvalue = 90
        self.avalue = 0
        self.angle = None
        self.radius = 0
        self.offset = 0
        self.uiloader = FreeCADGui.UiLoader()
        self.autogroup = None
        self.isCenterPlane = False
        self.input_fields = {
            "xValue": {"value": "x", "unit": "Length"},
            "yValue": {"value": "y", "unit": "Length"},
            "zValue": {"value": "z", "unit": "Length"},
            "lengthValue": {"value": "lvalue", "unit": "Length"},
            "radiusValue": {"value": "radius", "unit": "Length"},
            "angleValue": {"value": "avalue", "unit": "Angle"},
        }

        # add only a dummy widget, since widgets are created on demand
        self.baseWidget = DraftBaseWidget()
        self.tray = FreeCADGui.UiLoader().createWidget("Gui::ToolBar")
        self.tray.setObjectName("Draft tray")
        self.tray.setWindowTitle("Draft Tray")
        self.toptray = self.tray
        self.bottomtray = self.tray
        self.setupTray()
        self.setupStyle()
        mw = FreeCADGui.getMainWindow()
        mw.addToolBar(self.tray)
        self.tray.setParent(mw)
        self.tray.hide()
        self.display_point_active = False  # prevent cyclic processing of point values

    # ---------------------------------------------------------------------------
    # General UI setup
    # ---------------------------------------------------------------------------

    def _pushbutton(
        self, name, layout, hide=True, icon=None, width=None, checkable=False, square=False
    ):
        if square:
            button = QtWidgets.QToolButton(self.baseWidget)
            if width is not None:
                button.setFixedHeight(width)
                button.setFixedWidth(width)
        else:
            button = QtWidgets.QPushButton(self.baseWidget)
        button.setObjectName(name)
        if hide:
            button.hide()
        if icon:
            if icon.endswith(".svg"):
                button.setIcon(QtGui.QIcon(icon))
            else:
                button.setIcon(QtGui.QIcon.fromTheme(icon, QtGui.QIcon(":/icons/" + icon + ".svg")))
        if checkable:
            button.setCheckable(True)
            button.setChecked(False)
        layout.addWidget(button)
        return button

    def _label(self, name, layout, hide=True, wrap=False):
        label = QtWidgets.QLabel(self.baseWidget)
        label.setObjectName(name)
        if wrap:
            label.setWordWrap(True)
        if hide:
            label.hide()
        layout.addWidget(label)
        return label

    def _lineedit(self, name, layout, hide=True, width=None):
        bsize = params.get_param("ToolbarIconSize", path="General") - 2
        lineedit = DraftLineEdit(self.baseWidget)
        lineedit.setObjectName(name)
        if hide:
            lineedit.hide()
        # if not width: width = 800
        # lineedit.setMaximumSize(QtCore.QSize(width,bsize))
        layout.addWidget(lineedit)
        return lineedit

    def _inputfield(self, name, layout, hide=True, width=None):
        inputfield = self.uiloader.createWidget("Gui::InputField")
        inputfield.setObjectName(name)
        if hide:
            inputfield.hide()
        if not width:
            sizePolicy = QtWidgets.QSizePolicy(
                QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Preferred
            )
            inputfield.setSizePolicy(sizePolicy)
            inputfield.setMinimumWidth(110)
        else:
            inputfield.setMaximumWidth(width)
        layout.addWidget(inputfield)
        return inputfield

    def _spinbox(self, name, layout, val=None, vmax=None, hide=True, double=False, size=None):
        if double:
            sbox = QtWidgets.QDoubleSpinBox(self.baseWidget)
            sbox.setDecimals(params.get_param("Decimals", path="Units"))
        else:
            sbox = QtWidgets.QSpinBox(self.baseWidget)
        sbox.setObjectName(name)
        if vmax:
            sbox.setMaximum(vmax)
        if val:
            sbox.setValue(val)
        # if size: sbox.setMaximumSize(QtCore.QSize(size[0],size[1]))
        if hide:
            sbox.hide()
        layout.addWidget(sbox)
        return sbox

    def _checkbox(self, name, layout, checked=True, hide=True):
        chk = QtWidgets.QCheckBox(self.baseWidget)
        chk.setChecked(checked)
        chk.setObjectName(name)
        if hide:
            chk.hide()
        layout.addWidget(chk)
        return chk

    def _combo(self, name, layout, hide=True):
        cb = QtWidgets.QComboBox(self.baseWidget)
        cb.setObjectName(name)
        if hide:
            cb.hide()
        layout.addWidget(cb)

    def setupToolBar(self, task=False):
        """sets the draft toolbar up"""

        # command

        self.promptlabel = self._label("promptlabel", self.layout, hide=task)
        self.cmdlabel = self._label("cmdlabel", self.layout, hide=task)
        boldtxt = QtGui.QFont()
        boldtxt.setBold(True)
        self.cmdlabel.setFont(boldtxt)

        # point

        xl = QtWidgets.QHBoxLayout()
        yl = QtWidgets.QHBoxLayout()
        zl = QtWidgets.QHBoxLayout()
        bl = QtWidgets.QHBoxLayout()
        self.layout.addLayout(xl)
        self.layout.addLayout(yl)
        self.layout.addLayout(zl)
        self.layout.addLayout(bl)
        self.labelx = self._label("labelx", xl)
        self.xValue = self._inputfield("xValue", xl)  # width=60
        self.xValue.installEventFilter(self.baseWidget)
        self.xValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        self.labely = self._label("labely", yl)
        self.yValue = self._inputfield("yValue", yl)
        self.yValue.installEventFilter(
            self.baseWidget
        )  # Required to detect snap cycling in case of Y constraining.
        self.yValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        self.labelz = self._label("labelz", zl)
        self.zValue = self._inputfield("zValue", zl)
        self.zValue.installEventFilter(
            self.baseWidget
        )  # Required to detect snap cycling in case of Z constraining.
        self.zValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        self.pointButton = self._pushbutton("addButton", bl, icon="Draft_AddPoint")

        # text

        self.textValue = QtWidgets.QTextEdit(self.baseWidget)
        self.textValue.setObjectName("textValue")
        self.textValue.setTabChangesFocus(True)
        self.layout.addWidget(self.textValue)
        self.textValue.hide()
        tl = QtWidgets.QHBoxLayout()
        self.layout.addLayout(tl)
        self.textOkButton = self._pushbutton("textButton", tl, icon="button_valid")

        # additional line controls

        ll = QtWidgets.QHBoxLayout()
        al = QtWidgets.QHBoxLayout()
        self.layout.addLayout(ll)
        self.layout.addLayout(al)
        self.labellength = self._label("labellength", ll)
        self.lengthValue = self._inputfield("lengthValue", ll)
        self.lengthValue.installEventFilter(
            self.baseWidget
        )  # Required to detect snap cycling if focusOnLength is True.
        self.lengthValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        self.labelangle = self._label("labelangle", al)
        self.angleLock = self._checkbox("angleLock", al, checked=self.alock)
        self.angleValue = self._inputfield("angleValue", al)
        self.angleValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Angle).UserString)

        # options

        fl = QtWidgets.QHBoxLayout()
        self.layout.addLayout(fl)
        self.numFacesLabel = self._label("numfaceslabel", fl)
        self.numFaces = self._spinbox("numFaces", fl, 3)
        ol = QtWidgets.QHBoxLayout()
        self.layout.addLayout(ol)
        rl = QtWidgets.QHBoxLayout()
        self.layout.addLayout(rl)
        self.labelRadius = self._label("labelRadius", rl)
        self.radiusValue = self._inputfield("radiusValue", rl)
        self.radiusValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        bl = QtWidgets.QHBoxLayout()
        self.layout.addLayout(bl)
        self.undoButton = self._pushbutton("undoButton", bl, icon="Draft_Rotate")
        bl = QtWidgets.QHBoxLayout()
        self.layout.addLayout(bl)
        self.finishButton = self._pushbutton("finishButton", bl, icon="Draft_Finish")
        bl = QtWidgets.QHBoxLayout()
        self.layout.addLayout(bl)
        self.closeButton = self._pushbutton("closeButton", bl, icon="Draft_Lock")
        bl = QtWidgets.QHBoxLayout()
        self.layout.addLayout(bl)
        self.wipeButton = self._pushbutton("wipeButton", bl, icon="Draft_Wipe")
        bl = QtWidgets.QHBoxLayout()
        self.layout.addLayout(bl)
        self.orientWPButton = self._pushbutton("orientWPButton", bl, icon="Draft_SelectPlane")
        bl = QtWidgets.QHBoxLayout()
        self.layout.addLayout(bl)
        self.selectButton = self._pushbutton("selectButton", bl, icon="view-select")

        # update modes from parameters:
        self.relativeMode = params.get_param("RelativeMode")
        self.globalMode = params.get_param("GlobalMode")
        self.makeFaceMode = params.get_param("MakeFaceMode")

        if getattr(FreeCAD, "activeDraftCommand", None) and getattr(
            FreeCAD.activeDraftCommand, "featureName", None
        ):
            self.continueMode = params.get_param(
                FreeCAD.activeDraftCommand.featureName, "Mod/Draft/ContinueMode", silent=True
            )

        self.chainedMode = params.get_param("ChainedMode")

        # Note: The order of the calls to self._checkbox() below controls
        #       the position of the checkboxes in the task panel.

        # update checkboxes with parameters and internal modes:
        self.isRelative = self._checkbox("isRelative", self.layout, checked=self.relativeMode)
        self.isGlobal = self._checkbox("isGlobal", self.layout, checked=self.globalMode)
        self.makeFace = self._checkbox("makeFace", self.layout, checked=self.makeFaceMode)
        self.continueCmd = self._checkbox(
            "continueCmd", self.layout, checked=bool(self.continueMode)
        )
        self.chainedModeCmd = self._checkbox(
            "chainedModeCmd", self.layout, checked=self.chainedMode
        )

        self.chainedModeCmd.setEnabled(
            not (hasattr(self.sourceCmd, "contMode") and self.continueMode)
        )
        self.continueCmd.setEnabled(not (hasattr(self.sourceCmd, "chain") and self.chainedMode))

        # update checkboxes without parameters and without internal modes:
        self.occOffset = self._checkbox("occOffset", self.layout, checked=False)

        # update checkboxes with parameters but without internal modes:
        # self.isCopy is also updated in modUi ("CopyMode") and offsetUi ("OffsetCopyMode")
        self.isCopy = self._checkbox("isCopy", self.layout, checked=params.get_param("CopyMode"))
        self.isSubelementMode = self._checkbox(
            "isSubelementMode", self.layout, checked=params.get_param("SubelementMode")
        )

        # spacer
        spacerItem = QtWidgets.QSpacerItem(
            20, 40, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding
        )
        self.layout.addItem(spacerItem)

        self.xValue.valueChanged.connect(self.changeXValue)
        self.yValue.valueChanged.connect(self.changeYValue)
        self.zValue.valueChanged.connect(self.changeZValue)
        self.lengthValue.valueChanged.connect(self.changeLengthValue)
        self.angleValue.valueChanged.connect(self.changeAngleValue)
        if hasattr(self.angleLock, "checkStateChanged"):  # Qt version >= 6.7.0
            self.angleLock.checkStateChanged.connect(self.toggleAngle)
        else:  # Qt version < 6.7.0
            self.angleLock.stateChanged.connect(self.toggleAngle)
        self.radiusValue.valueChanged.connect(self.changeRadiusValue)
        self.xValue.returnPressed.connect(self.checkx)
        self.yValue.returnPressed.connect(self.checky)
        self.lengthValue.returnPressed.connect(self.checklength)
        self.xValue.textEdited.connect(self.checkSpecialChars)
        self.yValue.textEdited.connect(self.checkSpecialChars)
        self.zValue.textEdited.connect(self.checkSpecialChars)
        self.lengthValue.textEdited.connect(self.checkSpecialChars)
        self.radiusValue.textEdited.connect(self.checkSpecialChars)
        self.angleValue.textEdited.connect(self.checkSpecialChars)
        self.zValue.returnPressed.connect(self.validatePoint)
        self.pointButton.clicked.connect(self.validatePoint)
        self.radiusValue.returnPressed.connect(self.validatePoint)
        self.angleValue.returnPressed.connect(self.validatePoint)
        self.textValue.textChanged.connect(self.checkEnterText)
        self.textOkButton.clicked.connect(self.sendText)
        self.zValue.returnPressed.connect(self.setFocus)

        self.finishButton.clicked.connect(self.finish)
        self.closeButton.clicked.connect(self.closeLine)
        self.wipeButton.clicked.connect(self.wipeLine)
        self.orientWPButton.clicked.connect(self.orientWP)
        self.undoButton.clicked.connect(self.undoSegment)
        self.selectButton.clicked.connect(self.selectEdge)
        if hasattr(self.continueCmd, "checkStateChanged"):  # Qt version >= 6.7.0
            self.continueCmd.checkStateChanged.connect(self.setContinue)
            self.chainedModeCmd.checkStateChanged.connect(self.setChainedMode)
            self.isCopy.checkStateChanged.connect(self.setCopymode)
            self.isSubelementMode.checkStateChanged.connect(self.setSubelementMode)
            self.isRelative.checkStateChanged.connect(self.setRelative)
            self.isGlobal.checkStateChanged.connect(self.setGlobal)
            self.makeFace.checkStateChanged.connect(self.setMakeFace)
        else:  # Qt version < 6.7.0
            self.continueCmd.stateChanged.connect(self.setContinue)
            self.chainedModeCmd.stateChanged.connect(self.setChainedMode)
            self.isCopy.stateChanged.connect(self.setCopymode)
            self.isSubelementMode.stateChanged.connect(self.setSubelementMode)
            self.isRelative.stateChanged.connect(self.setRelative)
            self.isGlobal.stateChanged.connect(self.setGlobal)
            self.makeFace.stateChanged.connect(self.setMakeFace)

    def setupTray(self):
        """sets draft tray buttons up"""

        self.wplabel = self._pushbutton(
            "wplabel", self.toptray, icon="Draft_SelectPlane", hide=False, width=120
        )

        self.styleButton = self._pushbutton(
            "stylebutton", self.toptray, icon="Draft_Apply", hide=False, width=120
        )
        self.setStyleButton()
        self.constrButton = self._pushbutton(
            "constrButton",
            self.toptray,
            hide=False,
            icon="Draft_Construction",
            width=self.styleButton.sizeHint().height(),
            checkable=True,
            square=True,
        )
        self.constrColor = QtGui.QColor(self.paramconstr)
        self.autoGroupButton = self._pushbutton(
            "autoGroup", self.bottomtray, icon=":/icons/button_invalid.svg", hide=False, width=120
        )
        self.autoGroupButton.setText(translate("draft", "None"))
        self.autoGroupButton.setFlat(True)

        self.wplabel.clicked.connect(self.selectplane)
        self.styleButton.clicked.connect(self.setstyle)
        self.constrButton.toggled.connect(self.toggleConstrMode)
        self.autoGroupButton.clicked.connect(self.runAutoGroup)

        QtCore.QTimer.singleShot(
            2000, self.retranslateTray
        )  # delay so translations get a chance to load

    def setupStyle(self):
        style = "#constrButton:Checked {background-color: "
        style += self.getDefaultColor("constr", rgb=True) + " } "
        style += "#addButton:Checked, #delButton:checked, "
        style += "#sharpButton:Checked, "
        style += "#tangentButton:Checked, #symmetricButton:checked {"
        style += "background-color: rgb(20,100,250) }"
        self.baseWidget.setStyleSheet(style)
        # if hasattr(self,"tray"):
        #    self.tray.setStyleSheet(style)

    # ---------------------------------------------------------------------------
    # language tools
    # ---------------------------------------------------------------------------

    def retranslateUi(self, widget=None):
        self.promptlabel.setText(translate("draft", "active command:"))
        self.cmdlabel.setText(translate("draft", "None"))
        self.cmdlabel.setToolTip(translate("draft", "Active Draft command"))
        self.xValue.setToolTip(translate("draft", "X coordinate of the point"))
        self.labelx.setText(translate("draft", "X"))
        self.labely.setText(translate("draft", "Y"))
        self.labelz.setText(translate("draft", "Z"))
        self.yValue.setToolTip(translate("draft", "Y coordinate of the point"))
        self.zValue.setToolTip(translate("draft", "Z coordinate of the point"))
        self.pointButton.setText(translate("draft", "Enter Point"))
        self.pointButton.setToolTip(translate("draft", "Enter a point with given coordinates"))
        self.labellength.setText(translate("draft", "Length"))
        self.labelangle.setText(translate("draft", "Angle"))
        self.lengthValue.setToolTip(translate("draft", "Length of the current segment"))
        self.angleValue.setToolTip(translate("draft", "Angle of the current segment"))
        self.angleLock.setToolTip(
            translate("draft", "Locks the current angle")
            + " ("
            + _get_incmd_shortcut("Length")
            + ")"
        )
        self.labelRadius.setText(translate("draft", "Radius"))
        self.radiusValue.setToolTip(translate("draft", "Radius of the circle"))
        self.isRelative.setText(
            translate("draft", "Relative") + " (" + _get_incmd_shortcut("Relative") + ")"
        )
        self.isRelative.setToolTip(
            translate(
                "draft",
                "Coordinates relative to last point or to coordinate system "
                + "origin\nif is the first point to set",
            )
        )
        self.isGlobal.setText(
            translate("draft", "Global") + " (" + _get_incmd_shortcut("Global") + ")"
        )
        self.isGlobal.setToolTip(
            translate(
                "draft",
                "Coordinates relative to global coordinate system."
                + "\nUncheck to use working plane coordinate system",
            )
        )
        self.makeFace.setText(
            translate("draft", "Make face") + " (" + _get_incmd_shortcut("MakeFace") + ")"
        )
        self.makeFace.setToolTip(
            translate(
                "draft",
                "If checked, the object will be filled with a face."
                + "\nNot available if the 'Use Part Primitives' preference is enabled",
            )
        )
        self.finishButton.setText(
            translate("draft", "Finish") + " (" + _get_incmd_shortcut("Exit") + ")"
        )
        self.finishButton.setToolTip(
            translate("draft", "Finishes the current drawing or editing operation")
        )
        self.continueCmd.setText(
            translate("draft", "Continue") + " (" + _get_incmd_shortcut("Continue") + ")"
        )
        self.continueCmd.setToolTip(
            translate(
                "draft",
                "If checked, the command will not finish until pressing "
                + "the command button again",
            )
        )
        self.chainedModeCmd.setText(translate("draft", "Chained mode"))
        self.chainedModeCmd.setToolTip(
            translate(
                "draft",
                "If checked, the next dimension will be placed in a chain"
                " with the previously placed Dimension",
            )
        )
        self.occOffset.setText(translate("draft", "OCC-style offset"))
        self.occOffset.setToolTip(
            translate(
                "draft",
                "If checked, an OCC-style offset will be performed"
                + " instead of the classic offset",
            )
        )
        self.undoButton.setText(
            translate("draft", "Undo") + " (" + _get_incmd_shortcut("Undo") + ")"
        )
        self.undoButton.setToolTip(translate("draft", "Undo the last segment"))
        self.closeButton.setText(
            translate("draft", "Close") + " (" + _get_incmd_shortcut("Close") + ")"
        )
        self.closeButton.setToolTip(translate("draft", "Finishes and closes the current line"))
        self.wipeButton.setText(
            translate("draft", "Wipe") + " (" + _get_incmd_shortcut("Wipe") + ")"
        )
        self.wipeButton.setToolTip(
            translate(
                "draft",
                "Wipes the existing segments of this line and starts again from the last point",
            )
        )
        self.orientWPButton.setText(
            translate("draft", "Set Working Plane") + " (" + _get_incmd_shortcut("SetWP") + ")"
        )
        self.orientWPButton.setToolTip(
            translate("draft", "Reorients the working plane on the last segment")
        )
        self.selectButton.setText(
            translate("draft", "Select Edge") + " (" + _get_incmd_shortcut("SelectEdge") + ")"
        )
        self.selectButton.setToolTip(
            translate("draft", "Selects an existing edge to be measured by this dimension")
        )
        self.numFacesLabel.setText(translate("draft", "Sides"))
        self.numFaces.setToolTip(translate("draft", "Number of sides"))

        self.isCopy.setText(translate("draft", "Copy") + " (" + _get_incmd_shortcut("Copy") + ")")
        self.isCopy.setToolTip(
            translate("draft", "If checked, objects will be copied instead of moved")
        )
        self.isSubelementMode.setText(
            translate("draft", "Modify subelements")
            + " ("
            + _get_incmd_shortcut("SubelementMode")
            + ")"
        )
        self.isSubelementMode.setToolTip(
            translate("draft", "If checked, subelements will be modified instead of entire objects")
        )
        self.textOkButton.setText(translate("draft", "Create Text"))
        self.textOkButton.setToolTip(
            translate("draft", "Creates the text object and finishes the command")
        )
        self.retranslateTray(widget)

        # Update the maximum width of the push buttons
        maxwidth = 66  # that's the default
        pb = []
        for i in range(self.layout.count()):
            w = self.layout.itemAt(i).widget()
            if w is not None and w.inherits("QPushButton"):
                pb.append(w)

        for i in pb:
            fm = QtGui.QFontMetrics(i.font())
            fw = fm.width(i.text())
            fw = max(fw, maxwidth)

        maxwidth = maxwidth + 16 + 10  # add icon width and a margin
        for i in pb:
            i.setMaximumWidth(maxwidth)

    def retranslateTray(self, widget=None):

        self.styleButton.setToolTip(translate("draft", "Changes the default style for new objects"))
        self.constrButton.setToolTip(translate("draft", "Toggles construction mode"))
        self.autoGroupButton.setToolTip(translate("draft", "Autogroup off"))

    # ---------------------------------------------------------------------------
    # Interface modes
    # ---------------------------------------------------------------------------

    def _show_dialog(self, panel):
        task = FreeCADGui.Control.showDialog(panel)
        task.setDocumentName(FreeCADGui.ActiveDocument.Document.Name)
        task.setAutoCloseOnDeletedDocument(True)

    def taskUi(self, title="Draft", extra=None, icon="Draft_Draft"):
        # reset InputField values
        self.reset_ui_values()
        self.isTaskOn = True
        todo.delay(FreeCADGui.Control.closeDialog, None)
        self.baseWidget = DraftBaseWidget()
        self.layout = QtWidgets.QVBoxLayout(self.baseWidget)
        self.setupToolBar(task=True)
        self.retranslateUi(self.baseWidget)
        self.panel = DraftTaskPanel(self.baseWidget, extra)
        todo.delay(self._show_dialog, self.panel)
        self.setTitle(title, icon)

    def redraw(self):
        """utility function that is performed after each clicked point"""
        self.checkLocal()

    def setFocus(self, f=None):

        # Do not set focus on Length if length+angle input is problematic:
        force_xyz = False
        if f in ("x", "y", "z"):
            if not self.globalMode:
                if f == "z":
                    force_xyz = True
            else:
                axis = WorkingPlane.get_working_plane(update=False).axis
                constraint_dir = FreeCAD.Vector(
                    1 if f == "x" else 0, 1 if f == "y" else 0, 1 if f == "z" else 0
                )
                # Using a high tolerance:
                if axis.isEqual(constraint_dir, 0.1) or axis.isEqual(-constraint_dir, 0.1):
                    force_xyz = True

        if not force_xyz and params.get_param("focusOnLength") and self.lengthValue.isVisible():
            self.lengthValue.setFocus()
            self.lengthValue.setSelection(0, self.number_length(self.lengthValue.text()))
        elif not force_xyz and self.angleLock.isVisible() and self.angleLock.isChecked():
            self.lengthValue.setFocus()
            self.lengthValue.setSelection(0, self.number_length(self.lengthValue.text()))
        elif f == "x":
            self.xValue.setFocus()
            self.xValue.setSelection(0, self.number_length(self.xValue.text()))
        elif f == "y":
            self.yValue.setFocus()
            self.yValue.setSelection(0, self.number_length(self.yValue.text()))
        elif f == "z":
            self.zValue.setFocus()
            self.zValue.setSelection(0, self.number_length(self.zValue.text()))
        elif f == "radius":
            self.radiusValue.setFocus()
            self.radiusValue.setSelection(0, self.number_length(self.radiusValue.text()))
        else:
            # f is None
            self.xValue.setFocus()
            self.xValue.setSelection(0, self.number_length(self.xValue.text()))

    def number_length(self, st):
        nl = len(st)
        for char in st[::-1]:
            if char in "0123456789.,-+/":
                break
            nl -= 1
        return nl

    def extraLineUi(self):
        """shows length and angle controls"""
        self.labellength.show()
        self.lengthValue.show()
        self.labelangle.show()
        self.angleValue.show()
        self.angleLock.show()
        self.angleLock.setChecked(False)

    def hideXYZ(self):
        """turn off all the point entry widgets"""
        self.labelx.hide()
        self.labely.hide()
        self.labelz.hide()
        self.labellength.hide()
        self.labelangle.hide()
        self.xValue.hide()
        self.yValue.hide()
        self.zValue.hide()
        self.pointButton.hide()
        self.lengthValue.hide()
        self.angleValue.hide()
        self.angleLock.hide()
        self.isRelative.hide()
        self.isGlobal.hide()

    def lineUi(
        self,
        title=translate("draft", "Line"),
        cancel=None,
        extra=None,
        getcoords=None,
        rel=False,
        icon="Draft_Line",
    ):
        self.pointUi(title, cancel, extra, getcoords, rel, icon)
        self.extraLineUi()
        self.xValue.setEnabled(True)
        self.yValue.setEnabled(True)
        self.continueCmd.show()

    def wireUi(
        self,
        title=translate("draft", "DWire"),
        cancel=None,
        extra=None,
        getcoords=None,
        rel=False,
        icon="Draft_Wire",
    ):
        self.pointUi(title, cancel, extra, getcoords, rel, icon)
        self.xValue.setEnabled(True)
        self.yValue.setEnabled(True)
        if params.get_param("UsePartPrimitives"):
            self.makeFace.setEnabled(False)
        else:
            self.makeFace.setEnabled(True)
        self.makeFace.show()
        self.finishButton.show()
        self.closeButton.show()
        self.wipeButton.show()
        self.orientWPButton.show()
        self.undoButton.show()
        self.continueCmd.show()

    def circleUi(self):
        self.pointUi(translate("draft", "Circle"), icon="Draft_Circle")
        self.extUi()
        self.isRelative.hide()

    def arcUi(self):
        self.pointUi(translate("draft", "Arc"), icon="Draft_Arc")
        self.continueCmd.show()
        self.isRelative.hide()

    def rotateSetCenterUi(self):
        self.pointUi(translate("draft", "Rotate"), icon="Draft_Rotate")
        self.modUi()
        self.isRelative.hide()

    def pointUi(
        self,
        title=translate("draft", "Point"),
        cancel=None,
        extra=None,
        getcoords=None,
        rel=False,
        icon="Draft_Draft",
    ):
        if cancel:
            self.cancel = cancel
        if getcoords:
            self.pointcallback = getcoords
        self.taskUi(title, extra, icon)
        self.xValue.setEnabled(True)
        self.yValue.setEnabled(True)
        self.isRelative.show()
        self.isGlobal.show()
        self.checkLocal()
        self.labelx.show()
        self.labely.show()
        self.labelz.show()
        self.xValue.show()
        self.yValue.show()
        self.zValue.show()
        # reset UI to (0,0,0) on start
        self.xValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        self.yValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        self.zValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        self.x = 0
        self.y = 0
        self.z = 0
        self.new_point = None
        self.last_point = None
        self.pointButton.show()
        if rel:
            self.isRelative.show()
        todo.delay(self.setFocus, None)

    def labelUi(self, title=translate("draft", "Label"), callback=None):
        w = QtWidgets.QWidget()
        w.setWindowTitle(translate("draft", "Label Type"))
        l = QtWidgets.QVBoxLayout(w)
        combo = QtWidgets.QComboBox()
        from draftobjects.label import get_label_types

        types = get_label_types()
        for s in types:
            combo.addItem(translate("Draft", s), userData=s)
        combo.setCurrentIndex(types.index(params.get_param("labeltype")))
        l.addWidget(combo)
        combo.currentIndexChanged.connect(callback)
        self.pointUi(title=title, extra=w, icon="Draft_Label")

    def extraUi(self):
        pass

    def offsetUi(self):
        self.taskUi(translate("draft", "Offset"), icon="Draft_Offset")
        self.radiusUi()
        self.isCopy.show()
        self.isCopy.setChecked(params.get_param("OffsetCopyMode"))
        self.occOffset.show()
        self.labelRadius.setText(translate("draft", "Distance"))
        self.radiusValue.setToolTip(translate("draft", "Offset distance"))
        self.radiusValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        todo.delay(self.setFocus, "radius")

    def offUi(self):
        todo.delay(FreeCADGui.Control.closeDialog, None)
        self.cancel = None
        self.sourceCmd = None
        self.pointcallback = None
        self.mask = None
        self.isTaskOn = False
        self.baseWidget = QtWidgets.QWidget()

    def trimUi(self, title=translate("draft", "Trimex")):
        self.taskUi(title, icon="Draft_Trimex")
        self.radiusUi()
        self.labelRadius.setText(translate("draft", "Distance"))
        self.radiusValue.setToolTip(translate("draft", "Offset distance"))
        self.radiusValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        todo.delay(self.setFocus, "radius")

    def radiusUi(self):
        self.hideXYZ()
        self.labelRadius.setText(translate("draft", "Radius"))
        self.radiusValue.setToolTip(translate("draft", "Radius of Circle"))
        self.labelRadius.show()
        self.radiusValue.setText(FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).UserString)
        self.radiusValue.show()
        todo.delay(self.setFocus, "radius")

    def textUi(self):
        self.hideXYZ()
        self.textValue.show()
        self.textOkButton.show()
        self.textValue.setText("")
        todo.delay(self.textValue.setFocus, None)
        self.textbuffer = []
        self.textline = 0
        self.continueCmd.show()
        # Change the checkbox label as the in-command shortcut cannot be used:
        self.continueCmd.setText(translate("draft", "Continue"))

    def switchUi(self, store=True):
        if store:
            self.state = []
            self.state.append(self.labelx.isVisible())
            self.state.append(self.labely.isVisible())
            self.state.append(self.labelz.isVisible())
            self.state.append(self.xValue.isVisible())
            self.state.append(self.yValue.isVisible())
            self.state.append(self.zValue.isVisible())
            self.hideXYZ()
        else:
            if self.state:
                if self.state[0]:
                    self.labelx.show()
                if self.state[1]:
                    self.labely.show()
                if self.state[2]:
                    self.labelz.show()
                if self.state[3]:
                    self.xValue.show()
                if self.state[4]:
                    self.yValue.show()
                if self.state[5]:
                    self.zValue.show()
                self.state = None

    def setTitle(self, title, icon="Draft_Draft"):
        self.baseWidget.setWindowTitle(title)
        self.baseWidget.setWindowIcon(QtGui.QIcon(":/icons/" + icon + ".svg"))

    def selectUi(self, extra=None, on_close_call=None):
        self.makeDumbTask(extra, on_close_call)

    def editUi(self):
        self.makeDumbTask(on_close_call=self.finish)

    def extUi(self):
        if params.get_param("UsePartPrimitives"):
            self.makeFace.setEnabled(False)
        else:
            self.makeFace.setEnabled(True)
        self.makeFace.show()
        self.continueCmd.show()

    def modUi(self):
        self.isCopy.show()
        self.isSubelementMode.show()
        self.isCopy.setChecked(params.get_param("CopyMode"))
        self.continueCmd.show()

    def checkLocal(self):
        """checks if x,y,z coords must be displayed as local or global"""
        if not self.globalMode and self.relativeMode:
            self.labelx.setText(
                translate("draft", "Local {}").format("\u0394X")
            )  # \u0394 = ∆ (Greek delta)
            self.labely.setText(translate("draft", "Local {}").format("\u0394Y"))
            self.labelz.setText(translate("draft", "Local {}").format("\u0394Z"))
        elif not self.globalMode and not self.relativeMode:
            self.labelx.setText(translate("draft", "Local {}").format("X"))
            self.labely.setText(translate("draft", "Local {}").format("Y"))
            self.labelz.setText(translate("draft", "Local {}").format("Z"))
        elif self.globalMode and self.relativeMode:
            self.labelx.setText(translate("draft", "Global {}").format("\u0394X"))
            self.labely.setText(translate("draft", "Global {}").format("\u0394Y"))
            self.labelz.setText(translate("draft", "Global {}").format("\u0394Z"))
        else:
            self.labelx.setText(translate("draft", "Global {}").format("X"))
            self.labely.setText(translate("draft", "Global {}").format("Y"))
            self.labelz.setText(translate("draft", "Global {}").format("Z"))

    def setNextFocus(self):
        def isThere(widget):
            if widget.isEnabled() and widget.isVisible():
                return True
            else:
                return False

        if self.isTaskOn:
            if isThere(self.xValue):
                self.setFocus()
            elif isThere(self.yValue):
                self.yValue.setFocus()
                self.yValue.selectAll()
            elif isThere(self.zValue):
                self.zValue.setFocus()
                self.zValue.selectAll()
            elif isThere(self.radiusValue):
                self.radiusValue.setFocus()
                self.radiusValue.selectAll()

    def makeDumbTask(self, extra=None, on_close_call=None):
        """create a dumb taskdialog to prevent deleting the temp object"""

        class TaskPanel:
            def __init__(self, extra=None, callback=None):
                if extra:
                    self.form = [extra]
                self.callback = callback

            def getStandardButtons(self):
                return QtWidgets.QDialogButtonBox.Close

            def reject(self):
                if self.callback:
                    self.callback()
                return True

        todo.delay(FreeCADGui.Control.closeDialog, None)
        panel = TaskPanel(extra, on_close_call)
        todo.delay(self._show_dialog, panel)

    # ---------------------------------------------------------------------------
    # Processing functions
    # ---------------------------------------------------------------------------

    def setContinue(self, val):
        params.set_param(
            FreeCAD.activeDraftCommand.featureName,
            bool(getattr(val, "value", val)),
            "Mod/Draft/ContinueMode",
        )
        self.continueMode = bool(getattr(val, "value", val))
        self.chainedModeCmd.setEnabled(not bool(getattr(val, "value", val)))

    def setChainedMode(self, val):
        params.set_param("ChainedMode", bool(getattr(val, "value", val)))
        self.chainedMode = bool(getattr(val, "value", val))
        self.continueCmd.setEnabled(not bool(getattr(val, "value", val)))
        if bool(getattr(val, "value", val)) == False:
            # If user has deselected the checkbox, reactive the command
            # which will result in closing it
            FreeCAD.activeDraftCommand.Activated()

    # val=-1 is used to temporarily switch to relativeMode and disable the checkbox.
    # val=-2 is used to switch back.
    # Used by:
    #     gui_ellipses.py
    #     gui_rectangles.py
    #     gui_stretch.py
    def setRelative(self, val=-1):
        val = getattr(val, "value", val)
        if val < 0:
            if hasattr(self.isRelative, "checkStateChanged"):  # Qt version >= 6.7.0
                self.isRelative.checkStateChanged.disconnect(self.setRelative)
            else:  # Qt version < 6.7.0
                self.isRelative.stateChanged.disconnect(self.setRelative)
            if val == -1:
                self.isRelative.setChecked(True)
                self.relativeMode = True
            elif val == -2:
                val = params.get_param("RelativeMode")
                self.isRelative.setChecked(val)
                self.relativeMode = val
            if hasattr(self.isRelative, "checkStateChanged"):  # Qt version >= 6.7.0
                self.isRelative.checkStateChanged.connect(self.setRelative)
            else:  # Qt version < 6.7.0
                self.isRelative.stateChanged.connect(self.setRelative)
        else:
            params.set_param("RelativeMode", bool(val))
            self.relativeMode = bool(val)
        self.checkLocal()
        self.displayPoint(self.new_point, self.get_last_point())
        self.updateSnapper()

    def setGlobal(self, val):
        params.set_param("GlobalMode", bool(getattr(val, "value", val)))
        self.globalMode = bool(getattr(val, "value", val))
        self.checkLocal()
        self.displayPoint(self.new_point, self.get_last_point())
        self.updateSnapper()

    def setMakeFace(self, val):
        params.set_param("MakeFaceMode", bool(getattr(val, "value", val)))
        self.makeFaceMode = bool(getattr(val, "value", val))

    def setCopymode(self, val):
        # special value for offset command
        if self.sourceCmd and self.sourceCmd.featureName == "Offset":
            params.set_param("OffsetCopyMode", bool(getattr(val, "value", val)))
        else:
            params.set_param("CopyMode", bool(getattr(val, "value", val)))
            # if CopyMode is changed ghosts must be updated.
            # Moveable children should not be included if CopyMode is True.
            self.sourceCmd.set_ghosts()

    def setSubelementMode(self, val):
        params.set_param("SubelementMode", bool(getattr(val, "value", val)))
        self.sourceCmd.set_ghosts()

    def checkx(self):
        if self.yValue.isEnabled():
            self.yValue.setFocus()
            self.yValue.setSelection(0, self.number_length(self.yValue.text()))
            self.updateSnapper()
        else:
            self.checky()

    def checky(self):
        if self.zValue.isEnabled():
            self.zValue.setFocus()
            self.zValue.setSelection(0, self.number_length(self.zValue.text()))
            self.updateSnapper()
        else:
            self.validatePoint()

    def checklength(self):
        if self.angleValue.isEnabled():
            self.angleValue.setFocus()
            self.angleValue.setSelection(0, self.number_length(self.angleValue.text()))
            self.updateSnapper()
        else:
            self.validatePoint()

    def validatePoint(self):
        """function for checking and sending numbers entered manually"""
        self.mouse = True
        self.mouse_delay_input_start = time.time()
        if self.sourceCmd or self.pointcallback:
            if self.labelRadius.isVisible():
                try:
                    rad = self.radius
                except (ValueError, AttributeError):
                    print("debug: DraftGui.validatePoint: AttributeError")
                else:
                    self.sourceCmd.numericRadius(rad)
            elif self.labelx.isVisible():
                try:
                    numx = self.x
                    numy = self.y
                    numz = self.z
                except (ValueError, AttributeError):
                    print("debug: DraftGui.validatePoint: AttributeError")
                else:
                    delta = FreeCAD.Vector(numx, numy, numz)
                    if self.pointcallback:
                        self.pointcallback(delta, self.globalMode, self.relativeMode)
                    else:
                        self.new_point = self.get_new_point(delta)
                        self.sourceCmd.numericInput(*self.new_point)
            elif self.textValue.isVisible():
                return False
            else:
                FreeCADGui.ActiveDocument.resetEdit()
        return True

    def finish(self, cont=None):
        """finish button action"""
        if self.sourceCmd:
            if cont is None:
                cont = self.continueMode
            self.sourceCmd.finish(cont=cont)
        if self.cancel:
            self.cancel()
            self.cancel = None
        if FreeCADGui.ActiveDocument:
            FreeCADGui.ActiveDocument.resetEdit()

    def escape(self):
        """escapes the current command"""
        self.finish(cont=False)

    def closeLine(self):
        """close button action"""
        self.sourceCmd.finish(cont=self.continueMode, closed=True)
        FreeCADGui.ActiveDocument.resetEdit()

    def wipeLine(self):
        """wipes existing segments of a line"""
        self.sourceCmd.wipe()

    def orientWP(self):
        """reorients the current working plane"""
        self.sourceCmd.orientWP()

    def selectEdge(self):
        """allows the dimension command to select an edge"""
        if hasattr(self.sourceCmd, "selectEdge"):
            self.sourceCmd.selectEdge()

    def undoSegment(self):
        """undo last line segment"""
        if hasattr(self.sourceCmd, "undolast"):
            self.sourceCmd.undolast()

    def checkSpecialChars(self, txt):
        """checks for special characters in the entered coords that must be
        treated as shortcuts
        """

        if txt == "":
            self.updateSnapper()
            return

        if txt[0] in "0123456789.,-":
            self.updateSnapper()
            self.setMouseMode(mode=False)
            return

        txt = txt[0].upper()
        spec = False
        self.last_point = self.get_last_point()
        # Most frequently used shortcuts first:
        if txt == _get_incmd_shortcut("Relative"):
            if self.isRelative.isVisible():
                self.isRelative.setChecked(not self.isRelative.isChecked())
                # setRelative takes care of rest
            spec = True
        elif txt == _get_incmd_shortcut("Global"):
            if self.isGlobal.isVisible():
                self.isGlobal.setChecked(not self.isGlobal.isChecked())
                # setGlobal takes care of rest
            spec = True
        elif txt == _get_incmd_shortcut("Length"):
            if self.lengthValue.isVisible():
                self.constrain("angle")
            self.displayPoint(self.new_point, self.last_point)
            spec = True
        elif txt == _get_incmd_shortcut("RestrictX"):
            self.constrain("x")
            self.displayPoint(self.new_point, self.last_point)
            spec = True
        elif txt == _get_incmd_shortcut("RestrictY"):
            self.constrain("y")
            self.displayPoint(self.new_point, self.last_point)
            spec = True
        elif txt == _get_incmd_shortcut("RestrictZ"):
            self.constrain("z")
            self.displayPoint(self.new_point, self.last_point)
            spec = True
        elif txt == _get_incmd_shortcut("Copy"):
            if self.isCopy.isVisible():
                self.isCopy.setChecked(not self.isCopy.isChecked())
            spec = True
        elif txt == _get_incmd_shortcut("Exit"):
            if self.finishButton.isVisible():
                self.finish()
        elif txt == _get_incmd_shortcut("Close"):
            if self.closeButton.isVisible():
                self.closeLine()
        elif txt == _get_incmd_shortcut("AddHold"):
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.addHoldPoint()
            spec = True
        elif txt == _get_incmd_shortcut("Recenter"):
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.recenter_workingplane()
            spec = True
        elif txt == _get_incmd_shortcut("Snap"):
            self.togglesnap()
            spec = True
        elif txt == _get_incmd_shortcut("MakeFace"):
            if self.makeFace.isVisible():
                self.makeFace.setChecked(not self.makeFace.isChecked())
            spec = True
        elif txt == _get_incmd_shortcut("Continue"):
            if self.continueCmd.isVisible():
                self.toggleContinue()
            spec = True
        elif txt == _get_incmd_shortcut("SetWP"):
            if self.orientWPButton.isVisible():
                self.orientWP()
            spec = True
        elif txt == _get_incmd_shortcut("SelectEdge"):
            self.selectEdge()
            spec = True
        elif txt == _get_incmd_shortcut("SubelementMode"):
            if self.isSubelementMode.isVisible():
                self.isSubelementMode.setChecked(not self.isSubelementMode.isChecked())
            spec = True
        elif txt == _get_incmd_shortcut("Wipe"):
            if self.wipeButton.isVisible():
                self.wipeLine()
            spec = True
        elif txt == _get_incmd_shortcut("Undo"):
            self.undoSegment()
            spec = True
        elif txt == _get_incmd_shortcut("IncreaseRadius"):
            self.toggleradius(1)
            spec = True
        elif txt == _get_incmd_shortcut("DecreaseRadius"):
            self.toggleradius(-1)
            spec = True

        if spec:
            widget = self.baseWidget.focusWidget()
            field = self.input_fields[widget.objectName()]
            value = getattr(self, field["value"])
            unit = getattr(FreeCAD.Units, field["unit"])
            v = FreeCAD.Units.Quantity(value, unit).getUserPreferred()[0]
            widget.setProperty("text", v)
            widget.setFocus()
            widget.selectAll()
        self.updateSnapper()

    def updateSnapper(self):
        """updates the snapper track line if applicable"""
        if not self.xValue.isVisible():
            return
        if (
            hasattr(FreeCADGui, "Snapper")
            and FreeCADGui.Snapper.trackLine
            and FreeCADGui.Snapper.trackLine.Visible
        ):
            point = self.get_new_point(FreeCAD.Vector(self.x, self.y, self.z))
            FreeCADGui.Snapper.trackLine.p2(point)

    def setMouseMode(self, mode=True, recorded_input_start=0.0):
        """Sets self.mouse True (default) or False and sets a timer
        to set it back to True if applicable. self.mouse is then
        used by gui_tools_utils.get_point() to know if the mouse can
        update field values and point position or not."""
        if recorded_input_start and recorded_input_start != self.mouse_delay_input_start:
            # Do nothing if a new input sequence has started.
            return
        if mode:
            self.mouse = True
        elif self.mouse:
            delay = params.get_param("MouseDelay")
            if delay:
                self.mouse = False
                recorded_input_start = self.mouse_delay_input_start
                QtCore.QTimer.singleShot(
                    delay * 1000, lambda: self.setMouseMode(True, recorded_input_start)
                )

    def checkEnterText(self):
        """this function checks if the entered text ends with two blank lines"""
        t = self.textValue.toPlainText()
        if t.endswith("\n\n"):
            self.sendText()

    def sendText(self):
        """this function sends the entered text to the active draft command
        if enter has been pressed twice. Otherwise it blanks the line.
        """
        self.sourceCmd.text = (
            self.textValue.toPlainText()
            .replace("\\", "\\\\")
            .replace('"', '\\"')
            .replace("'", "\\'")
            .splitlines()
        )
        self.sourceCmd.createObject()

    def displayPoint(self, point=None, last=None, plane=None, mask=None):
        """Displays point values in the widgets and updates self."""
        if not self.isTaskOn:
            return

        self.display_point_active = True  # prevent cyclic processing of point values

        if point:
            if not plane:
                plane = WorkingPlane.get_working_plane(update=False)
            if not last:
                if self.globalMode:
                    last = FreeCAD.Vector()
                else:
                    last = plane.position

            self.new_point = FreeCAD.Vector(point)
            self.last_point = FreeCAD.Vector(last)

            if self.relativeMode:
                if self.globalMode:
                    delta = point - last
                else:
                    delta = plane.get_local_coords(point - last, as_vector=True)
            else:
                if self.globalMode:
                    delta = point
                else:
                    delta = plane.get_local_coords(point)

            length, _, phi = DraftVecUtils.get_spherical_coords(*delta)
            phi = math.degrees(phi)

            self.x = delta.x
            self.y = delta.y
            self.z = delta.z
            self.lvalue = length
            self.avalue = phi

            self.xValue.setText(display_external(delta.x, None, "Length"))
            self.yValue.setText(display_external(delta.y, None, "Length"))
            self.zValue.setText(display_external(delta.z, None, "Length"))
            self.lengthValue.setText(display_external(length, None, "Length"))
            self.angleValue.setText(display_external(phi, None, "Angle"))

        # set masks
        if (mask == "x") or (self.mask == "x"):
            self.xValue.setEnabled(True)
            self.yValue.setEnabled(False)
            self.zValue.setEnabled(False)
            self.angleValue.setEnabled(False)
            self.setFocus("x")
        elif (mask == "y") or (self.mask == "y"):
            self.xValue.setEnabled(False)
            self.yValue.setEnabled(True)
            self.zValue.setEnabled(False)
            self.angleValue.setEnabled(False)
            self.setFocus("y")
        elif (mask == "z") or (self.mask == "z"):
            self.xValue.setEnabled(False)
            self.yValue.setEnabled(False)
            self.zValue.setEnabled(True)
            self.angleValue.setEnabled(False)
            self.setFocus("z")
        else:
            self.xValue.setEnabled(True)
            self.yValue.setEnabled(True)
            self.zValue.setEnabled(True)
            self.angleValue.setEnabled(True)
            self.setFocus()

        self.display_point_active = False

    def getDefaultColor(self, typ, rgb=False):
        """gets color from the preferences or toolbar"""
        r = 0
        g = 0
        b = 0
        if typ == "snap":
            r, g, b, _ = utils.get_rgba_tuple(params.get_param("snapcolor"))
        elif typ == "ui":
            print(
                'draft: deprecation warning: Do not use getDefaultColor("ui") anymore - use getDefaultColor("line") instead.'
            )
            r = float(self.color.red() / 255.0)
            g = float(self.color.green() / 255.0)
            b = float(self.color.blue() / 255.0)
        elif typ == "line":
            r, g, b, _ = utils.get_rgba_tuple(params.get_param_view("DefaultShapeLineColor"))
        elif typ == "text":
            r, g, b, _ = utils.get_rgba_tuple(params.get_param("DefaultTextColor"))
        elif typ == "face":
            r, g, b, _ = utils.get_rgba_tuple(params.get_param_view("DefaultShapeColor"))
        elif typ == "constr":
            r, g, b, _ = utils.get_rgba_tuple(params.get_param("constructioncolor"))
        else:
            print("draft: error: couldn't get a color for ", typ, " typ.")
        if rgb:
            return (
                "rgb(" + str(int(r * 255)) + "," + str(int(g * 255)) + "," + str(int(b * 255)) + ")"
            )
        else:
            return (r, g, b)

    def cross(self, on=True):
        """deprecated"""
        pass

    def toggleConstrMode(self, checked):
        self.baseWidget.setStyleSheet(
            "#constrButton:Checked {background-color: "
            + self.getDefaultColor("constr", rgb=True)
            + " }"
        )
        self.constrMode = checked

    def toggleContinue(self):
        FreeCAD.Console.PrintMessage("toggle continue\n")
        self.continueMode = not self.continueMode
        try:
            if hasattr(self, "continueCmd"):
                if self.continueCmd.isVisible():
                    self.continueCmd.toggle()
            if hasattr(self, "panel"):
                if hasattr(self.panel, "form"):
                    if isinstance(self.panel.form, list):
                        for w in self.panel.form:
                            c = w.findChild(QtWidgets.QCheckBox, "ContinueCmd")
                            if c:
                                c.toggle()
                    else:
                        c = self.panel.form.findChild(QtWidgets.QCheckBox, "ContinueCmd")
                        if c:
                            c.toggle()
        except Exception:
            pass

    def isConstructionMode(self):
        return self.tray is not None and self.constrButton.isChecked()

    def selectplane(self):
        FreeCADGui.runCommand("Draft_SelectPlane")

    def setstyle(self):
        FreeCADGui.runCommand("Draft_SetStyle")

    def setStyleButton(self):
        "sets icon and text on the style button"
        linecolor = QtGui.QColor(utils.rgba_to_argb(params.get_param_view("DefaultShapeLineColor")))
        facecolor = QtGui.QColor(utils.rgba_to_argb(params.get_param_view("DefaultShapeColor")))
        im = QtGui.QImage(32, 32, QtGui.QImage.Format_ARGB32)
        im.fill(QtCore.Qt.transparent)
        pt = QtGui.QPainter(im)
        pt.setPen(QtGui.QPen(QtCore.Qt.black, 1, QtCore.Qt.SolidLine, QtCore.Qt.FlatCap))
        pt.setBrush(QtGui.QBrush(linecolor, QtCore.Qt.SolidPattern))
        pts = [QtCore.QPointF(4.0, 4.0), QtCore.QPointF(4.0, 26.0), QtCore.QPointF(26.0, 4.0)]
        pt.drawPolygon(pts, QtCore.Qt.OddEvenFill)
        pt.setBrush(QtGui.QBrush(facecolor, QtCore.Qt.SolidPattern))
        pts = [QtCore.QPointF(28.0, 28.0), QtCore.QPointF(8.0, 28.0), QtCore.QPointF(28.0, 8.0)]
        pt.drawPolygon(pts, QtCore.Qt.OddEvenFill)
        pt.end()
        icon = QtGui.QIcon(QtGui.QPixmap.fromImage(im))
        linewidth = params.get_param_view("DefaultShapeLineWidth")
        fontsize = params.get_param("textheight")
        txt = (
            str(linewidth)
            + "px | "
            + FreeCAD.Units.Quantity(fontsize, FreeCAD.Units.Length).UserString
        )
        self.styleButton.setIcon(icon)
        self.styleButton.setText(txt)

        # FOR BACKWARDS COMPATIBILITY
        self.color = linecolor
        self.facecolor = facecolor
        self.linewidth = linewidth
        self.fontsize = fontsize

    def popupMenu(self, llist, ilist=None, pos=None):
        """pops up a menu filled with the given list

        "---" in llist inserts a separator
        """
        self.groupmenu = QtWidgets.QMenu()
        for i, l in enumerate(llist):
            if "---" in l:
                self.groupmenu.addSeparator()
            elif ilist:
                self.groupmenu.addAction(ilist[i], l)
            else:
                self.groupmenu.addAction(l)
        if not pos:
            pos = FreeCADGui.getMainWindow().cursor().pos()
        self.groupmenu.popup(pos)
        self.groupmenu.triggered.connect(self.popupTriggered)

    def getIcon(self, iconpath):
        return QtGui.QIcon(iconpath)

    def popupTriggered(self, action):
        self.sourceCmd.proceed(str(action.text()))

    def setRadiusValue(self, val, unit=None):
        # print("DEBUG: setRadiusValue val: ", val, " unit: ", unit)
        if not isinstance(val, (int, float)):  # some code passes strings
            t = val
        elif unit:
            t = display_external(val, None, unit)
        else:
            t = display_external(val, None, None)
        self.radiusValue.setText(t)
        self.setFocus("radius")

    def runAutoGroup(self):
        FreeCADGui.runCommand("Draft_AutoGroup")

    def setAutoGroup(self, value=None):
        if value is None:
            self.autogroup = None
            self.autoGroupButton.setText(translate("draft", "None"))
            self.autoGroupButton.setIcon(
                QtGui.QIcon.fromTheme(
                    "Draft_AutoGroup_off", QtGui.QIcon(":/icons/button_invalid.svg")
                )
            )
            self.autoGroupButton.setToolTip(translate("draft", "Autogroup off"))
            self.autoGroupButton.setDown(False)
        else:
            obj = FreeCAD.ActiveDocument.getObject(value)
            if obj:
                self.autogroup = value
                self.autoGroupButton.setText(obj.Label)
                self.autoGroupButton.setIcon(obj.ViewObject.Icon)
                self.autoGroupButton.setToolTip(translate("draft", "Autogroup:") + " " + obj.Label)
                self.autoGroupButton.setDown(False)
            else:
                self.autogroup = None
                self.autoGroupButton.setText(translate("draft", "None"))
                self.autoGroupButton.setIcon(
                    QtGui.QIcon.fromTheme(
                        "Draft_AutoGroup_off", QtGui.QIcon(":/icons/button_invalid.svg")
                    )
                )
                self.autoGroupButton.setToolTip(translate("draft", "Autogroup off"))
                self.autoGroupButton.setDown(False)

    def getXPM(self, iconname, size=16):
        i = QtGui.QIcon(":/icons/" + iconname + ".svg")
        p = i.pixmap(size, size)
        a = QtCore.QByteArray()
        b = QtCore.QBuffer(a)
        b.open(QtCore.QIODevice.WriteOnly)
        p.save(b, "XPM")
        b.close()
        return str(a)

    def togglesnap(self):
        FreeCADGui.doCommand('FreeCADGui.runCommand("Draft_Snap_Lock")')

    def toggleradius(self, val):
        if hasattr(FreeCADGui, "Snapper"):
            par = params.get_param("snapRange")
            params.set_param("snapRange", max(0, par + val))
            FreeCADGui.Snapper.showradius()

    def constrain(self, val):
        if val == "angle":
            self.alock = not (self.alock)
            self.angleLock.setChecked(self.alock)
        elif self.mask == val:
            self.mask = None
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.mask = None
        else:
            self.mask = val
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.mask = val
                self.new_point = FreeCADGui.Snapper.constrain(self.new_point, self.get_last_point())

    def changeXValue(self, d):
        if self.display_point_active:
            return
        if not self.xValue.hasFocus():
            return
        self.x = d.Value
        self.update_spherical_coords()
        self.updateSnapper()

    def changeYValue(self, d):
        if self.display_point_active:
            return
        if not self.yValue.hasFocus():
            return
        self.y = d.Value
        self.update_spherical_coords()
        self.updateSnapper()

    def changeZValue(self, d):
        if self.display_point_active:
            return
        if not self.zValue.hasFocus():
            return
        self.z = d.Value
        self.update_spherical_coords()
        self.updateSnapper()

    def changeRadiusValue(self, d):
        if self.display_point_active:
            return
        if not self.radiusValue.hasFocus():
            return
        self.radius = d.Value

    def changeLengthValue(self, d):
        if self.display_point_active:
            return
        if not self.lengthValue.hasFocus():
            return
        self.lvalue = d.Value
        self.update_cartesian_coords()
        self.updateSnapper()

    def changeAngleValue(self, d):
        if self.display_point_active:
            return
        if not self.angleValue.hasFocus():
            return
        self.avalue = d.Value
        self.update_cartesian_coords()
        self.updateSnapper()
        if self.angleLock.isChecked():
            if not self.globalMode:
                plane = WorkingPlane.get_working_plane(update=False)
                angle_vec = plane.get_global_coords(self.angle, as_vector=True)
            else:
                angle_vec = self.angle
            FreeCADGui.Snapper.setAngle(angle_vec)

    def toggleAngle(self, b):
        self.alock = self.angleLock.isChecked()
        self.update_cartesian_coords()
        self.updateSnapper()
        if self.alock:
            if not self.globalMode:
                plane = WorkingPlane.get_working_plane(update=False)
                angle_vec = plane.get_global_coords(self.angle, as_vector=True)
            else:
                angle_vec = self.angle
            FreeCADGui.Snapper.setAngle(angle_vec)
        else:
            FreeCADGui.Snapper.setAngle()
            self.angle = None

    def update_spherical_coords(self):
        length, theta, phi = DraftVecUtils.get_spherical_coords(self.x, self.y, self.z)
        self.lvalue = length
        self.pvalue = math.degrees(theta)
        self.avalue = math.degrees(phi)
        self.angle = FreeCAD.Vector(DraftVecUtils.get_cartesian_coords(1, theta, phi))
        self.lengthValue.setText(display_external(self.lvalue, None, "Length"))
        self.angleValue.setText(display_external(self.avalue, None, "Angle"))

    def update_cartesian_coords(self):
        self.x, self.y, self.z = DraftVecUtils.get_cartesian_coords(
            self.lvalue, math.radians(self.pvalue), math.radians(self.avalue)
        )
        self.angle = FreeCAD.Vector(
            DraftVecUtils.get_cartesian_coords(
                1, math.radians(self.pvalue), math.radians(self.avalue)
            )
        )
        self.xValue.setText(display_external(self.x, None, "Length"))
        self.yValue.setText(display_external(self.y, None, "Length"))
        self.zValue.setText(display_external(self.z, None, "Length"))

    def get_last_point(self):
        """Get the last point in the GCS."""
        if getattr(self.sourceCmd, "node", []):
            return self.sourceCmd.node[-1]
        if self.last_point is not None:
            return self.last_point
        if self.globalMode:
            return FreeCAD.Vector()
        return WorkingPlane.get_working_plane(update=False).position

    def get_new_point(self, delta):
        """Get the new point in the GCS.

        The delta vector (from the task panel) can be global/local
        and relative/absolute.
        """
        if self.globalMode:
            base_point = FreeCAD.Vector()
        else:
            plane = WorkingPlane.get_working_plane(update=False)
            delta = plane.get_global_coords(delta, as_vector=True)
            base_point = plane.position
        if self.relativeMode:
            base_point = self.get_last_point()
        return base_point + delta

    # ---------------------------------------------------------------------------
    # TaskView operations
    # ---------------------------------------------------------------------------

    def setWatchers(self):
        class DraftCreateWatcher:
            def __init__(self):
                self.commands = [
                    "Draft_Line",
                    "Draft_Wire",
                    "Draft_Rectangle",
                    "Draft_Arc",
                    "Draft_Circle",
                    "Draft_BSpline",
                    "Draft_Text",
                    "Draft_Dimension",
                    "Draft_ShapeString",
                    "Draft_BezCurve",
                ]
                self.title = "Create objects"

            def shouldShow(self):
                return (FreeCAD.ActiveDocument is not None) and (
                    not FreeCADGui.Selection.getSelection()
                )

        class DraftModifyWatcher:
            def __init__(self):
                self.commands = [
                    "Draft_Move",
                    "Draft_Rotate",
                    "Draft_Scale",
                    "Draft_Offset",
                    "Draft_Trimex",
                    "Draft_Upgrade",
                    "Draft_Downgrade",
                    "Draft_Edit",
                ]
                self.title = translate("draft", "Modify Objects")

            def shouldShow(self):
                return (FreeCAD.ActiveDocument is not None) and (
                    FreeCADGui.Selection.getSelection() != []
                )

        FreeCADGui.Control.addTaskWatcher([DraftCreateWatcher(), DraftModifyWatcher()])

    def changeEvent(self, event):
        if event.type() == QtCore.QEvent.LanguageChange:
            # print("Language changed!")
            self.ui.retranslateUi(self)

    def Activated(self):
        self.setWatchers()
        if hasattr(self, "tray"):
            todo.delay(self.tray.show, None)

    def Deactivated(self):
        if FreeCAD.activeDraftCommand is not None:
            self.continueMode = False
            FreeCAD.activeDraftCommand.finish()
        FreeCADGui.Control.clearTaskWatcher()
        # self.tray = None
        if hasattr(self, "tray"):
            todo.delay(self.tray.hide, None)

    def reset_ui_values(self):
        """Method to reset task panel values"""
        self.x = 0
        self.y = 0
        self.z = 0
        self.new_point = None
        self.last_point = None
        self.lvalue = 0
        self.pvalue = 90
        self.avalue = 0
        self.angle = None
        self.radius = 0
        self.offset = 0


class FacebinderTaskPanel:
    """A TaskPanel for the facebinder"""

    def __init__(self):

        self.obj = None
        self.form = QtWidgets.QWidget()
        self.form.setObjectName("FacebinderTaskPanel")
        self.grid = QtWidgets.QGridLayout(self.form)
        self.grid.setObjectName("grid")
        self.title = QtWidgets.QLabel(self.form)
        self.grid.addWidget(self.title, 0, 0, 1, 2)

        # tree
        self.tree = QtWidgets.QTreeWidget(self.form)
        self.grid.addWidget(self.tree, 1, 0, 1, 2)
        self.tree.setColumnCount(2)
        self.tree.setHeaderLabels(["Name", "Subelement"])

        # buttons
        self.addButton = QtWidgets.QPushButton(self.form)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 3, 0, 1, 1)

        self.delButton = QtWidgets.QPushButton(self.form)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 3, 1, 1, 1)

        self.addButton.clicked.connect(self.addElement)
        self.delButton.clicked.connect(self.removeElement)
        self.update()

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return QtWidgets.QDialogButtonBox.Ok

    def update(self):
        """fills the treewidget"""
        self.tree.clear()
        if self.obj:
            for f in self.obj.Faces:
                if isinstance(f[1], tuple):
                    for subf in f[1]:
                        item = QtWidgets.QTreeWidgetItem(self.tree)
                        item.setText(0, f[0].Name)
                        item.setIcon(0, QtGui.QIcon(":/icons/Part_3D_object.svg"))
                        item.setText(1, subf)
                else:
                    item = QtWidgets.QTreeWidgetItem(self.tree)
                    item.setText(0, f[0].Name)
                    item.setIcon(0, QtGui.QIcon(":/icons/Part_3D_object.svg"))
                    item.setText(1, f[1])
        self.retranslateUi(self.form)

    def addElement(self):
        if self.obj:
            for sel in FreeCADGui.Selection.getSelectionEx("", 0):
                if sel.HasSubObjects:
                    obj = sel.Object
                    for elt in sel.SubElementNames:
                        if "Face" in elt:
                            flist = self.obj.Faces
                            found = False
                            for face in flist:
                                if face[0] == obj.Name:
                                    if isinstance(face[1], tuple):
                                        for subf in face[1]:
                                            if subf == elt:
                                                found = True
                                    else:
                                        if face[1] == elt:
                                            found = True
                            if not found:
                                flist.append((obj, elt))
                                self.obj.Faces = flist
                                FreeCAD.ActiveDocument.recompute()
            self.update()

    def removeElement(self):
        if self.obj:
            it = self.tree.currentItem()
            if it:
                obj = FreeCAD.ActiveDocument.getObject(str(it.text(0)))
                elt = str(it.text(1))
                flist = []
                for face in self.obj.Faces:
                    if face[0].Name != obj.Name:
                        flist.append(face)
                    else:
                        if isinstance(face[1], tuple):
                            for subf in face[1]:
                                if subf != elt:
                                    flist.append((obj, subf))
                        else:
                            if face[1] != elt:
                                flist.append(face)
                self.obj.Faces = flist
                FreeCAD.ActiveDocument.recompute()
            self.update()

    def accept(self):
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtWidgets.QApplication.translate("draft", "Faces", None))
        self.delButton.setText(QtWidgets.QApplication.translate("draft", "Remove", None))
        self.addButton.setText(QtWidgets.QApplication.translate("draft", "Add", None))
        self.title.setText(QtWidgets.QApplication.translate("draft", "Facebinder Elements", None))


# def translateWidget(w, context=None, disAmb=None):
#    '''translator for items where retranslateUi() is unavailable.
#    translates widget w and children.'''
#    #handle w itself
#    if w.metaObject().className() == "QWidget":
#        origText = None
#        origText = w.windowTitle()
#        if origText:
#            newText = translate(context, str(origText))
#            if newText:
#                w.setWindowTitle(newText)

#    #handle children
#    wKids = w.findChildren(QtWidgets.QWidget)
#    for i in wKids:
#        className = i.metaObject().className()
#        if hasattr(i,"text") and hasattr(i,"setText"):
#            origText = i.text()
#            newText = translate(context, str(origText))
#            if newText:
#                i.setText(newText)
#        elif hasattr(i,"title") and hasattr(i,"setTitle"):
#            origText = i.title()
#            newText = translate(context, str(origText))
#            if newText:
#                i.setTitle(newText)
#        elif hasattr(i,"itemText") and hasattr(i,"setItemText"):
#            for item in range(i.count()):
#                oldText = i.itemText(item)
#                newText = translate(context, str(origText))
#                if newText:
#                    i.setItemText(item,newText)
##for debugging:
##        else:
##            msg = "TranslateWidget: Can not translate widget: {0} type: {1}\n".format(w.objectName(),w.metaObject().className())
##            FreeCAD.Console.PrintMessage(msg)

if not hasattr(FreeCADGui, "draftToolBar"):
    FreeCADGui.draftToolBar = DraftToolBar()
# ----End of Python Features Definitions----#
