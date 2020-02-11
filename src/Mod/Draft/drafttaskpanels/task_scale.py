"""Provide the task panel for the Draft Scale tool."""
## @package task_scale
# \ingroup DRAFT
# \brief Provide the task panel for the Draft Scale tool.

# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
import sys
import FreeCAD as App
import FreeCADGui as Gui
import Draft
import Draft_rc
import DraftVecUtils
import DraftTools
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui
from draftutils.translate import translate
from draftutils.messages import _msg, _err

_Quantity = App.Units.Quantity


# So the resource file doesn't trigger errors from code checkers (flake8)
True if Draft_rc.__name__ else False


class ScaleTaskPanel:
    """A Task Panel for the Scale tool"""

    def __init__(self):
        self.sourceCmd = None
        self.form = QtGui.QWidget()
        layout = QtGui.QGridLayout(self.form)
        self.xLabel = QtGui.QLabel()
        layout.addWidget(self.xLabel,0,0,1,1)
        self.xValue = QtGui.QDoubleSpinBox()
        self.xValue.setRange(.0000001,1000000.0)
        self.xValue.setDecimals(Draft.getParam("precision"))
        self.xValue.setValue(1)
        layout.addWidget(self.xValue,0,1,1,1)
        self.yLabel = QtGui.QLabel()
        layout.addWidget(self.yLabel,1,0,1,1)
        self.yValue = QtGui.QDoubleSpinBox()
        self.yValue.setRange(.0000001,1000000.0)
        self.yValue.setDecimals(Draft.getParam("precision"))
        self.yValue.setValue(1)
        layout.addWidget(self.yValue,1,1,1,1)
        self.zLabel = QtGui.QLabel()
        layout.addWidget(self.zLabel,2,0,1,1)
        self.zValue = QtGui.QDoubleSpinBox()
        self.zValue.setRange(.0000001,1000000.0)
        self.zValue.setDecimals(Draft.getParam("precision"))
        self.zValue.setValue(1)
        layout.addWidget(self.zValue,2,1,1,1)
        self.lock = QtGui.QCheckBox()
        self.lock.setChecked(App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("ScaleUniform", False))
        layout.addWidget(self.lock,3,0,1,2)
        self.relative = QtGui.QCheckBox()
        self.relative.setChecked(App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("ScaleRelative", False))
        layout.addWidget(self.relative,4,0,1,2)
        self.isCopy = QtGui.QCheckBox()
        self.isCopy.setChecked(App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("ScaleCopy", False))
        layout.addWidget(self.isCopy,5,0,1,2)
        self.isSubelementMode = QtGui.QCheckBox()
        layout.addWidget(self.isSubelementMode,6,0,1,2)
        self.isClone = QtGui.QCheckBox()
        layout.addWidget(self.isClone,7,0,1,2)
        self.isClone.setChecked(App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("ScaleClone", False))
        self.pickrefButton = QtGui.QPushButton()
        layout.addWidget(self.pickrefButton,8,0,1,2)
        QtCore.QObject.connect(self.xValue,QtCore.SIGNAL("valueChanged(double)"),self.setValue)
        QtCore.QObject.connect(self.yValue,QtCore.SIGNAL("valueChanged(double)"),self.setValue)
        QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("valueChanged(double)"),self.setValue)
        QtCore.QObject.connect(self.pickrefButton,QtCore.SIGNAL("clicked()"),self.pickRef)
        QtCore.QObject.connect(self.lock,QtCore.SIGNAL("toggled(bool)"),self.setLock)
        QtCore.QObject.connect(self.relative,QtCore.SIGNAL("toggled(bool)"),self.setRelative)
        QtCore.QObject.connect(self.isCopy,QtCore.SIGNAL("toggled(bool)"),self.setCopy)
        QtCore.QObject.connect(self.isClone,QtCore.SIGNAL("toggled(bool)"),self.setClone)
        self.retranslateUi()

    def setLock(self,state):
        App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetBool("ScaleUniform", state)

    def setRelative(self,state):
        App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetBool("ScaleRelative", state)

    def setCopy(self,state):
        App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetBool("ScaleCopy", state)
        if state and self.isClone.isChecked():
            self.isClone.setChecked(False)

    def setClone(self,state):
        App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").SetBool("ScaleClone", state)
        if state and self.isCopy.isChecked():
            self.isCopy.setChecked(False)

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
        self.isCopy.setText(QtGui.QApplication.translate("draft", "Copy"))
        self.isSubelementMode.setText(QtGui.QApplication.translate("draft", "Modify subelements"))
        self.pickrefButton.setText(QtGui.QApplication.translate("Draft", "Pick from/to points", None))
        self.isClone.setText(QtGui.QApplication.translate("Draft", "Create a clone", None))

    def pickRef(self):
        if self.sourceCmd:
            self.sourceCmd.pickRef()

    def accept(self):
        if self.sourceCmd:
            self.sourceCmd.scale()
        Gui.ActiveDocument.resetEdit()
        return True

    def reject(self):
        if self.sourceCmd:
            self.sourceCmd.finish()
        Gui.ActiveDocument.resetEdit()
        return True
