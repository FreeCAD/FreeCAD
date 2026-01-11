# ***************************************************************************
# *   Copyright (c) 2023 Syres                                              *
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
"""Provides the TechDraw FillTemplateFields Task Dialog."""

__title__ = "TechDrawTools.CommandFillTemplateFields"
__author__ = "Syres"
__url__ = "https://www.freecad.org"
__version__ = "00.02"
__date__ = "2023/12/07"

from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtCore, QtGui
from PySide.QtGui import QLineEdit, QCheckBox
import FreeCAD as App
import FreeCADGui as Gui
import datetime
from datetime import date
import csv
import codecs
from fractions import Fraction
import os.path

CreatedByChkLst = []
ScaleChkLst = []
LabelChkLst = []
CommentChkLst = []
CompanyChkLst = []
LicenseChkLst = []
CreatedDateChkLst = []
LastModifiedDateChkLst = []
listofkeys = [
    "CreatedByChkLst",
    "ScaleChkLst",
    "LabelChkLst",
    "CommentChkLst",
    "CompanyChkLst",
    "LicenseChkLst",
    "CreatedDateChkLst",
    "LastModifiedDateChkLst",
]

"""Run the following code when the command is activated (button press)."""
file_path = App.getResourceDir() + "Mod/TechDraw/CSVdata/FillTemplateFields.csv"

if os.path.exists(file_path):
    with codecs.open(file_path, encoding="utf-8") as fp:
        reader = csv.DictReader(fp)
        if listofkeys == reader.fieldnames:
            for row in reader:
                CreatedByChkLst.append(row["CreatedByChkLst"])
                ScaleChkLst.append(row["ScaleChkLst"])
                LabelChkLst.append(row["LabelChkLst"])
                CommentChkLst.append(row["CommentChkLst"])
                CompanyChkLst.append(row["CompanyChkLst"])
                LicenseChkLst.append(row["LicenseChkLst"])
                CreatedDateChkLst.append(row["CreatedDateChkLst"])
                LastModifiedDateChkLst.append(row["LastModifiedDateChkLst"])
        else:
            errorMsg = QtCore.QT_TRANSLATE_NOOP(
                "Techdraw_FillTemplateFields",
                " file does not contain the correct field names therefore exiting",
            )
            App.Console.PrintError(file_path + errorMsg + "\n")
else:
    errorMsg = QtCore.QT_TRANSLATE_NOOP(
        "Techdraw_FillTemplateFields",
        " file has not been found therefore exiting",
    )
    App.Console.PrintError(file_path + errorMsg + "\n")

keyLst = []


class TaskFillTemplateFields:
    def __init__(self):
        objs = App.ActiveDocument.findObjects(Type="TechDraw::DrawPage")

        for obj in objs:
            if (
                os.path.exists(file_path)
                and listofkeys == reader.fieldnames
            ):
                self.page = obj
                if obj.Views == []:
                    msgBox = QtGui.QMessageBox()
                    msgTitle = QtCore.QT_TRANSLATE_NOOP(
                        "Techdraw_FillTemplateFields",
                        "View or projection group missing",
                    )
                    msg = QtCore.QT_TRANSLATE_NOOP(
                        "Techdraw_FillTemplateFields",
                        "There must be a view or projection group to"
                        " establish data for the scale field in " + self.page.Label,
                    )
                    msgBox.setText(msg)
                    msgBox.setWindowTitle(msgTitle)
                    msgBox.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)
                    msgBox.exec_()
                    break

                projgrp_view = None
                for pageObj in obj.Views:
                    if hasattr(pageObj, "Scale"):
                        # use the scale from the first DVP or DPG encountered to fill the template's
                        # Scale editable text.
                        projgrp_view = pageObj
                        break

                self.texts = self.page.Template.EditableTexts

                self.dialog = None
                self.s1 = None

                self.dialog = QtGui.QDialog()
                self.dialog.resize(1050, 400)
                self.dialog.setWindowTitle(
                    QtCore.QT_TRANSLATE_NOOP(
                        "TechDraw_FillTemplateFields", "Fill Template Fields In "
                    )
                    + self.page.Label
                )
                self.dialog.move(400, 200)
                self.la = QtGui.QGridLayout(self.dialog)
                updateCb = QtCore.QT_TRANSLATE_NOOP(
                    "TechDraw_FillTemplateFields", "Update"
                )
                icon_name = "button_right"
                svg = ":/icons/" + icon_name
                pix = QtGui.QPixmap(svg)
                self.checkBoxList = []
                self.lineTextList = []
                dialogRow = 0
                longestText = 0
                for key, value in self.texts.items():
                    App.Console.PrintLog("{0} = {1} | ".format(key, value))
                    if str(key).lower() in CreatedByChkLst:
                        t1 = QtGui.QLabel(value)
                        self.la.addWidget(t1, dialogRow, 0)
                        self.cb1 = QtGui.QCheckBox(updateCb)
                        self.cb1.setObjectName(key)
                        if hasattr(self.cb1, "setCheckState"):
                            self.cb1.setCheckState(QtCore.Qt.Checked)
                        else:
                            self.cb1.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb1, dialogRow, 1)
                        u1 = QtGui.QLabel("")
                        u1.setPixmap(pix.scaled(32, 32))
                        self.la.addWidget(u1, dialogRow, 2)
                        self.s1 = QtGui.QLineEdit()
                        self.s1.setText(App.ActiveDocument.CreatedBy)
                        self.la.addWidget(self.s1, dialogRow, 3)
                        self.cb1.setObjectName(key)
                        keyLst.append(self.cb1.objectName())
                        self.checkBoxList.append(self.cb1)
                        self.lineTextList.append(self.s1)
                        self.cb1.clicked.connect(self.on_cb1_clicked)
                        longestText = max(
                            longestText, len(App.ActiveDocument.CreatedBy)
                        )
                        dialogRow += 1
                    if str(key).lower() in ScaleChkLst and projgrp_view:
                        t2 = QtGui.QLabel(value)
                        self.la.addWidget(t2, dialogRow, 0)
                        self.cb2 = QtGui.QCheckBox(updateCb)
                        self.cb2.setObjectName(key)
                        if hasattr(self.cb2, "setCheckState"):
                            self.cb2.setCheckState(QtCore.Qt.Checked)
                        else:
                            self.cb2.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb2, dialogRow, 1)
                        u2 = QtGui.QLabel("")
                        u2.setPixmap(pix.scaled(32, 32))
                        self.la.addWidget(u2, dialogRow, 2)
                        self.s2 = QtGui.QLineEdit()
                        self.la.addWidget(self.s2, dialogRow, 3)
                        self.cb2.setObjectName(key)
                        keyLst.append(self.cb2.objectName())
                        self.checkBoxList.append(self.cb2)
                        self.lineTextList.append(self.s2)
                        self.cb2.clicked.connect(self.on_cb2_clicked)
                        if projgrp_view.Scale < 1:
                            fracScale = Fraction(projgrp_view.Scale).limit_denominator()
                            self.s2.setText(
                                str(fracScale.numerator)
                                + " : "
                                + str(fracScale.denominator)
                            )
                        elif int(projgrp_view.Scale) == 1 or (
                            projgrp_view.Scale > 1
                            and int(projgrp_view.Scale) == projgrp_view.Scale
                        ):
                            self.s2.setText(str(int(projgrp_view.Scale)) + " : 1")
                        else:  # must be something like 2.5 = 5 : 2
                            for x in range(2, 10):
                                if (
                                    int(projgrp_view.Scale * x)
                                    == projgrp_view.Scale * x
                                ):
                                    fracScale = Fraction(projgrp_view.Scale)
                                    self.s2.setText(
                                        str(fracScale.numerator)
                                        + " : "
                                        + str(fracScale.denominator)
                                    )
                                    break
                        dialogRow += 1
                    if str(key).lower() in LabelChkLst:
                        t3 = QtGui.QLabel(value)
                        self.la.addWidget(t3, dialogRow, 0)
                        self.cb3 = QtGui.QCheckBox(updateCb)
                        self.cb3.setObjectName(key)
                        if hasattr(self.cb3, "setCheckState"):
                            self.cb3.setCheckState(QtCore.Qt.Checked)
                        else:
                            self.cb3.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb3, dialogRow, 1)
                        u3 = QtGui.QLabel("")
                        u3.setPixmap(pix.scaled(32, 32))
                        self.la.addWidget(u3, dialogRow, 2)
                        self.s3 = QtGui.QLineEdit()
                        self.la.addWidget(self.s3, dialogRow, 3)
                        self.s3.setText(App.ActiveDocument.Label)
                        self.cb3.setObjectName(key)
                        keyLst.append(self.cb3.objectName())
                        self.checkBoxList.append(self.cb3)
                        self.lineTextList.append(self.s3)
                        self.cb3.clicked.connect(self.on_cb3_clicked)
                        longestText = max(longestText, len(App.ActiveDocument.Label))
                        dialogRow += 1
                    if str(key).lower() in CommentChkLst:
                        t4 = QtGui.QLabel(value)
                        self.la.addWidget(t4, dialogRow, 0)
                        self.cb4 = QtGui.QCheckBox(updateCb)
                        self.cb4.setObjectName(key)
                        if hasattr(self.cb4, "setCheckState"):
                            self.cb4.setCheckState(QtCore.Qt.Checked)
                        else:
                            self.cb4.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb4, dialogRow, 1)
                        u4 = QtGui.QLabel("")
                        u4.setPixmap(pix.scaled(32, 32))
                        self.la.addWidget(u4, dialogRow, 2)
                        self.s4 = QtGui.QLineEdit()
                        self.la.addWidget(self.s4, dialogRow, 3)
                        self.s4.setText(App.ActiveDocument.Comment)
                        self.cb4.setObjectName(key)
                        keyLst.append(self.cb4.objectName())
                        self.checkBoxList.append(self.cb4)
                        self.lineTextList.append(self.s4)
                        self.cb4.clicked.connect(self.on_cb4_clicked)
                        longestText = max(longestText, len(App.ActiveDocument.Comment))
                        dialogRow += 1
                    if str(key).lower() in CompanyChkLst:
                        t5 = QtGui.QLabel(value)
                        self.la.addWidget(t5, dialogRow, 0)
                        self.cb5 = QtGui.QCheckBox(updateCb)
                        self.cb5.setObjectName(key)
                        if hasattr(self.cb5, "setCheckState"):
                            self.cb5.setCheckState(QtCore.Qt.Checked)
                        else:
                            self.cb5.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb5, dialogRow, 1)
                        u5 = QtGui.QLabel("")
                        u5.setPixmap(pix.scaled(32, 32))
                        self.la.addWidget(u5, dialogRow, 2)
                        self.s5 = QtGui.QLineEdit()
                        self.la.addWidget(self.s5, dialogRow, 3)
                        self.s5.setText(App.ActiveDocument.Company)
                        self.cb5.setObjectName(key)
                        keyLst.append(self.cb5.objectName())
                        self.checkBoxList.append(self.cb5)
                        self.lineTextList.append(self.s5)
                        self.cb5.clicked.connect(self.on_cb5_clicked)
                        longestText = max(longestText, len(App.ActiveDocument.Company))
                        dialogRow += 1
                    if str(key).lower() in LicenseChkLst:
                        t6 = QtGui.QLabel(value)
                        self.la.addWidget(t6, dialogRow, 0)
                        self.cb6 = QtGui.QCheckBox(updateCb)
                        self.cb6.setObjectName(key)
                        if hasattr(self.cb6, "setCheckState"):
                            self.cb6.setCheckState(QtCore.Qt.Checked)
                        else:
                            self.cb6.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb6, dialogRow, 1)
                        u6 = QtGui.QLabel("")
                        u6.setPixmap(pix.scaled(32, 32))
                        self.la.addWidget(u6, dialogRow, 2)
                        self.s6 = QtGui.QLineEdit()
                        self.la.addWidget(self.s6, dialogRow, 3)
                        self.s6.setText(App.ActiveDocument.License)
                        self.cb6.setObjectName(key)
                        keyLst.append(self.cb6.objectName())
                        self.checkBoxList.append(self.cb6)
                        self.lineTextList.append(self.s6)
                        self.cb6.clicked.connect(self.on_cb6_clicked)
                        longestText = max(longestText, len(App.ActiveDocument.License))
                        dialogRow += 1
                    if str(key).lower() in LastModifiedDateChkLst:
                        t7 = QtGui.QLabel(value)
                        self.la.addWidget(t7, dialogRow, 0)
                        self.cb7 = QtGui.QCheckBox(updateCb)
                        self.cb7.setObjectName(key)
                        if hasattr(self.cb7, "setCheckState"):
                            self.cb7.setCheckState(QtCore.Qt.Checked)
                        else:
                            self.cb7.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb7, dialogRow, 1)
                        u7 = QtGui.QLabel("")
                        u7.setPixmap(pix.scaled(32, 32))
                        self.la.addWidget(u7, dialogRow, 2)
                        self.s7 = QtGui.QLineEdit()
                        self.la.addWidget(self.s7, dialogRow, 3)
                        self.cb7.setObjectName(key)
                        keyLst.append(self.cb7.objectName())
                        self.checkBoxList.append(self.cb7)
                        self.lineTextList.append(self.s7)
                        self.cb7.clicked.connect(self.on_cb7_clicked)
                        dialogRow += 1
                        try:
                            dt = datetime.datetime.strptime(
                                App.ActiveDocument.LastModifiedDate,
                                "%Y-%m-%dT%H:%M:%SZ",
                            )
                            if value == "MM/DD/YYYY":
                                self.s7.setText(
                                    "{0}/{1}/{2}".format(dt.month, dt.day, dt.year)
                                )
                            elif value == "YYYY-MM-DD":
                                self.s7.setText(
                                    "{0}-{1}-{2}".format(dt.year, dt.month, dt.day)
                                )
                            else:
                                self.s7.setText(
                                    "{0}/{1}/{2}".format(dt.day, dt.month, dt.year)
                                )
                        except ValueError:
                            App.Console.PrintLog(
                                "DateTime format not recognised: "
                                + App.ActiveDocument.LastModifiedDate
                                + "\n"
                            )
                            self.s7.setText(
                                "{0}/{1}/{2}".format(
                                    date.today().day,
                                    date.today().month,
                                    date.today().year,
                                )
                            )
                    if str(key).lower() in CreatedDateChkLst:
                        t8 = QtGui.QLabel(value)
                        self.la.addWidget(t8, dialogRow, 0)
                        self.cb8 = QtGui.QCheckBox(updateCb)
                        self.cb8.setObjectName(key)
                        if hasattr(self.cb8, "setCheckState"):
                            self.cb8.setCheckState(QtCore.Qt.Checked)
                        else:
                            self.cb8.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb8, dialogRow, 1)
                        u8 = QtGui.QLabel("")
                        u8.setPixmap(pix.scaled(32, 32))
                        self.la.addWidget(u8, dialogRow, 2)
                        self.s8 = QtGui.QLineEdit()
                        self.la.addWidget(self.s8, dialogRow, 3)
                        self.cb8.setObjectName(key)
                        keyLst.append(self.cb8.objectName())
                        self.checkBoxList.append(self.cb8)
                        self.lineTextList.append(self.s8)
                        self.cb8.clicked.connect(self.on_cb8_clicked)
                        dialogRow += 1
                        try:
                            dt = datetime.datetime.strptime(
                                App.ActiveDocument.CreationDate, "%Y-%m-%dT%H:%M:%SZ"
                            )
                            if value == "MM/DD/YYYY":
                                self.s8.setText(
                                    "{0}/{1}/{2}".format(dt.month, dt.day, dt.year)
                                )
                            elif value == "YYYY-MM-DD":
                                self.s8.setText(
                                    "{0}-{1}-{2}".format(dt.year, dt.month, dt.day)
                                )
                            else:
                                self.s8.setText(
                                    "{0}/{1}/{2}".format(dt.day, dt.month, dt.year)
                                )
                        except ValueError:
                            App.Console.PrintLog(
                                "DateTime format not recognised: "
                                + App.ActiveDocument.LastModifiedDate
                                + "\n"
                            )
                            self.s8.setText(
                                "{0}/{1}/{2}".format(
                                    date.today().day,
                                    date.today().month,
                                    date.today().year,
                                )
                            )
                App.Console.PrintLog("\n")
                if len(keyLst) > 1:
                    self.cbAll = QtGui.QCheckBox(
                        QtCore.QT_TRANSLATE_NOOP(
                            "TechDraw_FillTemplateFields",
                            "Update All",
                        )
                    )
                    if hasattr(self.cbAll, "setCheckState"):
                        self.cbAll.setCheckState(QtCore.Qt.Checked)
                    else:
                        self.cbAll.setChecked(QtCore.Qt.Checked)
                    self.la.addWidget(self.cbAll, dialogRow, 1)
                    self.cbAll.clicked.connect(self.on_cbAll_clicked)

                if len(keyLst) > 0:
                    self.okbox = QtGui.QDialogButtonBox(self.dialog)
                    self.okbox.setOrientation(QtCore.Qt.Horizontal)

                    self.okbox.setStandardButtons(
                        QtGui.QDialogButtonBox.Cancel | QtGui.QDialogButtonBox.Ok
                    )
                    self.la.addWidget(self.okbox)
                    QtCore.QObject.connect(
                        self.okbox, QtCore.SIGNAL("accepted()"), self.proceed
                    )
                    QtCore.QObject.connect(
                        self.okbox, QtCore.SIGNAL("rejected()"), self.close
                    )
                    self.okbox.button(QtGui.QDialogButtonBox.Ok).setText("&OK")
                    self.okbox.button(QtGui.QDialogButtonBox.Cancel).setText("&Cancel")
                    self.button = self.okbox.button(QtGui.QDialogButtonBox.Ok)
                    self.button.setEnabled(True)
                    self.dialog.resize(600 + longestText, dialogRow * 50 + 75)
                    self.dialog.move(400, 200 * (400 / (dialogRow * 50 + 75)))
                    self.dialog.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)
                    QtCore.QMetaObject.connectSlotsByName(self.dialog)
                    self.dialog.show()
                    self.dialog.exec_()

                # App.setActiveTransaction("Fill template fields")
                else:
                    msgBox = QtGui.QMessageBox()
                    msgTitle = QtCore.QT_TRANSLATE_NOOP(
                        "Techdraw_FillTemplateFields",
                        "Corresponding template fields missing",
                    )
                    msg = QtCore.QT_TRANSLATE_NOOP(
                        "Techdraw_FillTemplateFields",
                        "There were no corresponding fields found in "
                        + self.page.Label,
                    )
                    msgBox.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)
                    msgBox.setText(msg)
                    msgBox.setWindowTitle(msgTitle)
                    msgBox.move(400, 450)
                    msgBox.exec_()

    def on_cbAll_clicked(self):
        if self.cbAll.isChecked():
            for cbEach in self.checkBoxList:
                if hasattr(cbEach, "setCheckState"):
                    cbEach.setCheckState(QtCore.Qt.Checked)
                else:
                    cbEach.setChecked(QtCore.Qt.Checked)
            self.button.setEnabled(True)
        else:
            for cbEach in self.checkBoxList:
                if hasattr(cbEach, "setCheckState"):
                    cbEach.setCheckState(QtCore.Qt.Unchecked)
                else:
                    cbEach.setChecked(QtCore.Qt.Unchecked)
            self.button.setEnabled(False)

    def on_cb1_clicked(self):
        if self.cb1.isChecked():
            self.button.setEnabled(True)
        else:
            self.chkEachChkBox()

    def on_cb2_clicked(self):
        if self.cb2.isChecked():
            self.button.setEnabled(True)
        else:
            self.chkEachChkBox()

    def on_cb3_clicked(self):
        if self.cb3.isChecked():
            self.button.setEnabled(True)
        else:
            self.chkEachChkBox()

    def on_cb4_clicked(self):
        if self.cb4.isChecked():
            self.button.setEnabled(True)
        else:
            self.chkEachChkBox()

    def on_cb5_clicked(self):
        if self.cb5.isChecked():
            self.button.setEnabled(True)
        else:
            self.chkEachChkBox()

    def on_cb6_clicked(self):
        if self.cb6.isChecked():
            self.button.setEnabled(True)
        else:
            self.chkEachChkBox()

    def on_cb7_clicked(self):
        if self.cb7.isChecked():
            self.button.setEnabled(True)
        else:
            self.chkEachChkBox()

    def on_cb8_clicked(self):
        if self.cb8.isChecked():
            self.button.setEnabled(True)
        else:
            self.chkEachChkBox()

    def chkEachChkBox(self):
        boolFnd = False
        for cbEach in self.checkBoxList:
            if cbEach.isChecked():
                boolFnd = True
        if boolFnd:
            self.button.setEnabled(True)
        else:
            self.button.setEnabled(False)

    def proceed(self):
        transactionName = QtCore.QT_TRANSLATE_NOOP(
            "Techdraw_FillTemplateFields", "Fill template fields"
        )
        App.setActiveTransaction(transactionName)
        i = 0
        for cb in self.checkBoxList:
            if cb.isChecked():
                self.texts[keyLst[i]] = self.lineTextList[i].text()
            i += 1
        self.page.Template.EditableTexts = self.texts
        App.closeActiveTransaction(False)
        self.close()

        App.closeActiveTransaction()

    def close(self):
        self.dialog.hide()
        keyLst.clear()
        return True
