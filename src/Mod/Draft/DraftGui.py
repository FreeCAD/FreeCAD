# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
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

__title__="FreeCAD Draft Workbench - GUI part"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = ["http://www.freecadweb.org"]

## @package DraftGui
#  \ingroup DRAFT
#  \brief GUI elements and utilities of the Draft workbench
#
#  This module provides GUI tools for the Draft workbench, such as
#  toolbars and task panels, and Qt-dependent utilities such as 
#  a delayed (todo) commit system

'''
This is the GUI part of the Draft module.
Report to Draft.py for info
'''

import FreeCAD, FreeCADGui, os, Draft, sys, DraftVecUtils, math

try:
    from PySide import QtCore, QtGui, QtSvg
except ImportError:
    FreeCAD.Console.PrintMessage("Error: Python-pyside package must be installed on your system to use the Draft module.")

try:
    if sys.version_info.major >= 3:
        _encoding = None
    else:
        _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, utf8_decode=True):
        """convenience function for Qt translator
            context: str
                context is typically a class name (e.g., "MyDialog")
            text: str
                text which gets translated
            utf8_decode: bool [False]
                if set to true utf8 encoded unicode will be returned. This option does not have influence
                on python3 as for python3 we are returning utf-8 encoded unicode by default!
        """
        if sys.version_info.major >= 3:
            return QtGui.QApplication.translate(context, text, None)
        elif utf8_decode:
            return QtGui.QApplication.translate(context, text, None, _encoding)
        else:
            return QtGui.QApplication.translate(context, text, None, _encoding).encode("utf8")

except AttributeError:
    def translate(context, text, utf8_decode=False):
        """convenience function for Qt translator
            context: str
                context is typically a class name (e.g., "MyDialog")
            text: str
                text which gets translated
            utf8_decode: bool [False]
                if set to true utf8 encoded unicode will be returned. This option does not have influence
                on python3 as for python3 we are returning utf-8 encoded unicode by default!
        """
        if sys.version_info.major >= 3:
            return QtGui.QApplication.translate(context, text, None)
        elif QtCore.qVersion() > "4":
            if utf8_decode:
                return QtGui.QApplication.translate(context, text, None)
            else:
                return QtGui.QApplication.translate(context, text, None).encode("utf8")
        else:
            if utf8_decode:
                return QtGui.QApplication.translate(context, text, None, _encoding)
            else:
                return QtGui.QApplication.translate(context, text, None, _encoding).encode("utf8")

def utf8_decode(text):
    """py2: str     -> unicode
            unicode -> unicode
       py3: str     -> str
            bytes   -> str
    """
    try:
        return text.decode("utf-8")
    except AttributeError:
        return text


# in-command shortcut definitions: Shortcut / Translation / related UI control
inCommandShortcuts = {
    "Relative":   ["R",translate("draft","Relative"),             "isRelative"],
    "Continue":   ["T",translate("draft","Continue"),             "continueCmd"],
    "Close":      ["O",translate("draft","Close"),                "closeButton"],
    "Copy":       ["P",translate("draft","Copy"),                 "isCopy"],
    "Fill":       ["L",translate("draft","Fill"),                 "hasFill"],
    "Exit":       ["A",translate("draft","Exit"),                 "finishButton"],
    "Snap":       ["S",translate("draft","Snap On/Off"),          None],
    "Increase":   ["[",translate("draft","Increase snap radius"), None],
    "Decrease":   ["]",translate("draft","Decrease snap radius"), None],
    "RestrictX":  ["X",translate("draft","Restrict X"),           None],
    "RestrictY":  ["Y",translate("draft","Restrict Y"),           None],
    "RestrictZ":  ["Z",translate("draft","Restrict Z"),           None],
    "SelectEdge": ["E",translate("draft","Select edge"),          "selectButton"],
    "AddHold":    ["Q",translate("draft","Add custom snap point"),None],
    "Length":     ["H",translate("draft","Length mode"),          "lengthValue"],
    "Wipe":       ["W",translate("draft","Wipe"),                 "wipeButton"],
    "SetWP":      ["U",translate("draft","Set Working Plane"),    "orientWPButton"]
}


class todo:
    ''' static todo class, delays execution of functions.  Use todo.delay
    to schedule geometry manipulation that would crash coin if done in the
    event callback'''

    '''List of (function, argument) pairs to be executed by
    QtCore.QTimer.singleShot(0,doTodo).'''
    itinerary = []
    commitlist = []
    afteritinerary = []
    
    @staticmethod
    def doTasks():
        #print("debug: doing delayed tasks: commitlist: ",todo.commitlist," itinerary: ",todo.itinerary)
        try:
            for f, arg in todo.itinerary:
                try:
                    # print("debug: executing",f)
                    if arg or (arg == False):
                        f(arg)
                    else:
                        f()
                except:
                    wrn = "[Draft.todo.tasks] Unexpected error:", sys.exc_info()[0], "in ", f, "(", arg, ")"
                    FreeCAD.Console.PrintWarning (wrn)
        except ReferenceError:
            print ("Debug: DraftGui.todo.doTasks: queue contains a deleted object, skipping")
        todo.itinerary = []
        if todo.commitlist:
            for name,func in todo.commitlist:
                if sys.version_info.major < 3:
                    if isinstance(name,unicode):
                        name = name.encode("utf8")
                #print("debug: committing ",str(name))
                try:
                    name = str(name)
                    FreeCAD.ActiveDocument.openTransaction(name)
                    if isinstance(func,list):
                        for l in func:
                            FreeCADGui.doCommand(l)
                    else:
                        func()
                    FreeCAD.ActiveDocument.commitTransaction()
                except:
                    wrn = "[Draft.todo.commit] Unexpected error:", sys.exc_info()[0], "in ", func
                    FreeCAD.Console.PrintWarning (wrn)
            # restack Draft screen widgets after creation
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.restack()
        todo.commitlist = []
        for f, arg in todo.afteritinerary:
            try:
                # print("debug: executing",f)
                if arg:
                    f(arg)
                else:
                    f()
            except:
                wrn = "[Draft.todo.tasks] Unexpected error:", sys.exc_info()[0], "in ", f, "(", arg, ")"
                FreeCAD.Console.PrintWarning (wrn)
        todo.afteritinerary = []

    @staticmethod
    def delay (f, arg):
        # print("debug: delaying",f)
        if todo.itinerary == []:
            QtCore.QTimer.singleShot(0, todo.doTasks)
        todo.itinerary.append((f,arg))

    @staticmethod
    def delayCommit (cl):
        # print("debug: delaying commit",cl)
        QtCore.QTimer.singleShot(0, todo.doTasks)
        todo.commitlist = cl

    @staticmethod
    def delayAfter (f, arg):
        # print("debug: delaying",f)
        if todo.afteritinerary == []:
            QtCore.QTimer.singleShot(0, todo.doTasks)
        todo.afteritinerary.append((f,arg))

#---------------------------------------------------------------------------
# UNITS handling
#---------------------------------------------------------------------------
def getDefaultUnit(dim):
    '''return default Unit of Measure for a Dimension based on user preference
    Units Schema'''
    # only Length and Angle so far
    from FreeCAD import Units
    if dim == 'Length':
        qty = FreeCAD.Units.Quantity(1.0,FreeCAD.Units.Length)
        UOM = qty.getUserPreferred()[2]
    elif dim == 'Angle':
        qty = FreeCAD.Units.Quantity(1.0,FreeCAD.Units.Angle)
        UOM = qty.getUserPreferred()[2]
    else:
        UOM = "xx"
    return UOM
 
def makeFormatSpec(decimals=4,dim='Length'):
    ''' return a % format spec with specified decimals for a specified 
    dimension based on on user preference Units Schema'''
    if dim == 'Length':
        fmtSpec = "%." + str(decimals) + "f "+ getDefaultUnit('Length')
    elif dim == 'Angle':
        fmtSpec = "%." + str(decimals) + "f "+ getDefaultUnit('Angle')
    else:
        fmtSpec = "%." + str(decimals) + "f " + "??"
    return fmtSpec

def displayExternal(internValue,decimals=None,dim='Length',showUnit=True,unit=None):
    '''return an internal value (ie mm) Length or Angle converted for display according 
    to Units Schema in use. Unit can be used to force the value to express in a certain unit'''
    from FreeCAD import Units
    if dim == 'Length':
        q = FreeCAD.Units.Quantity(internValue,FreeCAD.Units.Length)
        if not unit:
            if (decimals == None) and showUnit:
                return q.UserString
            conversion = q.getUserPreferred()[1]
            uom = q.getUserPreferred()[2]
        else:
            uom = unit
            internValue = q.getValueAs(unit)
            conversion = 1
    elif dim == 'Angle':
        return FreeCAD.Units.Quantity(internValue,FreeCAD.Units.Angle).UserString
    else:
        conversion = 1.0
        if decimals == None:
            decimals = 2
        uom = "??"
    if not showUnit:
        uom = ""
    fmt = "{0:."+ str(decimals) + "f} "+ uom
    displayExt = fmt.format(float(internValue) / float(conversion))
    displayExt = displayExt.replace(".",QtCore.QLocale().decimalPoint())
    return displayExt

#---------------------------------------------------------------------------
# Customized widgets
#---------------------------------------------------------------------------

class DraftBaseWidget(QtGui.QWidget):
    def __init__(self,parent = None):
        QtGui.QWidget.__init__(self,parent)
    def eventFilter(self, widget, event):
        if event.type() == QtCore.QEvent.KeyPress and event.key()==QtCore.Qt.Key_Tab:
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.cycleSnapObject()
            return True
        return QtGui.QWidget.eventFilter(self, widget, event)

class DraftDockWidget(DraftBaseWidget):
    "custom Widget that emits a resized() signal when resized"
    def __init__(self,parent = None):
        QtGui.QWidget.__init__(self,parent)
    def resizeEvent(self,event):
        self.emit(QtCore.SIGNAL("resized()"))
    def changeEvent(self, event):
        if event.type() == QtCore.QEvent.LanguageChange:
            self.emit(QtCore.SIGNAL("retranslate()"))
        else:
            QtGui.QWidget.changeEvent(self,event)
                        
class DraftLineEdit(QtGui.QLineEdit):
    "custom QLineEdit widget that has the power to catch Escape keypress"
    def __init__(self, parent=None):
        QtGui.QLineEdit.__init__(self, parent)
    def keyPressEvent(self, event):
        if event.key() == QtCore.Qt.Key_Escape:
            self.emit(QtCore.SIGNAL("escaped()"))
        elif event.key() == QtCore.Qt.Key_Up:
            self.emit(QtCore.SIGNAL("up()"))
        elif event.key() == QtCore.Qt.Key_Down:
            self.emit(QtCore.SIGNAL("down()"))
        else:
            QtGui.QLineEdit.keyPressEvent(self, event)

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
    "main draft Toolbar"
    def __init__(self):
        self.tray = None
        self.sourceCmd = None
        self.cancel = None
        self.pointcallback = None
        self.taskmode = Draft.getParam("UiMode",1)
        #print("taskmode: ",str(self.taskmode))
        self.paramcolor = Draft.getParam("color",255)>>8
        self.color = QtGui.QColor(self.paramcolor)
        self.facecolor = QtGui.QColor(204,204,204)
        self.linewidth = Draft.getParam("linewidth",2)
        self.fontsize = Draft.getParam("textheight",0.20)
        self.paramconstr = Draft.getParam("constructioncolor",746455039)>>8
        self.constrMode = False
        self.continueMode = False
        self.relativeMode = True
        self.state = None
        self.textbuffer = []
        self.crossedViews = []
        self.isTaskOn = False
        self.fillmode = Draft.getParam("fillmode",False)
        self.mask = None
        self.alock = False
        self.angle = None
        self.avalue = None
        self.x = 0
        self.y = 0
        self.z = 0
        self.uiloader = FreeCADGui.UiLoader()
        self.autogroup = None
        self.isCenterPlane = False
        
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

        else:
            # create the draft Toolbar                
            self.draftWidget = QtGui.QDockWidget()
            self.baseWidget = DraftDockWidget()
            self.draftWidget.setObjectName("draftToolbar")
            self.draftWidget.setTitleBarWidget(self.baseWidget)
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

    def _pushbutton (self,name, layout, hide=True, icon=None, width=66, checkable=False):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
        bsize = p.GetInt("ToolbarIconSize",24)+2
        isize = p.GetInt("ToolbarIconSize",24)/3*2
        button = QtGui.QPushButton(self.baseWidget)
        button.setObjectName(name)
        button.setMaximumSize(QtCore.QSize(width,bsize))
        if hide:
            button.hide()
        if icon:
            button.setIcon(QtGui.QIcon(':/icons/'+icon+'.svg'))
            button.setIconSize(QtCore.QSize(isize, isize))
        if checkable:
            button.setCheckable(True)
            button.setChecked(False)
        layout.addWidget(button)
        return button

    def _label (self,name, layout, hide=True):
        label = QtGui.QLabel(self.baseWidget)
        label.setObjectName(name)
        if hide: label.hide()
        layout.addWidget(label)
        return label

    def _lineedit (self,name, layout, hide=True, width=None):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
        bsize = p.GetInt("ToolbarIconSize",24)-2
        lineedit = DraftLineEdit(self.baseWidget)
        lineedit.setObjectName(name)
        if hide: lineedit.hide()
        if not width: width = 800
        lineedit.setMaximumSize(QtCore.QSize(width,bsize))
        layout.addWidget(lineedit)
        return lineedit

    def _inputfield (self,name, layout, hide=True, width=None):
        inputfield = self.uiloader.createWidget("Gui::InputField")
        inputfield.setObjectName(name)
        if hide: inputfield.hide()
        if not width:
            sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Preferred)
            inputfield.setSizePolicy(sizePolicy)
        else:
            inputfield.setMaximumWidth(width)
        layout.addWidget(inputfield)
        return inputfield

    def _spinbox (self,name, layout, val=None, vmax=None, hide=True, double=False, size=None):
        if double:
            sbox = QtGui.QDoubleSpinBox(self.baseWidget)
            sbox.setDecimals(FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("Decimals",2))
        else:
            sbox = QtGui.QSpinBox(self.baseWidget)
        sbox.setObjectName(name)
        if val: sbox.setValue(val)
        if vmax: sbox.setMaximum(vmax)
        if size: sbox.setMaximumSize(QtCore.QSize(size[0],size[1]))
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
        "sets the draft toolbar up"
        
        # command

        self.promptlabel = self._label("promptlabel", self.layout, hide=task)
        self.cmdlabel = self._label("cmdlabel", self.layout, hide=task)
        boldtxt = QtGui.QFont()
        boldtxt.setWeight(75)
        boldtxt.setBold(True)
        self.cmdlabel.setFont(boldtxt)

        # subcommands

        self.addButton = self._pushbutton("addButton", self.layout, icon="Draft_AddPoint", width=22, checkable=True)
        self.delButton = self._pushbutton("delButton", self.layout, icon="Draft_DelPoint", width=22, checkable=True)
        self.sharpButton = self._pushbutton("sharpButton", self.layout, icon="Draft_BezSharpNode", width=22, checkable=True)
        self.tangentButton = self._pushbutton("tangentButton", self.layout, icon="Draft_BezTanNode", width=22, checkable=True)
        self.symmetricButton = self._pushbutton("symmetricButton", self.layout, icon="Draft_BezSymNode", width=22, checkable=True)

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
        self.yValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.labelz = self._label("labelz", zl)
        self.zValue = self._inputfield("zValue", zl)
        self.zValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.pointButton = self._pushbutton("addButton", bl, icon="Draft_AddPoint", width=100)
        
        # text
        
        self.textValue = self._lineedit("textValue", self.layout)
        
        # additional line controls

        ll = QtGui.QHBoxLayout()
        al = QtGui.QHBoxLayout()
        self.layout.addLayout(ll)
        self.layout.addLayout(al)
        self.labellength = self._label("labellength", ll)
        self.lengthValue = self._inputfield("lengthValue", ll)
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
        self.offsetLabel = self._label("offsetlabel", ol)
        self.offsetValue = self._inputfield("offsetValue", ol)
        self.offsetValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        rl = QtGui.QHBoxLayout()
        self.layout.addLayout(rl)
        self.labelRadius = self._label("labelRadius", rl)
        self.radiusValue = self._inputfield("radiusValue", rl)
        self.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.isRelative = self._checkbox("isRelative",self.layout,checked=self.relativeMode)
        self.hasFill = self._checkbox("hasFill",self.layout,checked=self.fillmode)
        self.continueCmd = self._checkbox("continueCmd",self.layout,checked=self.continueMode)
        self.occOffset = self._checkbox("occOffset",self.layout,checked=False)
        self.undoButton = self._pushbutton("undoButton", self.layout, icon='Draft_Rotate')
        self.finishButton = self._pushbutton("finishButton", self.layout, icon='Draft_Finish')
        self.closeButton = self._pushbutton("closeButton", self.layout, icon='Draft_Lock')
        self.wipeButton = self._pushbutton("wipeButton", self.layout, icon='Draft_Wipe')
        self.orientWPButton = self._pushbutton("orientWPButton", self.layout, icon='Draft_SelectPlane')
        self.selectButton = self._pushbutton("selectButton", self.layout, icon='view-select')
        self.xyButton = self._pushbutton("xyButton", self.layout,icon="view-top")
        self.xzButton = self._pushbutton("xzButton", self.layout,icon="view-front")
        self.yzButton = self._pushbutton("yzButton", self.layout,icon="view-right")
        self.currentViewButton = self._pushbutton("view", self.layout,icon="view-isometric")
        self.resetPlaneButton = self._pushbutton("none", self.layout,icon="view-axonometric")
        self.isCopy = self._checkbox("isCopy",self.layout,checked=False)
        gl = QtGui.QHBoxLayout()
        self.layout.addLayout(gl)
        self.gridLabel = self._label("gridLabel", gl)
        self.gridValue = self._inputfield("gridValue", gl)
        self.gridValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        ml = QtGui.QHBoxLayout()
        self.layout.addLayout(ml)
        self.mainlineLabel = self._label("mainlineLabel", ml)
        self.mainlineValue = self._spinbox("mainlineValue", ml)
        self.centerPlane = self._checkbox("centerPlane",self.layout,checked = self.isCenterPlane)

        # spacer
        if not self.taskmode:
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
        QtCore.QObject.connect(self.offsetValue,QtCore.SIGNAL("valueChanged(double)"),self.changeOffsetValue)
        QtCore.QObject.connect(self.xValue,QtCore.SIGNAL("returnPressed()"),self.checkx)
        QtCore.QObject.connect(self.yValue,QtCore.SIGNAL("returnPressed()"),self.checky)
        QtCore.QObject.connect(self.lengthValue,QtCore.SIGNAL("returnPressed()"),self.checkangle)
        QtCore.QObject.connect(self.xValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.yValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.radiusValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("returnPressed()"),self.validatePoint)
        QtCore.QObject.connect(self.pointButton,QtCore.SIGNAL("clicked()"),self.validatePoint)
        QtCore.QObject.connect(self.radiusValue,QtCore.SIGNAL("returnPressed()"),self.validatePoint)
        QtCore.QObject.connect(self.angleValue,QtCore.SIGNAL("returnPressed()"),self.validatePoint)
        QtCore.QObject.connect(self.textValue,QtCore.SIGNAL("textChanged(QString)"),self.storeCurrentText)
        QtCore.QObject.connect(self.textValue,QtCore.SIGNAL("returnPressed()"),self.sendText)
        #QtCore.QObject.connect(self.textValue,QtCore.SIGNAL("escaped()"),self.escape)
        QtCore.QObject.connect(self.textValue,QtCore.SIGNAL("down()"),self.sendText)
        QtCore.QObject.connect(self.textValue,QtCore.SIGNAL("up()"),self.lineUp)
        QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("returnPressed()"),self.setFocus)
        QtCore.QObject.connect(self.offsetValue,QtCore.SIGNAL("textEdited(QString)"),self.checkSpecialChars)
        QtCore.QObject.connect(self.offsetValue,QtCore.SIGNAL("returnPressed()"),self.validatePoint)
        QtCore.QObject.connect(self.addButton,QtCore.SIGNAL("toggled(bool)"),self.setAddMode)
        QtCore.QObject.connect(self.delButton,QtCore.SIGNAL("toggled(bool)"),self.setDelMode)
        QtCore.QObject.connect(self.sharpButton,QtCore.SIGNAL("toggled(bool)"),self.setSharpMode)
        QtCore.QObject.connect(self.tangentButton,QtCore.SIGNAL("toggled(bool)"),self.setTangentMode)
        QtCore.QObject.connect(self.symmetricButton,QtCore.SIGNAL("toggled(bool)"),self.setSymmetricMode)
        QtCore.QObject.connect(self.finishButton,QtCore.SIGNAL("pressed()"),self.finish)
        QtCore.QObject.connect(self.closeButton,QtCore.SIGNAL("pressed()"),self.closeLine)
        QtCore.QObject.connect(self.wipeButton,QtCore.SIGNAL("pressed()"),self.wipeLine)
        QtCore.QObject.connect(self.orientWPButton,QtCore.SIGNAL("pressed()"),self.orientWP)
        QtCore.QObject.connect(self.undoButton,QtCore.SIGNAL("pressed()"),self.undoSegment)
        QtCore.QObject.connect(self.selectButton,QtCore.SIGNAL("pressed()"),self.selectEdge)
        QtCore.QObject.connect(self.xyButton,QtCore.SIGNAL("clicked()"),self.selectXY)
        QtCore.QObject.connect(self.xzButton,QtCore.SIGNAL("clicked()"),self.selectXZ)
        QtCore.QObject.connect(self.yzButton,QtCore.SIGNAL("clicked()"),self.selectYZ)
        QtCore.QObject.connect(self.continueCmd,QtCore.SIGNAL("stateChanged(int)"),self.setContinue)
        QtCore.QObject.connect(self.isCopy,QtCore.SIGNAL("stateChanged(int)"),self.setCopymode)
        QtCore.QObject.connect(self.isRelative,QtCore.SIGNAL("stateChanged(int)"),self.setRelative)
        QtCore.QObject.connect(self.hasFill,QtCore.SIGNAL("stateChanged(int)"),self.setFill) 
        QtCore.QObject.connect(self.currentViewButton,QtCore.SIGNAL("clicked()"),self.selectCurrentView)
        QtCore.QObject.connect(self.resetPlaneButton,QtCore.SIGNAL("clicked()"),self.selectResetPlane)
        QtCore.QObject.connect(self.baseWidget,QtCore.SIGNAL("resized()"),self.relocate)
        QtCore.QObject.connect(self.baseWidget,QtCore.SIGNAL("retranslate()"),self.retranslateUi)
        QtCore.QObject.connect(self.SSizeValue,QtCore.SIGNAL("valueChanged(double)"),self.changeSSizeValue)
        QtCore.QObject.connect(self.SSizeValue,QtCore.SIGNAL("returnPressed()"),self.validateSNumeric)
        QtCore.QObject.connect(self.STrackValue,QtCore.SIGNAL("valueChanged(double)"),self.changeSTrackValue)
        QtCore.QObject.connect(self.STrackValue,QtCore.SIGNAL("returnPressed()"),self.validateSNumeric)
        QtCore.QObject.connect(self.SStringValue,QtCore.SIGNAL("returnPressed()"),self.validateSString)
        QtCore.QObject.connect(self.chooserButton,QtCore.SIGNAL("pressed()"),self.pickFile)
        QtCore.QObject.connect(self.FFileValue,QtCore.SIGNAL("returnPressed()"),self.validateFile)
        QtCore.QObject.connect(self.gridValue,QtCore.SIGNAL("textEdited(QString)"),self.setGridSize)
        QtCore.QObject.connect(self.mainlineValue,QtCore.SIGNAL("valueChanged(int)"),self.setMainline)
        QtCore.QObject.connect(self.centerPlane,QtCore.SIGNAL("stateChanged(int)"),self.setCenterPlane) 
        
        # following lines can cause a crash and are not needed anymore when using the task panel
        # http://forum.freecadweb.org/viewtopic.php?f=3&t=6952
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
        "sets draft tray buttons up"

        self.wplabel = self._pushbutton("wplabel", self.toptray, icon='Draft_SelectPlane',hide=False,width=120)
        defaultWP = Draft.getParam("defaultWP",0)
        if defaultWP == 1:
            self.wplabel.setText("Top")
        elif defaultWP == 2:
            self.wplabel.setText("Front")
        elif defaultWP == 3:
            self.wplabel.setText("Side")
        else:
            self.wplabel.setText("Auto")
        self.constrButton = self._pushbutton("constrButton", self.toptray, hide=False, icon='Draft_Construction',width=22, checkable=True)
        self.constrColor = QtGui.QColor(self.paramconstr)
        self.colorButton = self._pushbutton("colorButton",self.bottomtray, hide=False,width=22)
        self.colorPix = QtGui.QPixmap(16,16)
        self.colorPix.fill(self.color)
        self.colorButton.setIcon(QtGui.QIcon(self.colorPix))
        self.facecolorButton = self._pushbutton("facecolorButton",self.bottomtray, hide=False,width=22)
        self.facecolorPix = QtGui.QPixmap(16,16)
        self.facecolorPix.fill(self.facecolor)
        self.facecolorButton.setIcon(QtGui.QIcon(self.facecolorPix))
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
        bsize = p.GetInt("ToolbarIconSize",24)+2
        self.widthButton = self._spinbox("widthButton", self.bottomtray, val=self.linewidth,hide=False,size=(bsize * 2,bsize))
        self.widthButton.setSuffix("px")
        self.fontsizeButton = self._spinbox("fontsizeButton",self.bottomtray, val=self.fontsize,vmax=999, hide=False,double=True,size=(bsize * 3,bsize))
        self.applyButton = self._pushbutton("applyButton", self.toptray, hide=False, icon='Draft_Apply',width=22)
        self.autoGroupButton = self._pushbutton("autoGroup",self.bottomtray,icon="Draft_AutoGroup_off",hide=False,width=120)
        self.autoGroupButton.setText("None")
        
        self.wplabel.setToolTip(translate("draft", "Current working plane:",utf8_decode=True)+self.wplabel.text())
        self.constrButton.setToolTip(translate("draft", "Toggle construction mode"))
        self.colorButton.setToolTip(translate("draft", "Current line color"))
        self.facecolorButton.setToolTip(translate("draft", "Current face color"))
        self.widthButton.setToolTip(translate("draft", "Current line width"))
        self.fontsizeButton.setToolTip(translate("draft", "Current font size"))
        self.applyButton.setToolTip(translate("draft", "Apply to selected objects"))
        self.autoGroupButton.setToolTip(translate("draft", "Autogroup off"))

        QtCore.QObject.connect(self.wplabel,QtCore.SIGNAL("pressed()"),self.selectplane)
        QtCore.QObject.connect(self.colorButton,QtCore.SIGNAL("pressed()"),self.getcol)
        QtCore.QObject.connect(self.facecolorButton,QtCore.SIGNAL("pressed()"),self.getfacecol)
        QtCore.QObject.connect(self.widthButton,QtCore.SIGNAL("valueChanged(int)"),self.setwidth)
        QtCore.QObject.connect(self.fontsizeButton,QtCore.SIGNAL("valueChanged(double)"),self.setfontsize)
        QtCore.QObject.connect(self.applyButton,QtCore.SIGNAL("pressed()"),self.apply)
        QtCore.QObject.connect(self.constrButton,QtCore.SIGNAL("toggled(bool)"),self.toggleConstrMode)
        QtCore.QObject.connect(self.autoGroupButton,QtCore.SIGNAL("pressed()"),self.runAutoGroup)

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
        self.pointButton.setToolTip(translate("draft", "Enter a new point with the given coordinates"))
        self.labellength.setText(translate("draft", "Length"))
        self.labelangle.setText(translate("draft", "Angle"))
        self.lengthValue.setToolTip(translate("draft", "Length of current segment"))
        self.angleValue.setToolTip(translate("draft", "Angle of current segment"))
        #self.angleLock.setText(translate("draft", "&Lock"))
        self.angleLock.setToolTip(translate("draft", "Check this to lock the current angle")+" ("+inCommandShortcuts["Length"][0]+")")
        self.labelRadius.setText(translate("draft", "Radius"))
        self.radiusValue.setToolTip(translate("draft", "Radius of Circle"))
        self.isRelative.setText(translate("draft", "Relative")+" ("+inCommandShortcuts["Relative"][0]+")")
        self.isRelative.setToolTip(translate("draft", "Coordinates relative to last point or absolute"))
        self.hasFill.setText(translate("draft", "Filled")+" ("+inCommandShortcuts["Fill"][0]+")")
        self.hasFill.setToolTip(translate("draft", "Check this if the object should appear as filled, otherwise it will appear as wireframe"))
        self.finishButton.setText(translate("draft", "Finish")+" ("+inCommandShortcuts["Exit"][0]+")")
        self.finishButton.setToolTip(translate("draft", "Finishes the current drawing or editing operation"))
        self.continueCmd.setToolTip(translate("draft", "If checked, command will not finish until you press the command button again"))
        self.continueCmd.setText(translate("draft", "Continue")+" ("+inCommandShortcuts["Continue"][0]+")")
        self.occOffset.setToolTip(translate("draft", "If checked, an OCC-style offset will be performed instead of the classic offset"))
        self.occOffset.setText(translate("draft", "&OCC-style offset"))
        self.addButton.setToolTip(translate("draft", "Add points to the current object"))
        self.delButton.setToolTip(translate("draft", "Remove points from the current object"))
        self.sharpButton.setToolTip(translate("draft", "Make Bezier node sharp"))
        self.tangentButton.setToolTip(translate("draft", "Make Bezier node tangent"))
        self.symmetricButton.setToolTip(translate("draft", "Make Bezier node symmetric"))
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
        self.offsetLabel.setText(translate("draft", "Offset"))
        self.xyButton.setText(translate("draft", "XY (top)"))
        self.xyButton.setToolTip(translate("draft", "Select XY plane"))
        self.xzButton.setText(translate("draft", "XZ (front)"))
        self.xzButton.setToolTip(translate("draft", "Select XZ plane"))
        self.yzButton.setText(translate("draft", "YZ (side)"))
        self.yzButton.setToolTip(translate("draft", "Select YZ plane"))
        self.currentViewButton.setText(translate("draft", "View"))
        self.currentViewButton.setToolTip(translate("draft", "Select plane perpendicular to the current view"))
        self.resetPlaneButton.setText(translate("draft", "Auto"))
        self.resetPlaneButton.setToolTip(translate("draft", "Do not project points to a drawing plane"))
        self.isCopy.setText(translate("draft", "Copy")+" ("+inCommandShortcuts["Copy"][0]+")")
        self.isCopy.setToolTip(translate("draft", "If checked, objects will be copied instead of moved. Preferences -> Draft -> Global copy mode to keep this mode in next commands"))
        self.SStringValue.setToolTip(translate("draft", "Text string to draw"))
        self.labelSString.setText(translate("draft", "String"))
        self.SSizeValue.setToolTip(translate("draft", "Height of text"))
        self.labelSSize.setText(translate("draft", "Height"))
        self.STrackValue.setToolTip(translate("draft", "Intercharacter spacing"))
        self.labelSTrack.setText(translate("draft", "Tracking"))
        self.labelFFile.setText(translate("draft", "Full path to font file:"))
        self.chooserButton.setToolTip(translate("draft", "Open a FileChooser for font file"))
        self.gridLabel.setText(translate("draft", "Grid spacing"))
        self.gridValue.setToolTip(translate("draft", "The spacing between the grid lines"))
        self.mainlineLabel.setText(translate("draft", "Main line every"))
        self.mainlineValue.setToolTip(translate("draft", "The number of lines between main lines"))
        self.centerPlane.setText(translate("draft", "Center plane on view"))
        
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

#---------------------------------------------------------------------------
# Interface modes
#---------------------------------------------------------------------------

    def taskUi(self,title="Draft",extra=None,icon="Draft_Draft"):
        if self.taskmode:
            self.isTaskOn = True
            todo.delay(FreeCADGui.Control.closeDialog,None)
            self.baseWidget = DraftBaseWidget()
            self.layout = QtGui.QVBoxLayout(self.baseWidget)
            self.setupToolBar(task=True)
            self.retranslateUi(self.baseWidget)
            self.panel = DraftTaskPanel(self.baseWidget,extra)
            todo.delay(FreeCADGui.Control.showDialog,self.panel)
        else:
            # create a dummy task to block the UI during the works
            class dummy:
                "an empty dialog"
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
        self.setTitle(title)
        
    def redraw(self):
        "utility function that is performed after each clicked point"
        self.checkLocal()
    
    def setFocus(self):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        if p.GetBool("focusOnLength",False):
            self.lengthValue.setFocus()
            self.lengthValue.selectAll()
        else:
            self.xValue.setFocus()
            self.xValue.selectAll()

    def selectPlaneUi(self):
        self.taskUi(translate("draft", "Select Plane"))
        self.xyButton.show()
        self.xzButton.show()
        self.yzButton.show()
        self.currentViewButton.show()
        self.resetPlaneButton.show()
        self.offsetLabel.show()
        self.offsetValue.show()
        self.gridLabel.show()
        self.gridValue.show()
        p = Draft.getParam("gridSpacing",1.0)
        self.gridValue.setText(FreeCAD.Units.Quantity(p,FreeCAD.Units.Length).UserString)
        self.mainlineLabel.show()
        self.mainlineValue.show()
        p = Draft.getParam("gridEvery",10)
        self.mainlineValue.setValue(p)
        self.centerPlane.show()
        
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

    def lineUi(self,title=None):
        if title:
            self.pointUi(title,icon="Draft_Line")
        else:
            self.pointUi(translate("draft", "Line"),icon="Draft_Line")
        self.extraLineUi()
        self.xValue.setEnabled(True)
        self.yValue.setEnabled(True)
        self.isRelative.show()
        self.undoButton.show()
        self.continueCmd.show()

    def wireUi(self,title=None):
        if title:
            self.pointUi(title)
        else:
            self.pointUi(translate("draft", "DWire"),icon="Draft_Wire")
        self.xValue.setEnabled(True)
        self.yValue.setEnabled(True)
        self.isRelative.show()
        self.hasFill.show()
        self.finishButton.show()
        self.closeButton.show()
        self.wipeButton.show()
        self.orientWPButton.show()
        self.undoButton.show()
        self.continueCmd.show()
        
    def circleUi(self):
        self.pointUi(translate("draft", "Circle"),icon="Draft_Circle")
        self.continueCmd.show()
        self.labelx.setText(translate("draft", "Center X"))
        self.hasFill.show()

    def arcUi(self):
        self.pointUi(translate("draft", "Arc"),icon="Draft_Arc")
        self.labelx.setText(translate("draft", "Center X"))
        self.continueCmd.show()

    def pointUi(self,title=translate("draft","Point"),cancel=None,extra=None,getcoords=None,rel=False,icon="Draft_Draft"):
        if cancel: self.cancel = cancel
        if getcoords: self.pointcallback = getcoords
        self.taskUi(title,extra,icon)
        self.xValue.setEnabled(True)
        self.yValue.setEnabled(True)
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
        self.showCommandOptions(title)
        
    def labelUi(self,title=translate("draft","Label"),callback=None):
        w = QtGui.QWidget()
        w.setWindowTitle(translate("draft","Label type", utf8_decode=True))
        l = QtGui.QVBoxLayout(w)
        combo = QtGui.QComboBox()
        for s in ["Custom","Name","Label","Position","Length","Area","Volume","Tag","Material"]:
            combo.addItem(s)
        combo.setCurrentIndex(["Custom","Name","Label","Position","Length","Area","Volume","Tag","Material"].index(Draft.getParam("labeltype","Custom")))
        l.addWidget(combo)
        QtCore.QObject.connect(combo,QtCore.SIGNAL("currentIndexChanged(int)"),callback)
        self.pointUi(title=title,extra=w)

    def extraUi(self):
        pass

    def offsetUi(self):
        self.taskUi(translate("draft","Offset"))
        self.radiusUi()
        self.isCopy.show()
        self.occOffset.show()
        self.labelRadius.setText(translate("draft","Distance"))
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
        else:
            self.setTitle(translate("draft", "None"))
            self.labelx.setText(translate("draft", "X"))
            self.hideXYZ()
            self.numFacesLabel.hide()
            self.numFaces.hide()
            self.isRelative.hide()
            self.hasFill.hide()
            self.finishButton.hide()
            self.addButton.hide()
            self.delButton.hide()
            self.sharpButton.hide()
            self.tangentButton.hide()
            self.symmetricButton.hide()
            self.undoButton.hide()
            self.closeButton.hide()
            self.wipeButton.hide()
            self.orientWPButton.hide()
            self.selectButton.hide()
            self.xyButton.hide()
            self.xzButton.hide()
            self.yzButton.hide()
            self.currentViewButton.hide()
            self.resetPlaneButton.hide()
            self.offsetLabel.hide()
            self.offsetValue.hide()
            self.labelRadius.hide()
            self.radiusValue.hide()
            self.isCopy.hide()
            self.textValue.hide()
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
            self.gridLabel.hide()
            self.gridValue.hide()
            self.mainlineLabel.hide()
            self.mainlineValue.hide()
            self.centerPlane.hide()
            
    def trimUi(self,title=translate("draft","Trim")):
        self.taskUi(title)
        self.radiusUi()
        self.labelRadius.setText(translate("draft","Distance"))
        self.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        todo.delay(self.radiusValue.setFocus,None)
        self.radiusValue.selectAll()

    def radiusUi(self):
        self.hideXYZ()
        self.labelRadius.setText(translate("draft", "Radius"))
        self.labelRadius.show()
        self.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Length).UserString)
        self.radiusValue.show()

    def textUi(self):
        self.hideXYZ()
        self.textValue.show()
        self.textValue.setText('')
        todo.delay(self.textValue.setFocus,None)
        self.textbuffer=[]
        self.textline=0
        self.continueCmd.show()

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
        else:
            self.cmdlabel.setText(title)

    def selectUi(self,extra=None,callback=None):
        if not self.taskmode:
            self.labelx.setText(translate("draft", "Pick Object"))
            self.labelx.show()
        self.makeDumbTask(extra,callback)

    def editUi(self, mode=None):
        self.taskUi(translate("draft", "Edit"))
        self.hideXYZ()
        self.numFaces.hide()
        self.isRelative.hide()
        self.hasFill.hide()
        self.addButton.show()
        self.delButton.show()
        if mode == 'BezCurve':
            self.sharpButton.show()
            self.tangentButton.show()
            self.symmetricButton.show()
        self.finishButton.show()
        self.closeButton.show()
        # always start Edit with buttons unchecked
        self.addButton.setChecked(False)
        self.delButton.setChecked(False)
        self.sharpButton.setChecked(False)
        self.tangentButton.setChecked(False)
        self.symmetricButton.setChecked(False)

    def extUi(self):
        self.hasFill.show()
        self.continueCmd.show()

    def modUi(self):
        self.isCopy.show()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        if p.GetBool("copymode",True):
            self.isCopy.setChecked(p.GetBool("copymodeValue",False))
        self.continueCmd.show()

    def vertUi(self,addmode=True):
        self.addButton.setChecked(addmode)
        self.delButton.setChecked(not(addmode))

    def showCommandOptions(self,name):
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("Verbose",True):
            cmdstr = "\n"+name+" "+translate("draft","options")+" : "
            first = True
            for k,v in inCommandShortcuts.items():
                if v[2]:
                    if getattr(self,v[2]).isVisible():
                        if first:
                            first = False
                        else:
                            cmdstr += ", "
                        cmdstr += v[0] + ":" + v[1]
                else:
                    if first:
                        first = False
                    else:
                        cmdstr += ", "
                    cmdstr += v[0] + ":" + v[1]
            FreeCAD.Console.PrintMessage(cmdstr+"\n\n")

    def checkLocal(self):
        "checks if x,y,z coords must be displayed as local or global"
        self.labelx.setText(translate("draft", "Global X"))
        self.labely.setText(translate("draft", "Global Y"))
        self.labelz.setText(translate("draft", "Global Z"))
        if hasattr(FreeCAD,"DraftWorkingPlane"):
            if not FreeCAD.DraftWorkingPlane.isGlobal():
                self.labelx.setText(translate("draft", "Local X"))
                self.labely.setText(translate("draft", "Local Y"))
                self.labelz.setText(translate("draft", "Local Z"))

    def setEditButtons(self,mode):
        self.addButton.setEnabled(mode)
        self.delButton.setEnabled(mode)

    def setBezEditButtons(self,mode):
        self.sharpButton.setEnabled(mode)
        self.tangentButton.setEnabled(mode)
        self.symmetricButton.setEnabled(mode)

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
        if (not self.taskmode) or self.isTaskOn:
            self.isRelative.show()
            
    def setCopymode(self,val=0):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        p.SetBool("copymodeValue",bool(val))

    def relocate(self):
        "relocates the right-aligned buttons depending on the toolbar size"
        if self.baseWidget.geometry().width() < 400:
            self.layout.setDirection(QtGui.QBoxLayout.TopToBottom)
        else:
            self.layout.setDirection(QtGui.QBoxLayout.LeftToRight)

    def makeDumbTask(self,extra=None,callback=None):
        "create a dumb taskdialog to prevent deleting the temp object"
        class TaskPanel:
            def __init__(self,extra=None,callback=None):
                if extra:
                    self.form = [extra]
            def getStandardButtons(self):
                return int(QtGui.QDialogButtonBox.Close)
            def reject(self):
                if callback:
                    callback()
                return True
        FreeCADGui.Control.closeDialog()
        panel = TaskPanel(extra,callback)
        FreeCADGui.Control.showDialog(panel)
        
    def setGridSize(self,text):
        "sets the Draft grid to the given grid size"
        try:
            q = FreeCAD.Units.Quantity(text)
        except:
            pass
        else:
            Draft.setParam("gridSpacing",q.Value)
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
                
    def setMainline(self,val):
        "sets the grid main lines"
        if val > 1:
            Draft.setParam("gridEvery",val)
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
                
    def setCenterPlane(self,val):
        self.isCenterPlane = bool(val)

#---------------------------------------------------------------------------
# Processing functions
#---------------------------------------------------------------------------

    def getcol(self):
        "opens a color picker dialog"
        oldColor = self.color
        self.color=QtGui.QColorDialog.getColor()
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
        "opens a color picker dialog"
        oldColor = self.facecolor
        self.facecolor=QtGui.QColorDialog.getColor()
        if not QtGui.QColor.isValid(self.facecolor): #user canceled
            self.facecolor = oldColor
            return
        self.facecolorPix.fill(self.facecolor)
        self.facecolorButton.setIcon(QtGui.QIcon(self.facecolorPix))
        r = float(self.facecolor.red()/255.0)
        g = float(self.facecolor.green()/255.0)
        b = float(self.facecolor.blue()/255.0)
        col = (r,g,b,0.0)
        for i in FreeCADGui.Selection.getSelection():
            if "ShapeColor" in i.ViewObject.PropertiesList:
                i.ViewObject.ShapeColor = col

    def setwidth(self,val):
        self.linewidth = float(val)
        if Draft.getParam("saveonexit",False):
            Draft.setParam("linewidth",int(val))
        for i in FreeCADGui.Selection.getSelection():
            if "LineWidth" in i.ViewObject.PropertiesList:
                i.ViewObject.LineWidth = float(val)

    def setfontsize(self,val):
        self.fontsize = float(val)
        if Draft.getParam("saveonexit",False):
            Draft.setParam("textheight",float(val))
        for i in FreeCADGui.Selection.getSelection():
            if "FontSize" in i.ViewObject.PropertiesList:
                i.ViewObject.FontSize = float(val)

    def setContinue(self,val):
        self.continueMode = bool(val)

    def setFill(self,val):
        self.fillmode = bool(val)
        
    def apply(self):
        for i in FreeCADGui.Selection.getSelection():
            Draft.formatObject(i)

    def checkx(self):
        if self.yValue.isEnabled():
            self.yValue.setFocus()
            self.yValue.selectAll()
            self.updateSnapper()
        else:
            self.checky()

    def checky(self):
        if self.zValue.isEnabled():
            self.zValue.setFocus()
            self.zValue.selectAll()
            self.updateSnapper()
        else:
            self.validatePoint()
            
    def checkangle(self):
        self.angleValue.setFocus()
        self.angleValue.selectAll()
        self.updateSnapper()

    def validatePoint(self):
        "function for checking and sending numbers entered manually"
        if self.sourceCmd or self.pointcallback:
            if (self.labelRadius.isVisible()):
                try:
                    #rad=float(self.radiusValue.text())
                    rad = self.radius
                except ValueError:
                    pass
                else:
                    self.sourceCmd.numericRadius(rad)
            elif (self.offsetLabel.isVisible()):
                try:
                    #offset=float(self.offsetValue.text())
                    offset = self.offset
                except ValueError:
                    pass
                else:
                    self.sourceCmd.offsetHandler(offset)
            elif (self.labelx.isVisible()):
                try:
                    #numx=float(self.xValue.text())
                    numx = self.x
                    #numy=float(self.yValue.text())
                    numy = self.y
                    #numz=float(self.zValue.text())
                    numz = self.z
                except:
                    pass
                else:
                    if self.pointcallback:
                        self.pointcallback(FreeCAD.Vector(numx,numy,numz),self.relativeMode)
                    else:
                        if self.relativeMode:
                            if self.sourceCmd.node:
                                if self.sourceCmd.featureName == "Rectangle":
                                    last = self.sourceCmd.node[0]
                                else:
                                    last = self.sourceCmd.node[-1]
                                #print("last:",last)
                                v = FreeCAD.Vector(numx,numy,numz)
                                #print("orig:",v)
                                if FreeCAD.DraftWorkingPlane:
                                    v = FreeCAD.Vector(numx,numy,numz)
                                    v = FreeCAD.DraftWorkingPlane.getGlobalRot(v)
                                    #print("rotated:",v)
                                numx = last.x + v.x
                                numy = last.y + v.y
                                numz = last.z + v.z
                        self.sourceCmd.numericInput(numx,numy,numz)
            elif (self.textValue.isVisible() or self.SStringValue.isVisible() or self.SSizeValue.isVisible() or self.STrackValue.isVisible() or self.FFileValue.isVisible()):
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
#                    fname = utf8_decode(fname[0])  # 1947: utf8_decode fails ('ascii' codec can't encode character)
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
                    self.sourceCmd.validFFile(utf8_decode(self.FFileValue.text()))       #QString to PyString
                else:
                    FreeCAD.Console.PrintMessage(translate("draft", "Please enter a font file."))                    


    def finish(self):
        "finish button action"
        if self.sourceCmd:
            self.sourceCmd.finish(False)
        if self.cancel:
            self.cancel()
            self.cancel = None
        if FreeCADGui.ActiveDocument:
            FreeCADGui.ActiveDocument.resetEdit()

    def escape(self):
        "escapes the current command"
        self.continueMode = False
        if not self.taskmode:
            self.continueCmd.setChecked(False)
        self.finish()

    def closeLine(self):
        "close button action"
        self.sourceCmd.finish(True)
        FreeCADGui.ActiveDocument.resetEdit()

    def wipeLine(self):
        "wipes existing segments of a line"
        self.sourceCmd.wipe()

    def orientWP(self):
        "reorients the current working plane"
        self.sourceCmd.orientWP()

    def selectEdge(self):
        "allows the dimension command to select an edge"
        if hasattr(self.sourceCmd,"selectEdge"):
            self.sourceCmd.selectEdge()

    def selectXY(self):
        self.sourceCmd.selectHandler("XY")

    def selectXZ(self):
        self.sourceCmd.selectHandler("XZ")

    def selectYZ(self):
        self.sourceCmd.selectHandler("YZ")

    def selectCurrentView(self):
        self.sourceCmd.selectHandler("currentView")

    def selectResetPlane(self):
        self.sourceCmd.selectHandler("reset")

    def undoSegment(self):
        "undo last line segment"
        if hasattr(self.sourceCmd,"undolast"):
            self.sourceCmd.undolast()

    def checkSpecialChars(self,txt):
        '''
        checks for special characters in the entered coords that must be
        treated as shortcuts
        '''

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
        if txt.upper().endswith(inCommandShortcuts["Relative"][0]):
            self.isRelative.setChecked(not self.isRelative.isChecked())
            self.relativeMode = self.isRelative.isChecked()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["Fill"][0]):
            if self.hasFill.isVisible():
                self.hasFill.setChecked(not self.hasFill.isChecked())
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["Exit"][0]):
            if self.finishButton.isVisible():
                self.finish()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["Continue"][0]):
            self.toggleContinue()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["Wipe"][0]):
            self.wipeLine()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["SelectEdge"][0]):
            self.selectEdge()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["Snap"][0]):
            self.togglesnap()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["Increase"][0]):
            self.toggleradius(1)
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["Decrease"][0]):
            self.toggleradius(-1)
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["AddHold"][0]):
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.addHoldPoint()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["RestrictX"][0]):
            self.constrain("x")
            self.displayPoint()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["RestrictY"][0]):
            self.constrain("y")
            self.displayPoint()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["RestrictZ"][0]):
            self.constrain("z")
            self.displayPoint()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["Length"][0]):
            self.constrain("angle")
            self.displayPoint()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["Close"][0]):
            if self.closeButton.isVisible():
                self.closeLine()
        elif txt.upper().endswith(inCommandShortcuts["SetWP"][0]):
            self.orientWP()
            spec = True
        elif txt.upper().endswith(inCommandShortcuts["Copy"][0]):
            if self.isCopy.isVisible():
                self.isCopy.setChecked(not self.isCopy.isChecked())
            spec = True
        if spec:
            for i,k in enumerate([self.xValue,self.yValue,self.zValue,self.lengthValue,self.angleValue]):
                if (k.property("text") == txt):
                    #print "debug:matching:",k.property("text")
                    if i == 0:
                        v = FreeCAD.Units.Quantity(self.x,FreeCAD.Units.Length).getUserPreferred()[0]
                    elif i == 1:
                        v = FreeCAD.Units.Quantity(self.y,FreeCAD.Units.Length).getUserPreferred()[0]
                    elif i == 2:
                        v = FreeCAD.Units.Quantity(self.z,FreeCAD.Units.Length).getUserPreferred()[0]
                    elif i == 3:
                        v = FreeCAD.Units.Quantity(self.lvalue,FreeCAD.Units.Length).getUserPreferred()[0]
                    else:
                        v = FreeCAD.Units.Quantity(self.avalue,FreeCAD.Units.Angle).getUserPreferred()[0]
                    k.setProperty("text",v)
                    k.setFocus()
                    k.selectAll()
        self.updateSnapper()
                    
    def updateSnapper(self):
        "updates the snapper track line if applicable"
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
                    delta = FreeCAD.DraftWorkingPlane.getGlobalCoords(FreeCAD.Vector(self.x,self.y,self.z))
                    FreeCADGui.Snapper.trackLine.p2(last.add(delta))

    def storeCurrentText(self,qstr):
        self.currEditText = self.textValue.text()

    def setCurrentText(self,tstr):
        if (not self.taskmode) or (self.taskmode and self.isTaskOn):
            self.textValue.setText(tstr)
    
    def sendText(self):
        '''
        this function sends the entered text to the active draft command
        if enter has been pressed twice. Otherwise it blanks the line.
        '''
        if self.textline == len(self.textbuffer):
            if self.textline:
                if not self.currEditText:
                    self.sourceCmd.text=self.textbuffer
                    self.sourceCmd.createObject()
            self.textbuffer.append(self.currEditText)
            self.textline += 1
            self.setCurrentText('')
        elif self.textline < len(self.textbuffer):
            self.textbuffer[self.textline] = self.currEditText
            self.textline += 1
            if self.textline < len(self.textbuffer):
                self.setCurrentText(self.textbuffer[self.textline])
            else:
                self.setCurrentText('')

    def lineUp(self):
        "displays previous line in text editor"
        if self.textline:
            if self.textline == len(self.textbuffer):
                self.textbuffer.append(self.textValue.text())
                self.textline -= 1
                if self.textValue.text():
                    self.textValue.setText(self.textbuffer[self.textline])
            elif self.textline < len(self.textbuffer):
                self.textbuffer[self.textline] = self.textValue.text()
                self.textline -= 1
                self.textValue.setText(self.textbuffer[self.textline])

    def displayPoint(self, point=None, last=None, plane=None, mask=None):
        "this function displays the passed coords in the x, y, and z widgets"

        if (not self.taskmode) or self.isTaskOn:

            # get coords to display
            dp = None
            if point:
                dp = point
                if self.relativeMode and (last != None):
                    if plane:
                        dp = plane.getLocalRot(FreeCAD.Vector(point.x-last.x, point.y-last.y, point.z-last.z))
                    else:
                        dp = FreeCAD.Vector(point.x-last.x, point.y-last.y, point.z-last.z)
                elif plane:
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
                self.lengthValue.setText(displayExternal(dp.Length,None,'Length'))
                a = math.degrees(-DraftVecUtils.angle(dp,plane.u,plane.axis))
                if not self.angleLock.isChecked():
                    self.angleValue.setText(displayExternal(a,None,'Angle'))
                if not mask:
                    # automask
                    if a in [0,180,-180]:
                        mask = "x"
                    elif a in [90,270,-90]:
                        mask = "y"
                
            # set masks
            if (mask == "x") or (self.mask == "x"):
                self.xValue.setEnabled(True)
                self.yValue.setEnabled(False)
                self.zValue.setEnabled(False)
                self.setFocus()
            elif (mask == "y") or (self.mask == "y"):
                self.xValue.setEnabled(False)
                self.yValue.setEnabled(True)
                self.zValue.setEnabled(False)
                self.yValue.setFocus()
                self.yValue.selectAll()
            elif (mask == "z") or (self.mask == "z"):
                self.xValue.setEnabled(False)
                self.yValue.setEnabled(False)
                self.zValue.setEnabled(True)
                self.zValue.setFocus()
                self.zValue.selectAll()
            else:
                self.xValue.setEnabled(True)
                self.yValue.setEnabled(True)
                self.zValue.setEnabled(True)
                self.setFocus()
                
            
    def getDefaultColor(self,type,rgb=False):
        "gets color from the preferences or toolbar"
        r = 0
        g = 0
        b = 0
        if type == "snap":
            color = Draft.getParam("snapcolor",4294967295)
            r = ((color>>24)&0xFF)/255
            g = ((color>>16)&0xFF)/255
            b = ((color>>8)&0xFF)/255
        elif type == "ui":
            r = float(self.color.red()/255.0)
            g = float(self.color.green()/255.0)
            b = float(self.color.blue()/255.0)
        elif type == "face":
            r = float(self.facecolor.red()/255.0)
            g = float(self.facecolor.green()/255.0)
            b = float(self.facecolor.blue()/255.0)
        elif type == "constr":
            color = QtGui.QColor(Draft.getParam("constructioncolor",746455039)>>8)
            r = color.red()/255.0
            g = color.green()/255.0
            b = color.blue()/255.0
        else: 
            print("draft: error: couldn't get a color for ",type," type.")
        if rgb:
            return("rgb("+str(int(r*255))+","+str(int(g*255))+","+str(int(b*255))+")")
        else:
            return (r,g,b)

    def cross(self,on=True):
        "deprecated"
        pass
        
    def toggleConstrMode(self,checked):
        self.baseWidget.setStyleSheet("#constrButton:Checked {background-color: "+self.getDefaultColor("constr",rgb=True)+" }")
        self.constrMode = checked

    def toggleContinue(self):
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
        except:
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

    def popupMenu(self,mlist):
        "pops up a menu filled with the given list"
        self.groupmenu = QtGui.QMenu()
        for i in mlist:
            self.groupmenu.addAction(i)
        pos = FreeCADGui.getMainWindow().cursor().pos()
        self.groupmenu.popup(pos)
        QtCore.QObject.connect(self.groupmenu,QtCore.SIGNAL("triggered(QAction *)"),self.popupTriggered)

    def popupTriggered(self,action):
        self.sourceCmd.proceed(str(action.text()))

    def setAddMode(self,bool):
        if self.addButton.isChecked():
            self.delButton.setChecked(False)
            self.symmetricButton.setChecked(False)
            self.sharpButton.setChecked(False)
            self.tangentButton.setChecked(False)

    def setDelMode(self,bool):
        if self.delButton.isChecked():
            self.addButton.setChecked(False)
            self.symmetricButton.setChecked(False)
            self.sharpButton.setChecked(False)
            self.tangentButton.setChecked(False)

    def setSharpMode(self,bool):
        if self.sharpButton.isChecked():
            self.tangentButton.setChecked(False)
            self.symmetricButton.setChecked(False)
            self.addButton.setChecked(False)
            self.delButton.setChecked(False)

    def setTangentMode(self,bool):
        if self.tangentButton.isChecked():
            self.sharpButton.setChecked(False)
            self.symmetricButton.setChecked(False)
            self.addButton.setChecked(False)
            self.delButton.setChecked(False)

    def setSymmetricMode(self,bool):
        if self.symmetricButton.isChecked():
            self.sharpButton.setChecked(False)
            self.tangentButton.setChecked(False)
            self.addButton.setChecked(False)
            self.delButton.setChecked(False)

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
        if value == None:
            self.autogroup = None
            self.autoGroupButton.setText("None")
            self.autoGroupButton.setIcon(QtGui.QIcon(':/icons/Draft_AutoGroup_off.svg'))
            self.autoGroupButton.setToolTip(translate("draft", "Autogroup off"))
            self.autoGroupButton.setDown(False)
        else:
            obj = FreeCAD.ActiveDocument.getObject(value)
            if obj:
                self.autogroup = value
                self.autoGroupButton.setText(obj.Label)
                self.autoGroupButton.setIcon(QtGui.QIcon(':/icons/Draft_AutoGroup_on.svg'))
                self.autoGroupButton.setToolTip(translate("draft", "Autogroup: ")+obj.Label)
                self.autoGroupButton.setDown(False)
            else:
                self.autogroup = None
                self.autoGroupButton.setText("None")
                self.autoGroupButton.setIcon(QtGui.QIcon(':/icons/Draft_AutoGroup_off.svg'))
                self.autoGroupButton.setToolTip(translate("draft", "Autogroup off"))
                self.autoGroupButton.setDown(False)

    def show(self):
        if not self.taskmode:
            self.draftWidget.setVisible(True)

    def hide(self):
        if not self.taskmode:
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
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.toggle()

    def toggleradius(self,val):
        if hasattr(FreeCADGui,"Snapper"):
            par = Draft.getParam("snapRange",10)
            Draft.setParam("snapRange",par+val)
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

    def changeYValue(self,d):
        self.y = d

    def changeZValue(self,d):
        self.z = d

    def changeRadiusValue(self,d):
        self.radius = d

    def changeOffsetValue(self,d):
        self.offset = d

    def changeSSizeValue(self,d):
        self.SSize = d

    def changeSTrackValue(self,d):
        self.STrack = d
        
    def changeLengthValue(self,d):
        self.lvalue = d
        v = FreeCAD.Vector(self.x,self.y,self.z)
        if not v.Length:
            if self.angle:
                v = FreeCAD.Vector(self.angle)
            else:
                v = FreeCAD.Vector(FreeCAD.DraftWorkingPlane.u)
                if self.avalue:
                    v = DraftVecUtils.rotate(v,math.radians(d),FreeCAD.DraftWorkingPlane.axis)
        v = DraftVecUtils.scaleTo(v,d)
        self.xValue.setText(displayExternal(v.x,None,'Length'))
        self.yValue.setText(displayExternal(v.y,None,'Length'))
        self.zValue.setText(displayExternal(v.z,None,'Length'))
        
    def changeAngleValue(self,d):
        self.avalue = d
        v = FreeCAD.Vector(self.x,self.y,self.z)
        a = DraftVecUtils.angle(v,FreeCAD.DraftWorkingPlane.u,FreeCAD.DraftWorkingPlane.axis)
        a = math.radians(d)+a
        v = DraftVecUtils.rotate(v,a,FreeCAD.DraftWorkingPlane.axis)
        self.angle = v
        self.xValue.setText(displayExternal(v.x,None,'Length'))
        self.yValue.setText(displayExternal(v.y,None,'Length'))
        self.zValue.setText(displayExternal(v.z,None,'Length'))
        if self.angleLock.isChecked():
            FreeCADGui.Snapper.setAngle(self.angle)
        
    def toggleAngle(self,b):
        self.alock = self.angleLock.isChecked()
        if b:
            FreeCADGui.Snapper.setAngle(self.angle)
        else:
            FreeCADGui.Snapper.setAngle()
            self.angle = None


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
        else:
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
        else:
            self.draftWidget.setVisible(False)
            self.draftWidget.toggleViewAction().setVisible(False)
            
            
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
        'fills the treewidget'
        self.tree.clear()
        if self.obj:
            for f in self.obj.Faces:
                if isinstance(f[1],tuple):
                    for subf in f[1]:
                        item = QtGui.QTreeWidgetItem(self.tree)
                        item.setText(0,f[0].Name)
                        item.setIcon(0,QtGui.QIcon(":/icons/Tree_Part.svg"))
                        item.setText(1,subf)  
                else:
                    item = QtGui.QTreeWidgetItem(self.tree)
                    item.setText(0,f[0].Name)
                    item.setIcon(0,QtGui.QIcon(":/icons/Tree_Part.svg"))
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


class ScaleTaskPanel:

    '''A Task Panel for the Scale tool'''

    def __init__(self):
        self.sourceCmd = None
        self.form = QtGui.QWidget()
        layout = QtGui.QGridLayout(self.form)
        self.xLabel = QtGui.QLabel()
        layout.addWidget(self.xLabel,0,0,1,1)
        self.xValue = QtGui.QDoubleSpinBox()
        self.xValue.setDecimals(Draft.getParam("precision"))
        self.xValue.setRange(.0000001,1000000.0)
        self.xValue.setValue(1)
        layout.addWidget(self.xValue,0,1,1,1)
        self.yLabel = QtGui.QLabel()
        layout.addWidget(self.yLabel,1,0,1,1)
        self.yValue = QtGui.QDoubleSpinBox()
        self.yValue.setDecimals(Draft.getParam("precision"))
        self.yValue.setRange(.0000001,1000000.0)
        self.yValue.setValue(1)
        layout.addWidget(self.yValue,1,1,1,1)
        self.zLabel = QtGui.QLabel()
        layout.addWidget(self.zLabel,2,0,1,1)
        self.zValue = QtGui.QDoubleSpinBox()
        self.zValue.setDecimals(Draft.getParam("precision"))
        self.zValue.setRange(.0000001,1000000.0)
        self.zValue.setValue(1)
        layout.addWidget(self.zValue,2,1,1,1)
        self.lock = QtGui.QCheckBox()
        layout.addWidget(self.lock,3,0,1,2)
        self.relative = QtGui.QCheckBox()
        layout.addWidget(self.relative,4,0,1,2)
        self.rLabel = QtGui.QLabel()
        layout.addWidget(self.rLabel,5,0,1,2)
        self.isClone = QtGui.QRadioButton()
        layout.addWidget(self.isClone,6,0,1,2)
        self.isClone.setChecked(True)
        self.isOriginal = QtGui.QRadioButton()
        layout.addWidget(self.isOriginal,7,0,1,2)
        self.isCopy = QtGui.QRadioButton()
        layout.addWidget(self.isCopy,8,0,1,2)
        self.pickrefButton = QtGui.QPushButton()
        layout.addWidget(self.pickrefButton,9,0,1,2)
        QtCore.QObject.connect(self.xValue,QtCore.SIGNAL("valueChanged(double)"),self.setValue)
        QtCore.QObject.connect(self.yValue,QtCore.SIGNAL("valueChanged(double)"),self.setValue)
        QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("valueChanged(double)"),self.setValue)
        QtCore.QObject.connect(self.pickrefButton,QtCore.SIGNAL("clicked()"),self.pickRef)
        self.retranslateUi()
        
    def setValue(self,val=None):
        if self.lock.isChecked():
            self.xValue.setValue(val)
            self.yValue.setValue(val)
            self.zValue.setValue(val)
        if self.sourceCmd:
            self.sourceCmd.scaleGhost(self.xValue.value(),self.yValue.value(),self.zValue.value(),self.relative.isChecked())
        
    def retranslateUi(self,widget=None):
        self.form.setWindowTitle(QtGui.QApplication.translate("Draft", "Scale", None))
        self.xLabel.setText(QtGui.QApplication.translate("Draft", "X factor", None))
        self.yLabel.setText(QtGui.QApplication.translate("Draft", "Y factor", None))
        self.zLabel.setText(QtGui.QApplication.translate("Draft", "Z factor", None))
        self.lock.setText(QtGui.QApplication.translate("Draft", "Uniform scaling", None))
        self.relative.setText(QtGui.QApplication.translate("Draft", "Working plane orientation", None))
        self.rLabel.setText(QtGui.QApplication.translate("Draft", "Result", None))
        self.isClone.setText(QtGui.QApplication.translate("Draft", "Create a clone", None))
        self.isOriginal.setText(QtGui.QApplication.translate("Draft", "Modify original", None))
        self.isCopy.setText(QtGui.QApplication.translate("Draft", "Create a copy", None))
        self.pickrefButton.setText(QtGui.QApplication.translate("Draft", "Pick from/to points", None))

    def pickRef(self):
        if self.sourceCmd:
            self.sourceCmd.pickRef()

    def accept(self):
        if self.sourceCmd:
            x = self.xValue.value()
            y = self.yValue.value()
            z = self.zValue.value()
            rel = self.relative.isChecked()
            if self.isClone.isChecked():
                mod = 0
            elif self.isOriginal.isChecked():
                mod = 1
            else:
                mod = 2
            self.sourceCmd.scale(x,y,z,rel,mod)
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):
        if self.sourceCmd:
            self.sourceCmd.finish()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

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

class ShapeStringTaskPanel:
    '''A TaskPanel for ShapeString'''
    def __init__(self):
        self.form = QtGui.QWidget()
        self.form.setObjectName("ShapeStringTaskPanel")
        self.form.setWindowTitle(translate("draft","ShapeString"))
        layout = QtGui.QVBoxLayout(self.form)
        uiFile = QtCore.QFile(u":/ui/TaskShapeString.ui")  #this has to change if ui not in Resource file
        loader = FreeCADGui.UiLoader()
        self.task = loader.load(uiFile)
        layout.addWidget(self.task)
        
        qStart = FreeCAD.Units.Quantity(0.0, FreeCAD.Units.Length)
        self.task.sbX.setProperty('rawValue',qStart.Value)
        self.task.sbX.setProperty('unit',qStart.getUserPreferred()[2])
        self.task.sbY.setProperty('rawValue',qStart.Value)
        self.task.sbY.setProperty('unit',qStart.getUserPreferred()[2])
        self.task.sbZ.setProperty('rawValue',qStart.Value)
        self.task.sbZ.setProperty('unit',qStart.getUserPreferred()[2])
        self.task.sbHeight.setProperty('rawValue',10.0)
        self.task.sbHeight.setProperty('unit',qStart.getUserPreferred()[2])

        self.stringText = translate("draft","Default")
        self.task.leString.setText(self.stringText)
        self.task.fcFontFile.setFileName(Draft.getParam("FontFile",""))
        self.fileSpec = Draft.getParam("FontFile","")

        QtCore.QObject.connect(self.task.fcFontFile,QtCore.SIGNAL("fileNameSelected(const QString&)"),self.fileSelect)

    def fileSelect(self, fn):
        self.fileSpec = fn

    def accept(self):
        FreeCAD.ActiveDocument.openTransaction("ShapeString")
        qr,sup,points,fil = self.sourceCmd.getStrings()
        height = FreeCAD.Units.Quantity(self.task.sbHeight.text()).Value
        ss = Draft.makeShapeString(str(self.task.leString.text()),  ##needs to be bytes for Py3!
                                   str(self.fileSpec),
                                   height,
                                   0.0)

        x = FreeCAD.Units.Quantity(self.task.sbX.text()).Value
        y = FreeCAD.Units.Quantity(self.task.sbY.text()).Value
        z = FreeCAD.Units.Quantity(self.task.sbZ.text()).Value
        ssBase = FreeCAD.Vector(x,y,z)
        plm=FreeCAD.Placement()
        plm.Base = ssBase
        elements = qr[1:-1].split(",")  #string to tuple
        mytuple = tuple(elements)       #to prevent
        plm.Rotation.Q = mytuple        #PyCXX: Error creating object of type N2Py5TupleE from '(0.0,-0.0,-0.0,1.0)'
        ss.Placement=plm
        if sup:
            ss.Support = FreeCAD.ActiveDocument.getObject(sup)
        Draft.autogroup(ss)
        FreeCAD.ActiveDocument.commitTransaction()

        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):
        FreeCADGui.ActiveDocument.resetEdit()
        return True

if not hasattr(FreeCADGui,"draftToolBar"):
    FreeCADGui.draftToolBar = DraftToolBar()
#----End of Python Features Definitions----#
   
if not hasattr(FreeCADGui,"Snapper"):
    import DraftSnap
