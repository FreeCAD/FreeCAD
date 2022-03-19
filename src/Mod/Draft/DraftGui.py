# -*- coding: utf8 -*-
#***************************************************************************
#*   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__  = "FreeCAD Draft Workbench - GUI part"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "https://www.freecadweb.org"

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
import math
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui

import FreeCAD
import FreeCADGui
import Draft
import DraftVecUtils

from draftutils.translate import translate
from draftutils.utils import utf8_decode

# in-command shortcut definitions: Shortcut / Translation / related UI control
inCommandShortcuts = {
    "Relative": [
        Draft.getParam("inCommandShortcutRelative", "R"),
        translate("draft","Relative"),
        "isRelative"
        ],
    "Global": [Draft.getParam("inCommandShortcutGlobal", "G"),
        translate("draft","Global"),
        "isGlobal"
        ],
    "Continue": [
        Draft.getParam("inCommandShortcutContinue", "T"),
        translate("draft","Continue"),
        "continueCmd"
        ],
    "Close": [
        Draft.getParam("inCommandShortcutClose", "O"),
        translate("draft","Close"),
        "closeButton"
        ],
    "Copy": [
        Draft.getParam("inCommandShortcutCopy", "P"),
        translate("draft","Copy"),
        "isCopy"
        ],
    "SubelementMode": [
        Draft.getParam("inCommandShortcutSubelementMode", "D"),
        translate("draft","Subelement mode"),
        "isSubelementMode"
        ],
    "Fill": [
        Draft.getParam("inCommandShortcutFill", "L"),
        translate("draft","Fill"),
        "hasFill"
        ],
    "Exit": [
        Draft.getParam("inCommandShortcutExit", "A"),
        translate("draft","Exit"),
        "finishButton"
        ],
    "Snap": [
        Draft.getParam("inCommandShortcutSnap", "S"),
        translate("draft","Snap On/Off"),
        None
        ],
    "Increase": [
        Draft.getParam("inCommandShortcutIncreaseRadius", "["),
        translate("draft","Increase snap radius"),
        None
        ],
    "Decrease": [
        Draft.getParam("inCommandShortcutDecreaseRadius", "]"),
        translate("draft","Decrease snap radius"),
        None
        ],
    "RestrictX": [
        Draft.getParam("inCommandShortcutRestrictX", "X"),
        translate("draft","Restrict X"),
        None
        ],
    "RestrictY": [
        Draft.getParam("inCommandShortcutRestrictY", "Y"),
        translate("draft","Restrict Y"),
        None
        ],
    "RestrictZ": [
        Draft.getParam("inCommandShortcutRestrictZ", "Z"),
        translate("draft","Restrict Z"),
        None
        ],
    "SelectEdge": [
        Draft.getParam("inCommandShortcutSelectEdge", "E"),
        translate("draft","Select edge"),
        "selectButton"
        ],
    "AddHold": [
        Draft.getParam("inCommandShortcutAddHold", "Q"),
        translate("draft","Add custom snap point"),
        None
        ],
    "Length": [
        Draft.getParam("inCommandShortcutLength", "H"),
        translate("draft","Length mode"),
        "lengthValue"
        ],
    "Wipe": [
        Draft.getParam("inCommandShortcutWipe", "W"),
        translate("draft","Wipe"),
        "wipeButton"
        ],
    "SetWP": [
        Draft.getParam("inCommandShortcutSetWP", "U"),
        translate("draft","Set Working Plane"),
        "orientWPButton"
        ],
    "CycleSnap": [
        Draft.getParam("inCommandShortcutCycleSnap", "`"),
        translate("draft","Cycle snap object"),
        None
        ],
    "NearSnap": [
        Draft.getParam("inCommandShortcutNearSnap", "N"),
        translate("draft","Toggle near snap on/off"),
        None
        ],
}

from draftutils.todo import todo

#---------------------------------------------------------------------------
# UNITS handling
#---------------------------------------------------------------------------
from draftutils.units import (getDefaultUnit,
                              makeFormatSpec,
                              displayExternal)

#---------------------------------------------------------------------------
# Customized widgets
#---------------------------------------------------------------------------

class DraftBaseWidget(QtGui.QWidget):
    def __init__(self,parent = None):
        super().__init__(parent)
    def eventFilter(self, widget, event):
        if (event.type() == QtCore.QEvent.KeyPress
            and event.text().upper() == inCommandShortcuts["CycleSnap"][0]):
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.cycleSnapObject()
            return True
        return super().eventFilter(widget, event)

class DraftDockWidget(DraftBaseWidget):
    """custom Widget that emits a resized() signal when resized"""
    def __init__(self,parent = None):
        super().__init__(parent)
    def resizeEvent(self,event):
        self.emit(QtCore.SIGNAL("resized()"))
    def changeEvent(self, event):
        if event.type() == QtCore.QEvent.LanguageChange:
            self.emit(QtCore.SIGNAL("retranslate()"))
        else:
            super().changeEvent(event)

class DraftLineEdit(QtGui.QLineEdit):
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
    def __init__(self,widget,extra=None):
        if extra:
            if isinstance(extra,list):
                self.form = [widget] + extra
            else:
                self.form = [widget,extra]
        else:
            self.form = widget
    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)
    def accept(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            return FreeCADGui.draftToolBar.validatePoint()
        else:
            if FreeCADGui.ActiveDocument:
                FreeCADGui.ActiveDocument.resetEdit()
            return True
    def reject(self):
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
        self.cancel = None
        self.pointcallback = None
        self.taskmode = 1  # Draft.getParam("UiMode",1)
        # taskmode = 0 was used by draft toolbar that is now obsolete.
        # print("taskmode: ",str(self.taskmode))

        # OBSOLETE BUT STILL USED BY SOME ADDONS AND MACROS
        self.paramcolor = Draft.getParam("color",255)>>8
        self.color = QtGui.QColor(self.paramcolor)
        self.facecolor = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")\
            .GetUnsigned("DefaultShapeColor",4294967295)>>8
        self.linewidth = Draft.getParam("linewidth",2)
        self.fontsize = Draft.getParam("textheight",0.20)

        self.paramconstr = Draft.getParam("constructioncolor",746455039)>>8
        self.constrMode = False
        self.continueMode = False
        self.relativeMode = True
        self.globalMode = False
        self.state = None
        self.textbuffer = []
        self.crossedViews = []
        self.isTaskOn = False
        self.fillmode = Draft.getParam("fillmode",False)
        self.mask = None
        self.alock = False
        self.x = 0
        self.y = 0
        self.z = 0
        self.lvalue = 0
        self.pvalue = 90
        self.avalue = 0
        self.angle = None
        self.radius = 0
        self.offset = 0
        self.uiloader = FreeCADGui.UiLoader()
        self.autogroup = None
        self.isCenterPlane = False
        self.lastMode = None
        self.input_fields = {
            "xValue":{"value":"x","unit":"Length"},
            "yValue":{"value":"y","unit":"Length"},
            "zValue":{"value":"z","unit":"Length"},
            "lengthValue":{"value":"lvalue","unit":"Length"},
            "radiusValue":{"value":"radius","unit":"Length"},
            "angleValue":{"value":"avalue","unit":"Angle"}
        }

        if self.taskmode:
            # add only a dummy widget, since widgets are created on demand
            self.baseWidget = DraftBaseWidget()
            self.tray = QtGui.QToolBar(None)
            self.tray.setObjectName("Draft tray")
            self.tray.setWindowTitle("Draft tray")
            self.toptray = self.tray
            self.bottomtray = self.tray
            self.setupTray()
            self.setupStyle()
            mw = FreeCADGui.getMainWindow()
            mw.addToolBar(self.tray)
            self.tray.setParent(mw)
            self.tray.hide()

        else: # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            # create the draft Toolbar
            self.draftWidget = QtGui.QDockWidget()
            self.baseWidget = DraftDockWidget()
            self.draftWidget.setObjectName("draftToolbar")
            self.scroll = QtGui.QScrollArea()
            self.scroll.setWidgetResizable(True)
            self.scroll.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
            self.scroll.setWidget(self.baseWidget)
            self.draftWidget.setTitleBarWidget(self.scroll)
            p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
            size = p.GetInt("ToolbarIconSize", 24)
            if size < 24:
                scale = 3.5
            else:
                scale = 2.5
            self.scroll.setMinimumHeight(size * scale)
            self.draftWidget.setWindowTitle(translate("draft", "Draft Command Bar"))
            self.mw = FreeCADGui.getMainWindow()
            self.mw.addDockWidget(QtCore.Qt.TopDockWidgetArea,self.draftWidget)
            self.draftWidget.setVisible(False)
            self.draftWidget.toggleViewAction().setVisible(False)
            self.baseWidget.setObjectName("draftToolbar")
            self.layout = QtGui.QHBoxLayout(self.baseWidget)
            self.layout.setObjectName("layout")
            self.toptray = self.layout
            self.bottomtray = self.layout
            self.setupToolBar()
            self.setupTray()
            self.setupStyle()
            self.retranslateUi(self.baseWidget)

#---------------------------------------------------------------------------
# General UI setup
#---------------------------------------------------------------------------

    def _pushbutton(self,name, layout, hide=True, icon=None,
                    width=None, checkable=False, square=False):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
        bsize = p.GetInt("ToolbarIconSize",24)+2
        isize = p.GetInt("ToolbarIconSize",24)/3*2
        button = QtGui.QPushButton(self.baseWidget)
        button.setObjectName(name)
        if square:
            button.setMaximumSize(QtCore.QSize(button.height(), button.height()))
            button.setFlat(True)
        #button.setMaximumSize(QtCore.QSize(width,bsize))
        if hide:
            button.hide()
        if icon:
            if icon.endswith(".svg"):
                button.setIcon(QtGui.QIcon(icon))
            else:
                button.setIcon(QtGui.QIcon.fromTheme(
                    icon, QtGui.QIcon(':/icons/'+icon+'.svg')))
            #button.setIconSize(QtCore.QSize(isize, isize))
        if checkable:
            button.setCheckable(True)
            button.setChecked(False)
        layout.addWidget(button)
        return button

    def _label (self,name, layout, hide=True, wrap=False):
        label = QtGui.QLabel(self.baseWidget)
        label.setObjectName(name)
        if wrap:
            label.setWordWrap(True)
        if hide: label.hide()
        layout.addWidget(label)
        return label

    def _lineedit (self,name, layout, hide=True, width=None):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
        bsize = p.GetInt("ToolbarIconSize",24)-2
        lineedit = DraftLineEdit(self.baseWidget)
        lineedit.setObjectName(name)
        if hide: lineedit.hide()
        #if not width: width = 800
        #lineedit.setMaximumSize(QtCore.QSize(width,bsize))
        layout.addWidget(lineedit)
        return lineedit

    def _inputfield (self,name, layout, hide=True, width=None):
        inputfield = self.uiloader.createWidget("Gui::InputField")
        inputfield.setObjectName(name)
        if hide: inputfield.hide()
        if not width:
            sizePolicy = QtGui.QSizePolicy(
                QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Preferred)
            inputfield.setSizePolicy(sizePolicy)
            inputfield.setMinimumWidth(110)
        else:
            inputfield.setMaximumWidth(width)
        layout.addWidget(inputfield)
        return inputfield

    def _spinbox (self,name, layout, val=None, vmax=None,
                  hide=True, double=False, size=None):
        if double:
            sbox = QtGui.QDoubleSpinBox(self.baseWidget)
            sbox.setDecimals(FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")\
                .GetInt("Decimals",2))
        else:
            sbox = QtGui.QSpinBox(self.baseWidget)
        sbox.setObjectName(name)
        if vmax: sbox.setMaximum(vmax)
        if val: sbox.setValue(val)
        #if size: sbox.setMaximumSize(QtCore.QSize(size[0],size[1]))
        if hide: sbox.hide()
        layout.addWidget(sbox)
        return sbox

    def _checkbox (self,name, layout, checked=True, hide=True):
        chk = QtGui.QCheckBox(self.baseWidget)
        chk.setChecked(checked)
        chk.setObjectName(name)
        if hide: chk.hide()
        layout.addWidget(chk)
        return chk

    def _combo (self,name,layout,hide=True):
        cb = QtGui.QComboBox(self.baseWidget)
        cb.setObjectName(name)
        if hide: cb.hide()
        layout.addWidget(cb)

    def setupToolBar(self,task=False):
        """sets the draft toolbar up"""

        # command

        self.promptlabel = self._label("promptlabel", self.layout, hide=task)
        self.cmdlabel = self._label("cmdlabel", self.layout, hide=task)
        boldtxt = QtGui.QFont()
        boldtxt.setWeight(75)
        boldtxt.setBold(True)
        self.cmdlabel.setFont(boldtxt)

        # subcommands for draft Edit, OBSOLETE

        # self.addButton = self._pushbutton("addButton", self.layout, icon="Draft_AddPoint", width=22, checkable=True)
        # self.delButton = self._pushbutton("delButton", self.layout, icon="Draft_DelPoint", width=22, checkable=True)
        # self.sharpButton = self._pushbutton("sharpButton", self.layout, icon="Draft_BezSharpNode", width=22, checkable=True)
        # self.tangentButton = self._pushbutton("tangentButton", self.layout, icon="Draft_BezTanNode", width=22, checkable=True)
        # self.symmetricButton = self._pushbutton("symmetricButton", self.layout, icon="Draft_BezSymNode", width=22, checkable=True)
        # self.arc3PtButton = self._pushbutton("arc3PtButton", self.layout, icon="Draft_Arc", width=22, checkable=True)

        # point

        xl = QtGui.QHBoxLayout()
        yl = QtGui.QHBoxLayout()
        zl = QtGui.QHBoxLayout()
        bl = QtGui.QHBoxLayout()
        self.layout.addLayout(xl)
        self.layout.addLayout(yl)
        self.layout.addLayout(zl)
        self.layout.addLayout(bl)
        self.labelx = self._label("labelx", xl)
        self.xValue = self._inputfield("xValue", xl) #width=60
        self.xValue.installEventFilter(self.baseWidget)
        self.xValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.labely = self._label("labely", yl)
        self.yValue = self._inputfield("yValue", yl)
        self.yValue.installEventFilter(self.baseWidget) # Required to detect snap cycling in case of Y constraining.
        self.yValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.labelz = self._label("labelz", zl)
        self.zValue = self._inputfield("zValue", zl)
        self.zValue.installEventFilter(self.baseWidget) # Required to detect snap cycling in case of Z constraining.
        self.zValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.pointButton = self._pushbutton("addButton", bl, icon="Draft_AddPoint")

        # text

        self.textValue = QtGui.QTextEdit(self.baseWidget)
        self.textValue.setObjectName("textValue")
        self.textValue.setTabChangesFocus(True)
        self.layout.addWidget(self.textValue)
        self.textValue.hide()
        tl = QtGui.QHBoxLayout()
        self.layout.addLayout(tl)
        self.textOkButton = self._pushbutton("textButton", tl, icon="button_valid")

        # additional line controls

        ll = QtGui.QHBoxLayout()
        al = QtGui.QHBoxLayout()
        self.layout.addLayout(ll)
        self.layout.addLayout(al)
        self.labellength = self._label("labellength", ll)
        self.lengthValue = self._inputfield("lengthValue", ll)
        self.lengthValue.installEventFilter(self.baseWidget) # Required to detect snap cycling if focusOnLength is True.
        self.lengthValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.labelangle = self._label("labelangle", al)
        self.angleLock = self._checkbox("angleLock",al,checked=self.alock)
        self.angleValue = self._inputfield("angleValue", al)
        self.angleValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Angle).UserString)

        # shapestring

        self.labelSSize = self._label("labelSize", self.layout)
        self.SSizeValue = self._inputfield("SSizeValue", self.layout)           #, width=60)
        self.SSizeValue.setText(FreeCAD.Units.Quantity(1,FreeCAD.Units.Length).UserString)
        self.labelSTrack = self._label("labelTracking", self.layout)
        self.STrackValue = self._inputfield("STrackValue", self.layout)         #, width=60)
        self.STrackValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.labelSString = self._label("labelString", self.layout)
        self.SStringValue = self._lineedit("SStringValue", self.layout)
        self.SStringValue.setText("")
        self.labelFFile = self._label("labelFFile", self.layout)
        self.FFileValue = self._lineedit("FFileValue", self.layout)
        self.chooserButton = self._pushbutton("chooserButton", self.layout, width=26)
        self.chooserButton.setText("...")
        self.SSize = 1
        self.STrack = 0

        # options

        fl = QtGui.QHBoxLayout()
        self.layout.addLayout(fl)
        self.numFacesLabel = self._label("numfaceslabel", fl)
        self.numFaces = self._spinbox("numFaces", fl, 3)
        ol = QtGui.QHBoxLayout()
        self.layout.addLayout(ol)
        rl = QtGui.QHBoxLayout()
        self.layout.addLayout(rl)
        self.labelRadius = self._label("labelRadius", rl)
        self.radiusValue = self._inputfield("radiusValue", rl)
        self.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.isRelative = self._checkbox("isRelative",self.layout,checked=self.relativeMode)
        self.isGlobal = self._checkbox("isGlobal",self.layout,checked=self.globalMode)
        self.hasFill = self._checkbox("hasFill",self.layout,checked=self.fillmode)
        self.continueCmd = self._checkbox("continueCmd",self.layout,checked=self.continueMode)
        self.occOffset = self._checkbox("occOffset",self.layout,checked=False)
        bl = QtGui.QHBoxLayout()
        self.layout.addLayout(bl)
        self.undoButton = self._pushbutton("undoButton", bl, icon='Draft_Rotate')
        bl = QtGui.QHBoxLayout()
        self.layout.addLayout(bl)
        self.finishButton = self._pushbutton("finishButton", bl, icon='Draft_Finish')
        bl = QtGui.QHBoxLayout()
        self.layout.addLayout(bl)
        self.closeButton = self._pushbutton("closeButton", bl, icon='Draft_Lock')
        bl = QtGui.QHBoxLayout()
        self.layout.addLayout(bl)
        self.wipeButton = self._pushbutton("wipeButton", bl, icon='Draft_Wipe')
        bl = QtGui.QHBoxLayout()
        self.layout.addLayout(bl)
        self.orientWPButton = self._pushbutton("orientWPButton", bl, icon='Draft_SelectPlane')
        bl = QtGui.QHBoxLayout()
        self.layout.addLayout(bl)
        self.selectButton = self._pushbutton("selectButton", bl, icon='view-select')

        self.isCopy = self._checkbox("isCopy",self.layout,checked=False)
        self.isSubelementMode = self._checkbox("isSubelementMode",self.layout,checked=False)

        # spacer
        if not self.taskmode:
            # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding,
                                           QtGui.QSizePolicy.Minimum)
        else:
            spacerItem = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum,
                                           QtGui.QSizePolicy.Expanding)
        self.layout.addItem(spacerItem)


        QtCore.QObject.connect(self.xValue,QtCore.SIGNAL("valueChanged(double)"),self.changeXValue)
        QtCore.QObject.connect(self.yValue,QtCore.SIGNAL("valueChanged(double)"),self.changeYValue)
        QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("valueChanged(double)"),self.changeZValue)
        QtCore.QObject.connect(self.lengthValue,QtCore.SIGNAL("valueChanged(double)"),self.changeLengthValue)
        QtCore.QObject.connect(self.angleValue,QtCore.SIGNAL("valueChanged(double)"),self.changeAngleValue)
        QtCore.QObject.connect(self.angleLock,QtCore.SIGNAL("stateChanged(int)"),self.toggleAngle)
        QtCore.QObject.connect(self.radiusValue,QtCore.SIGNAL("valueChanged(double)"),self.changeRadiusValue)
        QtCore.QObject.connect(self.xValue,QtCore.SIGNAL("returnPressed()"),self.checkx)
        QtCore.QObject.connect(self.yValue,QtCore.SIGNAL("returnPressed()"),self.checky)
        QtCore.QObject.connect(self.lengthValue,QtCore.SIGNAL("returnPressed()"),self.checklength)
        QtCore.QObject.connect(self.xValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.yValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.lengthValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.radiusValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.angleValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("returnPressed()"),self.validatePoint)
        QtCore.QObject.connect(self.pointButton,QtCore.SIGNAL("clicked()"),self.validatePoint)
        QtCore.QObject.connect(self.radiusValue,QtCore.SIGNAL("returnPressed()"),self.validatePoint)
        QtCore.QObject.connect(self.angleValue,QtCore.SIGNAL("returnPressed()"),self.validatePoint)
        QtCore.QObject.connect(self.textValue,QtCore.SIGNAL("textChanged()"),self.checkEnterText)
        QtCore.QObject.connect(self.textOkButton,QtCore.SIGNAL("clicked()"),self.sendText)
        QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("returnPressed()"),self.setFocus)

        # Draft Edit UI obsolete due to introduction of incommand context menu
        # QtCore.QObject.connect(self.addButton,QtCore.SIGNAL("toggled(bool)"),self.setAddMode)
        # QtCore.QObject.connect(self.delButton,QtCore.SIGNAL("toggled(bool)"),self.setDelMode)
        # QtCore.QObject.connect(self.sharpButton,QtCore.SIGNAL("toggled(bool)"),self.setSharpMode)
        # QtCore.QObject.connect(self.tangentButton,QtCore.SIGNAL("toggled(bool)"),self.setTangentMode)
        # QtCore.QObject.connect(self.symmetricButton,QtCore.SIGNAL("toggled(bool)"),self.setSymmetricMode)
        # QtCore.QObject.connect(self.arc3PtButton,QtCore.SIGNAL("toggled(bool)"),self.setArc3PtMode)

        QtCore.QObject.connect(self.finishButton,QtCore.SIGNAL("pressed()"),self.finish)
        QtCore.QObject.connect(self.closeButton,QtCore.SIGNAL("pressed()"),self.closeLine)
        QtCore.QObject.connect(self.wipeButton,QtCore.SIGNAL("pressed()"),self.wipeLine)
        QtCore.QObject.connect(self.orientWPButton,QtCore.SIGNAL("pressed()"),self.orientWP)
        QtCore.QObject.connect(self.undoButton,QtCore.SIGNAL("pressed()"),self.undoSegment)
        QtCore.QObject.connect(self.selectButton,QtCore.SIGNAL("pressed()"),self.selectEdge)
        QtCore.QObject.connect(self.continueCmd,QtCore.SIGNAL("stateChanged(int)"),self.setContinue)

        QtCore.QObject.connect(self.isCopy,QtCore.SIGNAL("stateChanged(int)"),self.setCopymode)
        QtCore.QObject.connect(self.isSubelementMode, QtCore.SIGNAL("stateChanged(int)"), self.setSubelementMode)

        QtCore.QObject.connect(self.isRelative,QtCore.SIGNAL("stateChanged(int)"),self.setRelative)
        QtCore.QObject.connect(self.isGlobal,QtCore.SIGNAL("stateChanged(int)"),self.setGlobal)
        QtCore.QObject.connect(self.hasFill,QtCore.SIGNAL("stateChanged(int)"),self.setFill)
        QtCore.QObject.connect(self.baseWidget,QtCore.SIGNAL("resized()"),self.relocate)
        QtCore.QObject.connect(self.baseWidget,QtCore.SIGNAL("retranslate()"),self.retranslateUi)
        QtCore.QObject.connect(self.SSizeValue,QtCore.SIGNAL("valueChanged(double)"),self.changeSSizeValue)
        QtCore.QObject.connect(self.SSizeValue,QtCore.SIGNAL("returnPressed()"),self.validateSNumeric)
        QtCore.QObject.connect(self.STrackValue,QtCore.SIGNAL("valueChanged(double)"),self.changeSTrackValue)
        QtCore.QObject.connect(self.STrackValue,QtCore.SIGNAL("returnPressed()"),self.validateSNumeric)
        QtCore.QObject.connect(self.SStringValue,QtCore.SIGNAL("returnPressed()"),self.validateSString)
        QtCore.QObject.connect(self.chooserButton,QtCore.SIGNAL("pressed()"),self.pickFile)
        QtCore.QObject.connect(self.FFileValue,QtCore.SIGNAL("returnPressed()"),self.validateFile)


        # following lines can cause a crash and are not needed anymore when using the task panel
        # https://forum.freecadweb.org/viewtopic.php?f=3&t=6952
        #QtCore.QObject.connect(self.FFileValue,QtCore.SIGNAL("escaped()"),self.escape)
        #QtCore.QObject.connect(self.xValue,QtCore.SIGNAL("escaped()"),self.escape)
        #QtCore.QObject.connect(self.yValue,QtCore.SIGNAL("escaped()"),self.escape)
        #QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("escaped()"),self.escape)
        #QtCore.QObject.connect(self.radiusValue,QtCore.SIGNAL("escaped()"),self.escape)
        #QtCore.QObject.connect(self.SSizeValue,QtCore.SIGNAL("escaped()"),self.escape)
        #QtCore.QObject.connect(self.STrackValue,QtCore.SIGNAL("escaped()"),self.escape)
        #QtCore.QObject.connect(self.SStringValue,QtCore.SIGNAL("escaped()"),self.escape)

        # if Ui changed to have Size & Track visible at same time, use this
        #QtCore.QObject.connect(self.SSizeValue,QtCore.SIGNAL("returnPressed()"),self.checkSSize)
        #QtCore.QObject.connect(self.STrackValue,QtCore.SIGNAL("returnPressed()"),self.checkSTrack)

    def setupTray(self):
        """sets draft tray buttons up"""

        self.wplabel = self._pushbutton(
            "wplabel", self.toptray, icon='Draft_SelectPlane',
            hide=False,width=120)
        defaultWP = Draft.getParam("defaultWP",0)
        if defaultWP == 1:
            self.wplabel.setText(translate("draft","Top"))
        elif defaultWP == 2:
            self.wplabel.setText(translate("draft","Front"))
        elif defaultWP == 3:
            self.wplabel.setText(translate("draft","Side"))
        else:
            self.wplabel.setText(translate("draft","Auto"))
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
        bsize = p.GetInt("ToolbarIconSize",24)+2

        # OBSOLETE - replaced by style button
        #self.colorButton = self._pushbutton("colorButton",self.bottomtray, hide=False,square=True)
        #self.colorPix = QtGui.QPixmap(16,16)
        #self.colorPix.fill(self.color)
        #self.colorButton.setIcon(QtGui.QIcon(self.colorPix))
        #self.facecolorButton = self._pushbutton("facecolorButton",self.bottomtray, hide=False,square=True)
        #self.facecolorPix = QtGui.QPixmap(16,16)
        #self.facecolorPix.fill(QtGui.QColor(self.facecolor))
        #self.facecolorButton.setIcon(QtGui.QIcon(self.facecolorPix))
        #self.widthButton = self._spinbox("widthButton", self.bottomtray, val=self.linewidth,hide=False,size=(bsize * 2,bsize))
        #self.widthButton.setSuffix("px")
        #self.fontsizeButton = self._spinbox("fontsizeButton",self.bottomtray, val=self.fontsize,vmax=999, hide=False,double=True,size=(bsize * 4,bsize))
        #self.applyButton = self._pushbutton("applyButton", self.toptray, hide=False, icon='Draft_Apply',square=True)

        self.styleButton = self._pushbutton(
            "stylebutton", self.toptray, icon='Draft_Apply', hide=False,
            width=120)
        self.setStyleButton()
        self.constrButton = self._pushbutton(
            "constrButton", self.toptray, hide=False, icon='Draft_Construction',
             checkable=True, square=True)
        self.constrColor = QtGui.QColor(self.paramconstr)
        self.autoGroupButton = self._pushbutton(
            "autoGroup", self.bottomtray,icon=":/icons/button_invalid.svg",
            hide=False, width=120)
        self.autoGroupButton.setText(translate("draft", "None"))
        self.autoGroupButton.setFlat(True)

        QtCore.QObject.connect(self.wplabel,QtCore.SIGNAL("pressed()"),self.selectplane)
        QtCore.QObject.connect(self.styleButton,QtCore.SIGNAL("pressed()"),self.setstyle)
        QtCore.QObject.connect(self.constrButton,QtCore.SIGNAL("toggled(bool)"),self.toggleConstrMode)
        QtCore.QObject.connect(self.autoGroupButton,QtCore.SIGNAL("pressed()"),self.runAutoGroup)

        # OBSOLETE - replaced by style button
        #QtCore.QObject.connect(self.colorButton,QtCore.SIGNAL("pressed()"),self.getcol)
        #QtCore.QObject.connect(self.facecolorButton,QtCore.SIGNAL("pressed()"),self.getfacecol)
        #QtCore.QObject.connect(self.widthButton,QtCore.SIGNAL("valueChanged(int)"),self.setwidth)
        #QtCore.QObject.connect(self.fontsizeButton,QtCore.SIGNAL("valueChanged(double)"),self.setfontsize)
        #QtCore.QObject.connect(self.applyButton,QtCore.SIGNAL("pressed()"),self.apply)

        QtCore.QTimer.singleShot(2000,self.retranslateTray) # delay so translations get a chance to load

    def setupStyle(self):
        style = "#constrButton:Checked {background-color: "
        style += self.getDefaultColor("constr",rgb=True)+" } "
        style += "#addButton:Checked, #delButton:checked, "
        style += "#sharpButton:Checked, "
        style += "#tangentButton:Checked, #symmetricButton:checked {"
        style += "background-color: rgb(20,100,250) }"
        self.baseWidget.setStyleSheet(style)
        #if hasattr(self,"tray"):
        #    self.tray.setStyleSheet(style)


#---------------------------------------------------------------------------
# language tools
#---------------------------------------------------------------------------

    def retranslateUi(self, widget=None):
        self.promptlabel.setText(translate("draft", "active command:"))
        self.cmdlabel.setText(translate("draft", "None"))
        self.cmdlabel.setToolTip(translate("draft", "Active Draft command"))
        self.xValue.setToolTip(translate("draft", "X coordinate of next point"))
        self.labelx.setText(translate("draft", "X"))
        self.labely.setText(translate("draft", "Y"))
        self.labelz.setText(translate("draft", "Z"))
        self.yValue.setToolTip(translate("draft", "Y coordinate of next point"))
        self.zValue.setToolTip(translate("draft", "Z coordinate of next point"))
        self.pointButton.setText(translate("draft", "Enter point"))
        self.pointButton.setToolTip(translate(
            "draft","Enter a new point with the given coordinates"))
        self.labellength.setText(translate("draft", "Length"))
        self.labelangle.setText(translate("draft", "Angle"))
        self.lengthValue.setToolTip(translate("draft", "Length of current segment"))
        self.angleValue.setToolTip(translate("draft", "Angle of current segment"))
        #self.angleLock.setText(translate("draft", "&Lock"))
        self.angleLock.setToolTip(translate(
            "draft", "Check this to lock the current angle")\
            + " (" + inCommandShortcuts["Length"][0] + ")")
        self.labelRadius.setText(translate("draft", "Radius"))
        self.radiusValue.setToolTip(translate("draft", "Radius of Circle"))
        self.isRelative.setText(translate(
            "draft", "Relative") + " (" + inCommandShortcuts["Relative"][0] + ")")
        self.isRelative.setToolTip(translate(
            "draft", "Coordinates relative to last point or to coordinate system "
                     + "origin\nif is the first point to set"))
        self.isGlobal.setText(translate(
            "draft", "Global") + " (" + inCommandShortcuts["Global"][0] + ")")
        self.isGlobal.setToolTip(translate(
            "draft", "Coordinates relative to global coordinate system."
                     + "\nUncheck to use working plane coordinate system"))
        self.hasFill.setText(translate(
            "draft", "Filled")+" ("+inCommandShortcuts["Fill"][0]+")")
        self.hasFill.setToolTip(translate(
            "draft", "Check this if the object should appear as filled, "
                     + "otherwise it will appear as wireframe.\nNot available if "
                     + "Draft preference option 'Use Part Primitives' is enabled"))
        self.finishButton.setText(translate(
            "draft", "Finish")+" ("+inCommandShortcuts["Exit"][0]+")")
        self.finishButton.setToolTip(translate(
            "draft", "Finishes the current drawing or editing operation"))
        self.continueCmd.setToolTip(translate(
            "draft", "If checked, command will not finish until you press "
                     + "the command button again"))
        self.continueCmd.setText(translate(
            "draft", "Continue") + " (" + inCommandShortcuts["Continue"][0] + ")")
        self.occOffset.setToolTip(translate(
            "draft", "If checked, an OCC-style offset will be performed"
                     + "instead of the classic offset"))
        self.occOffset.setText(translate("draft", "&OCC-style offset"))

        # OBSOLETE
        # self.addButton.setToolTip(translate("draft", "Add points to the current object"))
        # self.delButton.setToolTip(translate("draft", "Remove points from the current object"))
        # self.sharpButton.setToolTip(translate("draft", "Make Bezier node sharp"))
        # self.tangentButton.setToolTip(translate("draft", "Make Bezier node tangent"))
        # self.symmetricButton.setToolTip(translate("draft", "Make Bezier node symmetric"))
        # self.arc3PtButton.setToolTip(translate("draft", "Toggle radius and angles arc editing"))

        self.undoButton.setText(translate("draft", "&Undo (CTRL+Z)"))
        self.undoButton.setToolTip(translate("draft", "Undo the last segment"))
        self.closeButton.setText(translate("draft", "Close")+" ("+inCommandShortcuts["Close"][0]+")")
        self.closeButton.setToolTip(translate("draft", "Finishes and closes the current line"))
        self.wipeButton.setText(translate("draft", "Wipe")+" ("+inCommandShortcuts["Wipe"][0]+")")
        self.wipeButton.setToolTip(translate("draft", "Wipes the existing segments of this line and starts again from the last point"))
        self.orientWPButton.setText(translate("draft", "Set WP")+" ("+inCommandShortcuts["SetWP"][0]+")")
        self.orientWPButton.setToolTip(translate("draft", "Reorients the working plane on the last segment"))
        self.selectButton.setText(translate("draft", "Select edge")+" ("+inCommandShortcuts["SelectEdge"][0]+")")
        self.selectButton.setToolTip(translate("draft", "Selects an existing edge to be measured by this dimension"))
        self.numFacesLabel.setText(translate("draft", "Sides"))
        self.numFaces.setToolTip(translate("draft", "Number of sides"))

        self.isCopy.setText(translate("draft", "Copy")+" ("+inCommandShortcuts["Copy"][0]+")")
        self.isCopy.setToolTip(translate("draft", "If checked, objects will be copied instead of moved. Preferences -> Draft -> Global copy mode to keep this mode in next commands"))
        self.isSubelementMode.setText(translate("draft", "Modify subelements")+" ("+inCommandShortcuts["SubelementMode"][0]+")")
        self.isSubelementMode.setToolTip(translate("draft", "If checked, subelements will be modified instead of entire objects"))
        self.SStringValue.setToolTip(translate("draft", "Text string to draw"))
        self.labelSString.setText(translate("draft", "String"))
        self.SSizeValue.setToolTip(translate("draft", "Height of text"))
        self.labelSSize.setText(translate("draft", "Height"))
        self.STrackValue.setToolTip(translate("draft", "Intercharacter spacing"))
        self.labelSTrack.setText(translate("draft", "Tracking"))
        self.labelFFile.setText(translate("draft", "Full path to font file:"))
        self.chooserButton.setToolTip(translate("draft", "Open a FileChooser for font file"))
        self.textOkButton.setText(translate("draft", "Create text"))
        self.textOkButton.setToolTip(translate("draft", "Press this button to create the text object, or finish your text with two blank lines"))
        self.retranslateTray(widget)

        # Update the maximum width of the push buttons
        maxwidth = 66 # that's the default
        pb = []
        for i in range(self.layout.count()):
            w = self.layout.itemAt(i).widget()
            if w != None and w.inherits('QPushButton'):
                pb.append(w)

        for i in pb:
            fm = QtGui.QFontMetrics(i.font())
            fw = fm.width(i.text())
            fw = max(fw, maxwidth)

        maxwidth = maxwidth + 16 +10 # add icon width and a margin
        for i in pb:
            i.setMaximumWidth(maxwidth)

    def retranslateTray(self,widget=None):

        self.wplabel.setToolTip(translate("draft", "Current working plane")+":"+self.wplabel.text())
        self.styleButton.setToolTip(translate("draft", "Change default style for new objects"))
        self.constrButton.setToolTip(translate("draft", "Toggle construction mode"))
        self.autoGroupButton.setToolTip(translate("draft", "Autogroup off"))

        # OBSOLETE - replaced by style button
        #self.colorButton.setToolTip(translate("draft", "Current line color"))
        #self.facecolorButton.setToolTip(translate("draft", "Current face color"))
        #self.widthButton.setToolTip(translate("draft", "Current line width"))
        #self.fontsizeButton.setToolTip(translate("draft", "Current font size"))
        #self.applyButton.setToolTip(translate("draft", "Apply to selected objects"))


#---------------------------------------------------------------------------
# Interface modes
#---------------------------------------------------------------------------

    def taskUi(self,title="Draft",extra=None,icon="Draft_Draft"):
        # reset InputField values
        self.reset_ui_values()
        if self.taskmode:
            self.isTaskOn = True
            todo.delay(FreeCADGui.Control.closeDialog,None)
            self.baseWidget = DraftBaseWidget()
            self.layout = QtGui.QVBoxLayout(self.baseWidget)
            self.setupToolBar(task=True)
            self.retranslateUi(self.baseWidget)
            self.panel = DraftTaskPanel(self.baseWidget,extra)
            todo.delay(FreeCADGui.Control.showDialog,self.panel)
        else: # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            # create a dummy task to block the UI during the works
            class dummy:
                """an empty dialog"""
                def __init__(self,extra=None):
                    if extra:
                        if isinstance(extra,list):
                            self.form = extra
                        else:
                            self.form = [extra]
                def getStandardButtons(self):
                    return int(QtGui.QDialogButtonBox.Close)
                def accept(self):
                    FreeCADGui.ActiveDocument.resetEdit()
                    return True
                def reject(self):
                    FreeCADGui.draftToolBar.isTaskOn = False
                    FreeCADGui.draftToolBar.escape()
                    FreeCADGui.ActiveDocument.resetEdit()
                    return True
                def isAllowedAlterDocument(self):
                    return False
            if FreeCADGui.Control.activeDialog():
                FreeCADGui.Control.closeDialog()
            todo.delay(FreeCADGui.Control.showDialog,dummy(extra))
        self.setTitle(title,icon)

    def redraw(self):
        """utility function that is performed after each clicked point"""
        self.checkLocal()

    def setFocus(self,f=None):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        if p.GetBool("focusOnLength",False) and self.lengthValue.isVisible():
            self.lengthValue.setFocus()
            self.lengthValue.setSelection(0,self.number_length(self.lengthValue.text()))
        elif self.angleLock.isVisible() and self.angleLock.isChecked():
            self.lengthValue.setFocus()
            self.lengthValue.setSelection(0,self.number_length(self.lengthValue.text()))
        elif (f is None) or (f == "x"):
            self.xValue.setFocus()
            self.xValue.setSelection(0,self.number_length(self.xValue.text()))
        elif f == "y":
            self.yValue.setFocus()
            self.yValue.setSelection(0,self.number_length(self.yValue.text()))
        elif f == "z":
            self.zValue.setFocus()
            self.zValue.setSelection(0,self.number_length(self.zValue.text()))
        elif f == "radius":
            self.radiusValue.setFocus()
            self.radiusValue.setSelection(0,self.number_length(self.radiusValue.text()))

    def number_length(self, str):
        nl = 0
        for char in str:
            if char in "0123456789.,-":
                nl += 1
        return nl

    def extraLineUi(self):
        '''shows length and angle controls'''
        self.labellength.show()
        self.lengthValue.show()
        self.labelangle.show()
        self.angleValue.show()
        self.angleLock.show()
        self.angleLock.setChecked(False)

    def hideXYZ(self):
        ''' turn off all the point entry widgets '''
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

    def lineUi(self, title=translate("draft", "Line"), cancel=None, extra=None,
               getcoords=None,rel=False,icon="Draft_Line"):
        self.pointUi(title, cancel, extra, getcoords, rel, icon)
        self.extraLineUi()
        self.xValue.setEnabled(True)
        self.yValue.setEnabled(True)
        self.continueCmd.show()

    def wireUi(self, title=translate("draft", "DWire"), cancel=None, extra=None,
               getcoords=None, rel=False, icon="Draft_Wire"):
        self.pointUi(title, cancel, extra, getcoords, rel, icon)
        self.xValue.setEnabled(True)
        self.yValue.setEnabled(True)
        if Draft.getParam("UsePartPrimitives",False):
            self.hasFill.setEnabled(False)
        else:
            self.hasFill.setEnabled(True)
        self.hasFill.show()
        self.finishButton.show()
        self.closeButton.show()
        self.wipeButton.show()
        self.orientWPButton.show()
        self.undoButton.show()
        self.continueCmd.show()

    def circleUi(self):
        self.pointUi(translate("draft", "Circle"),icon="Draft_Circle")
        self.extUi()
        self.isRelative.hide()

    def arcUi(self):
        self.pointUi(translate("draft", "Arc"),icon="Draft_Arc")
        self.continueCmd.show()
        self.isRelative.hide()

    def rotateSetCenterUi(self):
        self.pointUi(translate("draft", "Rotate"),icon="Draft_Rotate")
        self.modUi()
        self.isRelative.hide()

    def pointUi(self, title=translate("draft","Point"), cancel=None, extra=None,
                getcoords=None, rel=False, icon="Draft_Draft"):
        if cancel: self.cancel = cancel
        if getcoords: self.pointcallback = getcoords
        self.taskUi(title,extra,icon)
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
        self.xValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.yValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.zValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.x = 0
        self.y = 0
        self.z = 0
        self.pointButton.show()
        if rel: self.isRelative.show()
        todo.delay(self.setFocus,None)
        self.xValue.selectAll()

    def labelUi(self,title=translate("draft","Label"),callback=None):
        w = QtGui.QWidget()
        w.setWindowTitle(translate("draft","Label type", utf8_decode=True))
        l = QtGui.QVBoxLayout(w)
        combo = QtGui.QComboBox()
        from draftobjects.label import get_label_types
        types = get_label_types()
        for s in types:
            combo.addItem(s)
        combo.setCurrentIndex(types.index(Draft.getParam("labeltype","Custom")))
        l.addWidget(combo)
        QtCore.QObject.connect(combo,QtCore.SIGNAL("currentIndexChanged(int)"),callback)
        self.pointUi(title=title, extra=w, icon="Draft_Label")

    def extraUi(self):
        pass

    def offsetUi(self):
        self.taskUi(translate("draft","Offset"), icon="Draft_Offset")
        self.radiusUi()
        self.isCopy.show()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        self.isCopy.setChecked(p.GetBool("OffsetCopyMode",False))
        self.occOffset.show()
        self.labelRadius.setText(translate("draft","Distance"))
        self.radiusValue.setToolTip(translate("draft", "Offset distance"))
        self.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        todo.delay(self.radiusValue.setFocus,None)
        self.radiusValue.selectAll()

    def offUi(self):
        todo.delay(FreeCADGui.Control.closeDialog,None)
        self.cancel = None
        self.sourceCmd = None
        self.pointcallback = None
        self.mask = None
        if self.taskmode:
            self.isTaskOn = False
            self.baseWidget = QtGui.QWidget()
        else: # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            self.setTitle(translate("draft", "None"))
            self.labelx.setText(translate("draft", "X"))
            self.hideXYZ()
            self.numFacesLabel.hide()
            self.numFaces.hide()
            self.hasFill.hide()
            self.finishButton.hide()

            # OBSOLETE
            # self.addButton.hide()
            # self.delButton.hide()
            # self.sharpButton.hide()
            # self.tangentButton.hide()
            # self.symmetricButton.hide()
            # self.arc3PtButton.hide()

            self.undoButton.hide()
            self.closeButton.hide()
            self.wipeButton.hide()
            self.orientWPButton.hide()
            self.selectButton.hide()
            self.labelRadius.hide()
            self.radiusValue.hide()
            self.isCopy.hide()
            self.textValue.hide()
            self.textOkButton.hide()
            self.continueCmd.hide()
            self.occOffset.hide()
            self.labelSString.hide()
            self.SStringValue.hide()
            self.labelSSize.hide()
            self.SSizeValue.hide()
            self.labelSTrack.hide()
            self.STrackValue.hide()
            self.labelFFile.hide()
            self.FFileValue.hide()
            self.chooserButton.hide()


    def trimUi(self,title=translate("draft","Trimex")):
        self.taskUi(title, icon="Draft_Trimex")
        self.radiusUi()
        self.labelRadius.setText(translate("draft","Distance"))
        self.radiusValue.setToolTip(translate("draft", "Offset distance"))
        self.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        todo.delay(self.radiusValue.setFocus,None)
        self.radiusValue.selectAll()

    def radiusUi(self):
        self.hideXYZ()
        self.labelRadius.setText(translate("draft", "Radius"))
        self.radiusValue.setToolTip(translate("draft", "Radius of Circle"))
        self.labelRadius.show()
        self.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.radiusValue.show()
        todo.delay(self.radiusValue.setFocus,None)
        self.radiusValue.selectAll()

    def textUi(self):
        self.hideXYZ()
        self.textValue.show()
        self.textOkButton.show()
        self.textValue.setText('')
        todo.delay(self.textValue.setFocus,None)
        self.textbuffer=[]
        self.textline=0
        self.continueCmd.show()
        # Change the checkbox label as the in-command shortcut cannot be used:
        self.continueCmd.setText(translate("draft", "Continue"))

    def SSUi(self):
        ''' set up ui for ShapeString text entry '''
        self.hideXYZ()
        self.labelSString.show()
        self.SStringValue.show()
        self.SStringValue.setText('')
        todo.delay(self.SStringValue.setFocus,None)
        self.continueCmd.hide()

    def SSizeUi(self):
        ''' set up ui for ShapeString size entry '''
        self.labelSString.hide()
        self.SStringValue.hide()
        self.continueCmd.hide()
        self.labelSSize.show()
        self.SSizeValue.setText(FreeCAD.Units.Quantity(1,FreeCAD.Units.Length).UserString)
        self.SSizeValue.show()
        todo.delay(self.SSizeValue.setFocus,None)

    def STrackUi(self):
        ''' set up ui for ShapeString tracking entry '''
        self.labelSSize.hide()
        self.SSizeValue.hide()
        self.labelSTrack.show()
        self.STrackValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.STrackValue.show()
        todo.delay(self.STrackValue.setFocus,None)

    def SFileUi(self):
        ''' set up UI for ShapeString font file selection '''
        self.labelSTrack.hide()
        self.STrackValue.hide()
        if not self.FFileValue.text():
            self.FFileValue.setText(Draft.getParam("FontFile",""))
        self.labelFFile.show()
        self.FFileValue.show()
        self.chooserButton.show()
        todo.delay(self.FFileValue.setFocus,None)

    def switchUi(self,store=True):
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
                if self.state[0]:self.labelx.show()
                if self.state[1]:self.labely.show()
                if self.state[2]:self.labelz.show()
                if self.state[3]:self.xValue.show()
                if self.state[4]:self.yValue.show()
                if self.state[5]:self.zValue.show()
                self.state = None

    def setTitle(self,title,icon="Draft_Draft"):
        if self.taskmode:
            self.baseWidget.setWindowTitle(title)
            self.baseWidget.setWindowIcon(QtGui.QIcon(":/icons/"+icon+".svg"))
        else: # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            self.cmdlabel.setText(title)

    def selectUi(self,extra=None, on_close_call=None):
        if not self.taskmode:
             # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            self.labelx.setText(translate("draft", "Pick Object"))
            self.labelx.show()
        self.makeDumbTask(extra, on_close_call)

    def editUi(self, mode=None):
        self.lastMode=mode
        self.taskUi(translate("draft", "Edit"))
        self.hideXYZ()
        self.numFaces.hide()
        self.hasFill.hide()
        # self.addButton.show()
        # self.delButton.show()
        # if mode == 'Wire':
        #     self.setEditButtons(True)
        #     self.setBezEditButtons(False)
        # elif mode == 'Arc':
        #     self.addButton.hide()
        #     self.delButton.hide()
        #     self.arc3PtButton.show()
        # elif mode == 'BezCurve':
        #     self.sharpButton.show()
        #     self.tangentButton.show()
        #     self.symmetricButton.show()
        self.closeButton.show()
        # self.finishButton.show()
        # always start Edit with buttons unchecked
        # self.addButton.setChecked(False)
        # self.delButton.setChecked(False)
        # self.sharpButton.setChecked(False)
        # self.tangentButton.setChecked(False)
        # self.symmetricButton.setChecked(False)
        # self.arc3PtButton.setChecked(False)

    def extUi(self):
        if Draft.getParam("UsePartPrimitives",False):
            self.hasFill.setEnabled(False)
        else:
            self.hasFill.setEnabled(True)
        self.hasFill.show()
        self.continueCmd.show()

    def modUi(self):
        self.isCopy.show()
        self.isSubelementMode.show()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        if p.GetBool("copymode",False):
            self.isCopy.setChecked(p.GetBool("copymodeValue",False))
        self.continueCmd.show()

    # def vertUi(self,addmode=True):
    #     self.addButton.setChecked(addmode)
    #     self.delButton.setChecked(not(addmode))

    def checkLocal(self):
        """checks if x,y,z coords must be displayed as local or global"""
        if not self.globalMode and self.relativeMode:
            self.labelx.setText(translate("draft", "Local \u0394X"))
            self.labely.setText(translate("draft", "Local \u0394Y"))
            self.labelz.setText(translate("draft", "Local \u0394Z"))
        elif not self.globalMode and not self.relativeMode:
            self.labelx.setText(translate("draft", "Local X"))
            self.labely.setText(translate("draft", "Local Y"))
            self.labelz.setText(translate("draft", "Local Z"))
        elif self.globalMode and self.relativeMode:
            self.labelx.setText(translate("draft", "Global \u0394X"))
            self.labely.setText(translate("draft", "Global \u0394Y"))
            self.labelz.setText(translate("draft", "Global \u0394Z"))
        else:
            self.labelx.setText(translate("draft", "Global X"))
            self.labely.setText(translate("draft", "Global Y"))
            self.labelz.setText(translate("draft", "Global Z"))

    # def setEditButtons(self,mode):
    #     self.addButton.setEnabled(mode)
    #     self.delButton.setEnabled(mode)

    # def setBezEditButtons(self,mode):
    #     self.sharpButton.setEnabled(mode)
    #     self.tangentButton.setEnabled(mode)
    #     self.symmetricButton.setEnabled(mode)

    def setNextFocus(self):
        def isThere(widget):
            if widget.isEnabled() and widget.isVisible():
                return True
            else:
                return False
        if (not self.taskmode) or self.isTaskOn:
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

    def setRelative(self,val=1):
        self.relativeMode = bool(val)
        self.checkLocal()

    def setGlobal(self,val=0):
        self.globalMode = bool(val)
        self.checkLocal()

    def setCopymode(self,val=0):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        p.SetBool("copymodeValue",bool(val))
        # special value for offset command
        if self.sourceCmd:
            if self.sourceCmd.featureName == "Offset":
                p.SetBool("OffsetCopyMode",bool(val))

    def setSubelementMode(self):
        self.sourceCmd.set_ghosts()

    def relocate(self):
        """relocates the right-aligned buttons depending on the toolbar size"""
        if self.baseWidget.geometry().width() < 400:
            self.layout.setDirection(QtGui.QBoxLayout.TopToBottom)
        else:
            self.layout.setDirection(QtGui.QBoxLayout.LeftToRight)

    def makeDumbTask(self, extra=None, on_close_call=None):
        """create a dumb taskdialog to prevent deleting the temp object"""
        class TaskPanel:
            def __init__(self, extra=None, callback=None):
                if extra:
                    self.form = [extra]
                self.callback = callback
            def getStandardButtons(self):
                return int(QtGui.QDialogButtonBox.Close)
            def reject(self):
                if self.callback:
                    self.callback()
                return True
        FreeCADGui.Control.closeDialog()
        panel = TaskPanel(extra, on_close_call)
        FreeCADGui.Control.showDialog(panel)


#---------------------------------------------------------------------------
# Processing functions
#---------------------------------------------------------------------------

    def getcol(self):
        """opens a color picker dialog"""
        print("draft: warning: getcol() is obsolete")
        return
        oldColor = self.color
        self.color=QtGui.QColorDialog.getColor(self.color)
        if not QtGui.QColor.isValid(self.color): #user canceled
            self.color = oldColor
            return
        self.colorPix.fill(self.color)
        self.colorButton.setIcon(QtGui.QIcon(self.colorPix))
        if Draft.getParam("saveonexit",False):
            Draft.setParam("color",self.color.rgb()<<8)
        r = float(self.color.red()/255.0)
        g = float(self.color.green()/255.0)
        b = float(self.color.blue()/255.0)
        col = (r,g,b,0.0)
        for i in FreeCADGui.Selection.getSelection():
            if (i.TypeId == "App::Annotation"):
                i.ViewObject.TextColor=col
            else:
                if "LineColor" in i.ViewObject.PropertiesList:
                    i.ViewObject.LineColor = col
                if "PointColor" in i.ViewObject.PropertiesList:
                    i.ViewObject.PointColor = col

    def getfacecol(self):
        """opens a color picker dialog"""
        print("draft: warning: getfacecol() is obsolete")
        return
        oldColor = self.facecolor
        self.facecolor=QtGui.QColorDialog.getColor(self.facecolor)
        if not QtGui.QColor.isValid(self.facecolor): #user canceled
            self.facecolor = oldColor
            return
        self.facecolorPix.fill(self.facecolor)
        self.facecolorButton.setIcon(QtGui.QIcon(self.facecolorPix))
        if Draft.getParam("saveonexit",False):
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")\
            .SetUnsigned("DefaultShapeColor",self.facecolor.rgb()<<8)
        r = float(self.facecolor.red()/255.0)
        g = float(self.facecolor.green()/255.0)
        b = float(self.facecolor.blue()/255.0)
        col = (r,g,b,0.0)
        for i in FreeCADGui.Selection.getSelection():
            if "ShapeColor" in i.ViewObject.PropertiesList:
                i.ViewObject.ShapeColor = col

    def setwidth(self,val):
        print("draft: warning: setwidth() is obsolete")
        return
        self.linewidth = float(val)
        if Draft.getParam("saveonexit",False):
            Draft.setParam("linewidth",int(val))
        for i in FreeCADGui.Selection.getSelection():
            if "LineWidth" in i.ViewObject.PropertiesList:
                i.ViewObject.LineWidth = float(val)

    def setfontsize(self,val):
        print("draft: warning: setfontsize() is obsolete")
        return
        self.fontsize = float(val)
        if Draft.getParam("saveonexit",False):
            Draft.setParam("textheight",float(val))
        for i in FreeCADGui.Selection.getSelection():
            if "FontSize" in i.ViewObject.PropertiesList:
                i.ViewObject.FontSize = float(val)

    def setContinue(self,val):
        if self.continueCmd.isVisible():
            self.continueMode = bool(val)

    def setFill(self,val):
        if self.hasFill.isVisible():
            self.fillmode = bool(val)

    def apply(self):
        print("draft: warning: apply() is obsolete")
        return
        for i in FreeCADGui.Selection.getSelection():
            Draft.formatObject(i)

    def checkx(self):
        if self.yValue.isEnabled():
            self.yValue.setFocus()
            self.yValue.setSelection(0,self.number_length(self.yValue.text()))
            self.updateSnapper()
        else:
            self.checky()

    def checky(self):
        if self.zValue.isEnabled():
            self.zValue.setFocus()
            self.zValue.setSelection(0,self.number_length(self.zValue.text()))
            self.updateSnapper()
        else:
            self.validatePoint()

    def checklength(self):
        if self.angleValue.isEnabled():
            self.angleValue.setFocus()
            self.angleValue.setSelection(0,self.number_length(self.angleValue.text()))
            self.updateSnapper()
        else:
            self.validatePoint()

    def validatePoint(self):
        """function for checking and sending numbers entered manually"""
        if self.sourceCmd or self.pointcallback:
            if (self.labelRadius.isVisible()):
                try:
                    #rad=float(self.radiusValue.text())
                    rad = self.radius
                except (ValueError, AttributeError):
                    print("debug: DraftGui.validatePoint: AttributeError")
                else:
                    self.sourceCmd.numericRadius(rad)
            elif (self.labelx.isVisible()):
                try:
                    #numx=float(self.xValue.text())
                    numx = self.x
                    #numy=float(self.yValue.text())
                    numy = self.y
                    #numz=float(self.zValue.text())
                    numz = self.z
                except (ValueError, AttributeError):
                    print("debug: DraftGui.validatePoint: AttributeError")
                else:
                    num_vec = FreeCAD.Vector(numx, numy, numz)
                    ref_vec = FreeCAD.Vector(0,0,0)
                    if self.pointcallback:
                        self.pointcallback(num_vec, self.relativeMode)
                    else:
                        if FreeCAD.DraftWorkingPlane and not self.globalMode:
                            num_vec = FreeCAD.DraftWorkingPlane.getGlobalRot(num_vec)
                            ref_vec = FreeCAD.DraftWorkingPlane.getGlobalCoords(ref_vec)
                        if self.relativeMode:
                            if self.sourceCmd.node:
                                ref_vec = self.sourceCmd.node[-1]

                        numx, numy, numz = num_vec + ref_vec
                        self.sourceCmd.numericInput(numx,numy,numz)

            elif (self.textValue.isVisible() or self.SStringValue.isVisible()
                  or self.SSizeValue.isVisible() or self.STrackValue.isVisible()
                  or self.FFileValue.isVisible()):
                return False
            else:
                FreeCADGui.ActiveDocument.resetEdit()
        return True

    def validateSNumeric(self):
        ''' send valid numeric parameters to ShapeString '''
        if self.sourceCmd:
            if (self.labelSSize.isVisible()):
                try:
                    SSize=float(self.SSize)
                except ValueError:
                    FreeCAD.Console.PrintMessage(translate("draft", "Invalid Size value. Using 200.0."))
                    self.sourceCmd.numericSSize(200.0)
                else:
                    self.sourceCmd.numericSSize(SSize)
            elif (self.labelSTrack.isVisible()):
                try:
                    track=int(self.STrack)
                except ValueError:
                    FreeCAD.Console.PrintMessage(translate("draft", "Invalid Tracking value. Using 0."))
                    self.sourceCmd.numericSTrack(0)
                else:
                    self.sourceCmd.numericSTrack(track)

    def validateSString(self):
        ''' send a valid text string to ShapeString as unicode '''
        if self.sourceCmd:
            if (self.labelSString.isVisible()):
                if self.SStringValue.text():
                    #print("debug: D_G DraftToolBar.validateSString type(SStringValue.text): "  str(type(self.SStringValue.text)))
                    #self.sourceCmd.validSString(str(self.SStringValue.text()))    # QString to QByteArray to PyString
                    self.sourceCmd.validSString(self.SStringValue.text())    # PySide returns Unicode from QString
                else:
                    FreeCAD.Console.PrintMessage(translate("draft", "Please enter a text string."))


    def pickFile(self):
        ''' invoke a font file chooser dialog and send result to ShapeString to'''
        if self.sourceCmd:
            if (self.chooserButton.isVisible()):
                try:
                    dialogCaption = translate("draft", "Select a Font file")
                    dialogDir = os.path.dirname(Draft.getParam("FontFile",)) # reasonable default?
                    dialogFilter = "Fonts (*.ttf *.pfb *.otf);;All files (*.*)"
                    fname = QtGui.QFileDialog.getOpenFileName(self.baseWidget,
                                                              dialogCaption,
                                                              dialogDir,
                                                              dialogFilter)
                    # fname = utf8_decode(fname[0])  # 1947: utf8_decode fails ('ascii' codec can't encode character)
                                                    # when fname[0] contains accented chars
                    fname = fname[0].encode('utf8') #TODO: this needs changing for Py3??
                                                    # accented chars cause "UnicodeEncodeError" failure in DraftGui.todo without
                                                    # .encode('utf8')

                except Exception as e:
                    FreeCAD.Console.PrintMessage("DraftGui.pickFile: unable to select a font file.")
                    print(type(e))
                    print(e.args)
                else:
                    if fname[0]:
                        self.FFileValue.setText(fname)
                        self.sourceCmd.validFFile(fname)
                    else:
                        FreeCAD.Console.PrintMessage("DraftGui.pickFile: no file selected.")   # can this happen?

    def validateFile(self):
        ''' check and send font file parameter to ShapeString as unicode'''
        if self.sourceCmd:
            if (self.labelFFile.isVisible()):
                if self.FFileValue.text():
                    # QString to PyString
                    self.sourceCmd.validFFile(
                        utf8_decode(self.FFileValue.text()))
                else:
                    FreeCAD.Console.PrintMessage(
                        translate("draft", "Please enter a font file."))


    def finish(self):
        """finish button action"""
        if self.sourceCmd:
            self.sourceCmd.finish(False)
        if self.cancel:
            self.cancel()
            self.cancel = None
        if FreeCADGui.ActiveDocument:
            FreeCADGui.ActiveDocument.resetEdit()

    def escape(self):
        """escapes the current command"""
        self.continueMode = False
        if not self.taskmode:
            # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            self.continueCmd.setChecked(False)
        self.finish()

    def closeLine(self):
        """close button action"""
        self.sourceCmd.finish(True)
        FreeCADGui.ActiveDocument.resetEdit()

    def wipeLine(self):
        """wipes existing segments of a line"""
        self.sourceCmd.wipe()

    def orientWP(self):
        """reorients the current working plane"""
        self.sourceCmd.orientWP()

    def selectEdge(self):
        """allows the dimension command to select an edge"""
        if hasattr(self.sourceCmd,"selectEdge"):
            self.sourceCmd.selectEdge()

    def undoSegment(self):
        """undo last line segment"""
        if hasattr(self.sourceCmd,"undolast"):
            self.sourceCmd.undolast()

    def checkSpecialChars(self,txt):
        """checks for special characters in the entered coords that must be
        treated as shortcuts
        """

        # in-command shortcut definitions
        #Relative
        #Continue
        #Close
        #Copy
        #Fill
        #Exit
        #Snap
        #Increase
        #Decrease
        #RestrictX
        #RestrictY
        #RestrictZ
        #SelectEdge
        #AddHold
        #Length
        #Wipe
        #SetWP

        spec = False
        if txt.upper().startswith(inCommandShortcuts["Relative"][0]):
            if self.isRelative.isVisible():
                self.isRelative.setChecked(not self.isRelative.isChecked())
                self.relativeMode = self.isRelative.isChecked()
            spec = True
        if txt.upper().startswith(inCommandShortcuts["Global"][0]):
            if self.isGlobal.isVisible():
                self.isGlobal.setChecked(not self.isGlobal.isChecked())
                self.globalMode = self.isGlobal.isChecked()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["Fill"][0]):
            if self.hasFill.isVisible():
                self.hasFill.setChecked(not self.hasFill.isChecked())
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["Exit"][0]):
            if self.finishButton.isVisible():
                self.finish()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["Continue"][0]):
            if self.continueCmd.isVisible():
                self.toggleContinue()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["Wipe"][0]):
            if self.wipeButton.isVisible():
                self.wipeLine()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["SelectEdge"][0]):
            self.selectEdge()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["Snap"][0]):
            self.togglesnap()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["NearSnap"][0]):
            self.togglenearsnap()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["Increase"][0]):
            self.toggleradius(1)
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["Decrease"][0]):
            self.toggleradius(-1)
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["AddHold"][0]):
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.addHoldPoint()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["RestrictX"][0]):
            self.constrain("x")
            self.displayPoint()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["RestrictY"][0]):
            self.constrain("y")
            self.displayPoint()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["RestrictZ"][0]):
            self.constrain("z")
            self.displayPoint()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["Length"][0]):
            self.constrain("angle")
            self.displayPoint()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["Close"][0]):
            if self.closeButton.isVisible():
                self.closeLine()
        elif txt.upper().startswith(inCommandShortcuts["SetWP"][0]):
            if self.orientWPButton.isVisible():
                self.orientWP()
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["Copy"][0]):
            if self.isCopy.isVisible():
                self.isCopy.setChecked(not self.isCopy.isChecked())
            spec = True
        elif txt.upper().startswith(inCommandShortcuts["SubelementMode"][0]):
            if self.isSubelementMode.isVisible():
                self.isSubelementMode.setChecked(not self.isSubelementMode.isChecked())
            spec = True
        if spec:
            widget = self.baseWidget.focusWidget()
            field = self.input_fields[widget.objectName()]
            value = getattr(self, field["value"])
            unit = getattr(FreeCAD.Units, field["unit"])
            v = FreeCAD.Units.Quantity(value, unit).getUserPreferred()[0]
            widget.setProperty("text",v)
            widget.setFocus()
            widget.selectAll()
        self.updateSnapper()

    def updateSnapper(self):
        """updates the snapper track line if applicable"""
        if hasattr(FreeCADGui,"Snapper"):
            if FreeCADGui.Snapper.trackLine:
                if FreeCADGui.Snapper.trackLine.Visible:
                    last = FreeCAD.Vector(0,0,0)
                    if not self.xValue.isVisible():
                        return
                    if self.isRelative.isChecked():
                        if self.sourceCmd:
                            if hasattr(self.sourceCmd,"node"):
                                if self.sourceCmd.node:
                                    last = self.sourceCmd.node[-1]
                    delta = FreeCAD.DraftWorkingPlane.getGlobalCoords(
                        FreeCAD.Vector(self.x,self.y,self.z))
                    FreeCADGui.Snapper.trackLine.p2(last.add(delta))

    def checkEnterText(self):
        """this function checks if the entered text ends with two blank lines"""
        t = self.textValue.toPlainText()
        if t.endswith("\n\n"):
            self.sendText()

    def sendText(self):
        """this function sends the entered text to the active draft command
        if enter has been pressed twice. Otherwise it blanks the line.
        """
        self.sourceCmd.text = self.textValue.toPlainText()\
            .replace("\\","\\\\")\
            .replace("\"","\\\"")\
            .replace("\'","\\\'")\
            .splitlines()
        self.sourceCmd.createObject()

    def displayPoint(self, point=None, last=None, plane=None, mask=None):
        """this function displays the passed coords in the x, y, and z widgets"""
        if self.taskmode and (not self.isTaskOn):
            return

        if not plane:
            plane = FreeCAD.DraftWorkingPlane
        # get coords to display
        if not last:
            if self.globalMode:
                last = FreeCAD.Vector(0,0,0)
            else:
                last = plane.getPlacement().Base
        dp = None
        if point:
            dp = point
            if self.relativeMode: # and (last != None):
                if self.globalMode:
                    dp = point - last
                else:
                    dp = plane.getLocalRot(point - last)
            else:
                if self.globalMode:
                    dp = point
                else:
                    dp = plane.getLocalCoords(point)
        # set widgets
        if dp:
            if self.mask in ['y','z']:
                self.xValue.setText(displayExternal(dp.x,None,'Length'))
            else:
                self.xValue.setText(displayExternal(dp.x,None,'Length'))
            if self.mask in ['x','z']:
                self.yValue.setText(displayExternal(dp.y,None,'Length'))
            else:
                self.yValue.setText(displayExternal(dp.y,None,'Length'))
            if self.mask in ['x','y']:
                self.zValue.setText(displayExternal(dp.z,None,'Length'))
            else:
                self.zValue.setText(displayExternal(dp.z,None,'Length'))

        # set length and angle
        if last and dp and plane:
            length, theta, phi = DraftVecUtils.get_spherical_coords(*dp)
            theta = math.degrees(theta)
            phi = math.degrees(phi)
            self.lengthValue.setText(displayExternal(length,None,'Length'))
            #if not self.angleLock.isChecked():
            self.angleValue.setText(displayExternal(phi,None,'Angle'))
            if not mask:
                # automask, phi is rounded to identify one of the below cases
                phi = round(phi, Draft.getParam("precision"))
                if phi in [0,180,-180]:
                    mask = "x"
                elif phi in [90,270,-90,-270]:
                    mask = "y"

        # set masks
        if (mask == "x") or (self.mask == "x"):
            self.xValue.setEnabled(True)
            self.yValue.setEnabled(False)
            self.zValue.setEnabled(False)
            self.angleValue.setEnabled(False)
            self.setFocus()
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


    def getDefaultColor(self,type,rgb=False):
        """gets color from the preferences or toolbar"""
        r = 0
        g = 0
        b = 0
        if type == "snap":
            color = Draft.getParam("snapcolor",4294967295)
            r = ((color>>24)&0xFF)/255
            g = ((color>>16)&0xFF)/255
            b = ((color>>8)&0xFF)/255
        elif type == "ui":
            print("draft: deprecation warning: Do not use getDefaultColor(\"ui\") anymore - use getDefaultColor(\"line\") instead.")
            r = float(self.color.red()/255.0)
            g = float(self.color.green()/255.0)
            b = float(self.color.blue()/255.0)
        elif type == "line":
            color = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")\
                .GetUnsigned("DefaultShapeLineColor",255)
            r = ((color>>24)&0xFF)/255
            g = ((color>>16)&0xFF)/255
            b = ((color>>8)&0xFF)/255
        elif type == "text":
            color = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")\
                .GetUnsigned("DefaultTextColor",255)
            r = ((color>>24)&0xFF)/255
            g = ((color>>16)&0xFF)/255
            b = ((color>>8)&0xFF)/255
        elif type == "face":
            color = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")\
                .GetUnsigned("DefaultShapeColor",4294967295)
            r = ((color>>24)&0xFF)/255
            g = ((color>>16)&0xFF)/255
            b = ((color>>8)&0xFF)/255
        elif type == "constr":
            color = Draft.getParam("constructioncolor",746455039)
            r = ((color>>24)&0xFF)/255
            g = ((color>>16)&0xFF)/255
            b = ((color>>8)&0xFF)/255
        else:
            print("draft: error: couldn't get a color for ",type," type.")
        if rgb:
            return("rgb("+str(int(r*255))+","+str(int(g*255))+","+str(int(b*255))+")")
        else:
            return (r,g,b)

    def cross(self,on=True):
        """deprecated"""
        pass

    def toggleConstrMode(self,checked):
        self.baseWidget.setStyleSheet(
            "#constrButton:Checked {background-color: "
            + self.getDefaultColor("constr",rgb=True)+" }")
        self.constrMode = checked

    def toggleContinue(self):
        FreeCAD.Console.PrintMessage("toggle continue\n")
        self.continueMode = not self.continueMode
        try:
            if hasattr(self,"continueCmd"):
                if self.continueCmd.isVisible():
                    self.continueCmd.toggle()
            if hasattr(self,"panel"):
                if hasattr(self.panel,"form"):
                    if isinstance(self.panel.form,list):
                        for w in self.panel.form:
                            c = w.findChild(QtGui.QCheckBox,"ContinueCmd")
                            if c:
                                c.toggle()
                    else:
                        c = self.panel.form.findChild(QtGui.QCheckBox,"ContinueCmd")
                        if c:
                            c.toggle()
        except Exception:
            pass

    def isConstructionMode(self):
        if self.tray or (not self.taskmode):
            return self.constrButton.isChecked()
        else:
            return False

    def drawPage(self):
        self.sourceCmd.draw()

    def changePage(self,index):
        pagename = str(self.pageBox.itemText(index))
        vobj = FreeCADGui.ActiveDocument.getObject(pagename)
        if vobj:
            self.scaleBox.setEditText(str(vobj.HintScale))
            self.marginXValue.setValue(float(vobj.HintOffsetX))
            self.marginYValue.setValue(float(vobj.HintOffsetY))

    def selectplane(self):
        FreeCADGui.runCommand("Draft_SelectPlane")

    def setstyle(self):
        FreeCADGui.runCommand("Draft_SetStyle")

    def setStyleButton(self):
        "sets icon and text on the style button"
        linecolor = QtGui.QColor(Draft.getParam("color",255)>>8)
        facecolor = QtGui.QColor(
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
            .GetUnsigned( "DefaultShapeColor",4294967295)>>8)
        im = QtGui.QImage(32,32,QtGui.QImage.Format_ARGB32)
        im.fill(QtCore.Qt.transparent)
        pt = QtGui.QPainter(im)
        pt.setPen(QtGui.QPen(QtCore.Qt.black, 1, QtCore.Qt.SolidLine, QtCore.Qt.FlatCap))
        pt.setBrush(QtGui.QBrush(linecolor, QtCore.Qt.SolidPattern))
        pts = [QtCore.QPointF(4.0,4.0), QtCore.QPointF(4.0,26.0), QtCore.QPointF(26.0,4.0)]
        pt.drawPolygon(pts, QtCore.Qt.OddEvenFill)
        pt.setBrush(QtGui.QBrush(facecolor, QtCore.Qt.SolidPattern))
        pts = [QtCore.QPointF(28.0,28.0),QtCore.QPointF(8.0,28.0),QtCore.QPointF(28.0,8.0)]
        pt.drawPolygon(pts,QtCore.Qt.OddEvenFill)
        pt.end()
        icon = QtGui.QIcon(QtGui.QPixmap.fromImage(im))
        linewidth = Draft.getParam("linewidth",2)
        fontsize =  Draft.getParam("textheight",0.20)
        txt = str(linewidth) + "px | "\
            + FreeCAD.Units.Quantity(fontsize,FreeCAD.Units.Length).UserString
        self.styleButton.setIcon(icon)
        self.styleButton.setText(txt)

        # FOR BACKWARDS COMPATIBILITY
        self.color = linecolor
        self.facecolor = facecolor
        self.linewidth = linewidth
        self.fontsize = fontsize

    def popupMenu(self,llist,ilist=None,pos=None):
        """pops up a menu filled with the given list"""
        self.groupmenu = QtGui.QMenu()
        for i,l in enumerate(llist):
            if ilist:
                self.groupmenu.addAction(ilist[i],l)
            else:
                self.groupmenu.addAction(l)
        if not pos:
            pos = FreeCADGui.getMainWindow().cursor().pos()
        self.groupmenu.popup(pos)
        QtCore.QObject.connect(self.groupmenu,QtCore.SIGNAL("triggered(QAction *)"),self.popupTriggered)

    def getIcon(self,iconpath):
        return QtGui.QIcon(iconpath)

    def popupTriggered(self,action):
        self.sourceCmd.proceed(str(action.text()))

    # def setAddMode(self,bool):
    #     if self.addButton.isChecked():
    #         self.delButton.setChecked(False)
    #         self.symmetricButton.setChecked(False)
    #         self.sharpButton.setChecked(False)
    #         self.tangentButton.setChecked(False)

    # def setDelMode(self,bool):
    #     if self.delButton.isChecked():
    #         self.addButton.setChecked(False)
    #         self.symmetricButton.setChecked(False)
    #         self.sharpButton.setChecked(False)
    #         self.tangentButton.setChecked(False)

    # def setSharpMode(self,bool):
    #     if self.sharpButton.isChecked():
    #         self.tangentButton.setChecked(False)
    #         self.symmetricButton.setChecked(False)
    #         self.addButton.setChecked(False)
    #         self.delButton.setChecked(False)

    # def setTangentMode(self,bool):
    #     if self.tangentButton.isChecked():
    #         self.sharpButton.setChecked(False)
    #         self.symmetricButton.setChecked(False)
    #         self.addButton.setChecked(False)
    #         self.delButton.setChecked(False)

    # def setSymmetricMode(self,bool):
    #     if self.symmetricButton.isChecked():
    #         self.sharpButton.setChecked(False)
    #         self.tangentButton.setChecked(False)
    #         self.addButton.setChecked(False)
    #         self.delButton.setChecked(False)

    # def setArc3PtMode(self,bool):
    #     if self.arc3PtButton.isChecked():
    #         self.arc3PtButton.setChecked(True)

    def setRadiusValue(self,val,unit=None):
        #print("DEBUG: setRadiusValue val: ", val, " unit: ", unit)
        if  not isinstance(val, (int, float)):       #??some code passes strings or ???
            t = val
        elif unit:
            t= displayExternal(val,None, unit)
        else:
            print("Error: setRadiusValue called for number without Dimension")
            t = displayExternal(val,None, None)
        self.radiusValue.setText(t)
        self.radiusValue.setFocus()

    def runAutoGroup(self):
        FreeCADGui.runCommand("Draft_AutoGroup")

    def setAutoGroup(self,value=None):
        if value is None:
            self.autogroup = None
            self.autoGroupButton.setText(translate("draft", "None"))
            self.autoGroupButton.setIcon(QtGui.QIcon.fromTheme('Draft_AutoGroup_off',
                                                               QtGui.QIcon(':/icons/button_invalid.svg')))
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
                self.autoGroupButton.setIcon(QtGui.QIcon.fromTheme('Draft_AutoGroup_off',
                                                                   QtGui.QIcon(':/icons/button_invalid.svg')))
                self.autoGroupButton.setToolTip(translate("draft", "Autogroup off"))
                self.autoGroupButton.setDown(False)

    def show(self):
        if not self.taskmode:
            # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            self.draftWidget.setVisible(True)

    def hide(self):
        if not self.taskmode:
            # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            self.draftWidget.setVisible(False)

    def getXPM(self,iconname,size=16):
        i = QtGui.QIcon(":/icons/"+iconname+".svg")
        p = i.pixmap(size,size)
        a = QtCore.QByteArray()
        b = QtCore.QBuffer(a)
        b.open(QtCore.QIODevice.WriteOnly)
        p.save(b,"XPM")
        b.close()
        return str(a)

    def togglesnap(self):
        FreeCADGui.doCommand('FreeCADGui.runCommand("Draft_Snap_Lock")')

    def togglenearsnap(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonpassive":
                        b.toggle()

    def toggleradius(self,val):
        if hasattr(FreeCADGui,"Snapper"):
            par = Draft.getParam("snapRange", 8)
            Draft.setParam("snapRange", max(0, par+val))
            FreeCADGui.Snapper.showradius()

    def constrain(self,val):
        if val == "angle":
            self.alock = not(self.alock)
            self.angleLock.setChecked(self.alock)
        elif self.mask == val:
            self.mask = None
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.mask = None
        else:
            self.mask = val
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.mask = val

    def changeXValue(self,d):
        self.x = d
        if not self.xValue.hasFocus():
            return None
        self.update_spherical_coords()

    def changeYValue(self,d):
        self.y = d
        if not self.yValue.hasFocus():
            return None
        self.update_spherical_coords()

    def changeZValue(self,d):
        self.z = d
        if not self.zValue.hasFocus():
            return None
        self.update_spherical_coords()

    def changeRadiusValue(self,d):
        self.radius = d

    def changeSSizeValue(self,d):
        self.SSize = d

    def changeSTrackValue(self,d):
        self.STrack = d

    def changeLengthValue(self,d):
        self.lvalue = d
        if not self.lengthValue.hasFocus():
            return None
        self.update_cartesian_coords()

    def changeAngleValue(self,d):
        self.avalue = d
        if not self.angleValue.hasFocus():
            return None
        self.update_cartesian_coords()
        if self.angleLock.isChecked():
            if not self.globalMode:
                angle_vec = FreeCAD.DraftWorkingPlane.getGlobalRot(self.angle)
            else:
                angle_vec = self.angle
            FreeCADGui.Snapper.setAngle(angle_vec)

    def toggleAngle(self,b):
        self.alock = self.angleLock.isChecked()
        if b:
            if not self.globalMode:
                angle_vec = FreeCAD.DraftWorkingPlane.getGlobalRot(self.angle)
            else:
                angle_vec = self.angle
            FreeCADGui.Snapper.setAngle(angle_vec)
        else:
            FreeCADGui.Snapper.setAngle()
            self.angle = None

    def update_spherical_coords(self):
        length, theta, phi = DraftVecUtils.get_spherical_coords(
            self.x,self.y,self.z)
        self.lvalue = length
        self.pvalue = math.degrees(theta)
        self.avalue = math.degrees(phi)
        self.angle = FreeCAD.Vector(DraftVecUtils.get_cartesian_coords(
            1, theta, phi))
        self.lengthValue.setText(displayExternal(self.lvalue,None,'Length'))
        self.angleValue.setText(displayExternal(self.avalue,None,'Angle'))

    def update_cartesian_coords(self):
        self.x, self.y, self.z = DraftVecUtils.get_cartesian_coords(
            self.lvalue,math.radians(self.pvalue),math.radians(self.avalue))
        self.angle = FreeCAD.Vector(DraftVecUtils.get_cartesian_coords(
            1, math.radians(self.pvalue), math.radians(self.avalue)))
        self.xValue.setText(displayExternal(self.x,None,'Length'))
        self.yValue.setText(displayExternal(self.y,None,'Length'))
        self.zValue.setText(displayExternal(self.z,None,'Length'))

#---------------------------------------------------------------------------
# TaskView operations
#---------------------------------------------------------------------------

    def setWatchers(self):
        class DraftCreateWatcher:
            def __init__(self):
                self.commands = ["Draft_Line","Draft_Wire",
                                 "Draft_Rectangle","Draft_Arc",
                                 "Draft_Circle","Draft_BSpline",
                                 "Draft_Text","Draft_Dimension",
                                 "Draft_ShapeString","Draft_BezCurve"]
                self.title = "Create objects"
            def shouldShow(self):
                return (FreeCAD.ActiveDocument != None) and (not FreeCADGui.Selection.getSelection())

        class DraftModifyWatcher:
            def __init__(self):
                self.commands = ["Draft_Move","Draft_Rotate",
                                 "Draft_Scale","Draft_Offset",
                                 "Draft_Trimex","Draft_Upgrade",
                                 "Draft_Downgrade","Draft_Edit",
                                 "Draft_Drawing"]
                self.title = "Modify objects"
            def shouldShow(self):
                return (FreeCAD.ActiveDocument != None) and (FreeCADGui.Selection.getSelection() != [])

        # OBSOLETE
        #class DraftTrayWatcher:
        #    def __init__(self,traywidget):
        #        self.form = traywidget
        #        self.widgets = [self.form]
        #    def shouldShow(self):
        #        return True
        #self.traywidget = QtGui.QWidget()
        #self.tray = QtGui.QVBoxLayout(self.traywidget)
        #self.tray.setObjectName("traylayout")
        #self.toptray = QtGui.QHBoxLayout()
        #self.bottomtray = QtGui.QHBoxLayout()
        #self.tray.addLayout(self.toptray)
        #self.tray.addLayout(self.bottomtray)
        #self.setupTray()
        #self.setupStyle()
        #w = DraftTrayWatcher(self.traywidget)
        #FreeCADGui.Control.addTaskWatcher([w,DraftCreateWatcher(),DraftModifyWatcher()])
        FreeCADGui.Control.addTaskWatcher([DraftCreateWatcher(),DraftModifyWatcher()])

    def changeEvent(self, event):
        if event.type() == QtCore.QEvent.LanguageChange:
            #print("Language changed!")
            self.ui.retranslateUi(self)

    def Activated(self):
        if self.taskmode:
            self.setWatchers()
            if hasattr(self,"tray"):
                self.tray.show()
        else: # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            self.draftWidget.setVisible(True)
            self.draftWidget.toggleViewAction().setVisible(True)

    def Deactivated(self):
        if (FreeCAD.activeDraftCommand != None):
            self.continueMode = False
            FreeCAD.activeDraftCommand.finish()
        if self.taskmode:
            FreeCADGui.Control.clearTaskWatcher()
            #self.tray = None
            if hasattr(self,"tray"):
                self.tray.hide()
        else: # self.taskmode == 0  Draft toolbar is obsolete and has been disabled (February 2020)
            self.draftWidget.setVisible(False)
            self.draftWidget.toggleViewAction().setVisible(False)

    def reset_ui_values(self):
        """Method to reset task panel values"""
        self.x = 0
        self.y = 0
        self.z = 0
        self.lvalue = 0
        self.pvalue = 90
        self.avalue = 0
        self.angle = None
        self.radius = 0
        self.offset = 0


class FacebinderTaskPanel:
    '''A TaskPanel for the facebinder'''
    def __init__(self):

        self.obj = None
        self.form = QtGui.QWidget()
        self.form.setObjectName("FacebinderTaskPanel")
        self.grid = QtGui.QGridLayout(self.form)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.form)
        self.grid.addWidget(self.title, 0, 0, 1, 2)

        # tree
        self.tree = QtGui.QTreeWidget(self.form)
        self.grid.addWidget(self.tree, 1, 0, 1, 2)
        self.tree.setColumnCount(2)
        self.tree.setHeaderLabels(["Name","Subelement"])

        # buttons
        self.addButton = QtGui.QPushButton(self.form)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 3, 0, 1, 1)

        self.delButton = QtGui.QPushButton(self.form)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 3, 1, 1, 1)

        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        self.update()

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def update(self):
        """fills the treewidget"""
        self.tree.clear()
        if self.obj:
            for f in self.obj.Faces:
                if isinstance(f[1],tuple):
                    for subf in f[1]:
                        item = QtGui.QTreeWidgetItem(self.tree)
                        item.setText(0,f[0].Name)
                        item.setIcon(0, QtGui.QIcon(":/icons/Part_3D_object.svg"))
                        item.setText(1,subf)
                else:
                    item = QtGui.QTreeWidgetItem(self.tree)
                    item.setText(0,f[0].Name)
                    item.setIcon(0, QtGui.QIcon(":/icons/Part_3D_object.svg"))
                    item.setText(1,f[1])
        self.retranslateUi(self.form)

    def addElement(self):
        if self.obj:
            for sel in FreeCADGui.Selection.getSelectionEx():
                if sel.HasSubObjects:
                    obj = sel.Object
                    for elt in sel.SubElementNames:
                        if "Face" in elt:
                            flist = self.obj.Faces
                            found = False
                            for face in flist:
                                if (face[0] == obj.Name):
                                    if isinstance(face[1],tuple):
                                        for subf in face[1]:
                                            if subf == elt:
                                                found = True
                                    else:
                                        if (face[1] == elt):
                                            found = True
                            if not found:
                                flist.append((obj,elt))
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
                    if (face[0].Name != obj.Name):
                        flist.append(face)
                    else:
                        if isinstance(face[1],tuple):
                            for subf in face[1]:
                                if subf != elt:
                                    flist.append((obj,subf))
                        else:
                            if (face[1] != elt):
                                flist.append(face)
                self.obj.Faces = flist
                FreeCAD.ActiveDocument.recompute()
            self.update()

    def accept(self):
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("draft", "Faces", None))
        self.delButton.setText(QtGui.QApplication.translate("draft", "Remove", None))
        self.addButton.setText(QtGui.QApplication.translate("draft", "Add", None))
        self.title.setText(QtGui.QApplication.translate("draft", "Facebinder elements", None))

#def translateWidget(w, context=None, disAmb=None):
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
#    wKids = w.findChildren(QtGui.QWidget)
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

if not hasattr(FreeCADGui,"draftToolBar"):
    FreeCADGui.draftToolBar = DraftToolBar()
#----End of Python Features Definitions----#
