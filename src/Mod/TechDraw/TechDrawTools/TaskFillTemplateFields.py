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
        objs = App.ActiveDocument.Objects
        for obj in objs:
            if (
                obj.TypeId == "TechDraw::DrawPage"
                and os.path.exists(file_path)
                and listofkeys == reader.fieldnames
            ):
                self.page = obj
                if obj.Views == []:
                    msgBox = QtGui.QMessageBox()
                    msgTitle = QtCore.QT_TRANSLATE_NOOP(
                        "Techdraw_FillTemplateFields",
                        "View or Projection Group missing",
                    )
                    msg = QtCore.QT_TRANSLATE_NOOP(
                        "Techdraw_FillTemplateFields",
                        "There must be a view or projection group to"
                        " establish data for the scale field in " + self.page.Label,
                    )
                    msgBox.setText(msg)
                    msgBox.setWindowTitle(msgTitle)
                    msgBox.exec_()
                    break

                projgrp_view = None
                for pageObj in obj.Views:
                    if pageObj.isDerivedFrom("TechDraw::DrawViewPart"):
                        projgrp_view = self.page.Views[0]
                    elif pageObj.isDerivedFrom("TechDraw::DrawProjGroup"):
                        projgrp_view = self.page.Views[0]

                self.texts = self.page.Template.EditableTexts

                self.dialog = None
                self.s1 = None

                self.dialog = QtGui.QDialog()
                self.dialog.resize(1050, 400)
                self.dialog.setWindowTitle(
                    QtCore.QT_TRANSLATE_NOOP(
                        "TechDraw_FillTemplateFields", "Fill Template Fields in "
                    )
                    + self.page.Label
                )
                self.dialog.move(400, 200)
                self.la = QtGui.QGridLayout(self.dialog)
                updateCb = QtCore.QT_TRANSLATE_NOOP(
                    "TechDraw_FillTemplateFields", "Update"
                )
                updateTxt = (
                    "                                                       ==>>"
                )
                self.checkBoxList = []
                self.lineTextList = []
                for key, value in self.texts.items():
                    App.Console.PrintLog("{0} = {1} | ".format(key, value))
                    if str(key).lower() in CreatedByChkLst:
                        t1 = QtGui.QLabel(value)
                        self.la.addWidget(t1, 0, 0)
                        self.cb1 = QtGui.QCheckBox(updateCb)
                        self.cb1.setObjectName(key)
                        self.cb1.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb1, 0, 1)
                        u1 = QtGui.QLabel(updateTxt)
                        self.la.addWidget(u1, 0, 2)
                        self.s1 = QtGui.QLineEdit()
                        self.s1.setText(App.ActiveDocument.CreatedBy)
                        self.la.addWidget(self.s1, 0, 3)
                        self.cb1.setObjectName(key)
                        keyLst.append(self.cb1.objectName())
                        self.checkBoxList.append(self.cb1)
                        self.lineTextList.append(self.s1)
                        self.cb1.clicked.connect(self.on_cb1_clicked)
                    if str(key).lower() in ScaleChkLst and projgrp_view:
                        t2 = QtGui.QLabel(value)
                        self.la.addWidget(t2, 1, 0)
                        self.cb2 = QtGui.QCheckBox(updateCb)
                        self.cb2.setObjectName(key)
                        self.cb2.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb2, 1, 1)
                        u2 = QtGui.QLabel(updateTxt)
                        self.la.addWidget(u2, 1, 2)
                        self.s2 = QtGui.QLineEdit()
                        self.la.addWidget(self.s2, 1, 3)
                        self.cb2.setObjectName(key)
                        keyLst.append(self.cb2.objectName())
                        self.checkBoxList.append(self.cb2)
                        self.lineTextList.append(self.s2)
                        self.cb2.clicked.connect(self.on_cb2_clicked)
                        if projgrp_view.Scale < 1:
                            self.s2.setText("1 : " + str(int(1 / projgrp_view.Scale)))
                        else:
                            self.s2.setText(str(int(projgrp_view.Scale)) + " : 1")
                    if str(key).lower() in LabelChkLst:
                        t3 = QtGui.QLabel(value)
                        self.la.addWidget(t3, 2, 0)
                        self.cb3 = QtGui.QCheckBox(updateCb)
                        self.cb3.setObjectName(key)
                        self.cb3.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb3, 2, 1)
                        u3 = QtGui.QLabel(updateTxt)
                        self.la.addWidget(u3, 2, 2)
                        self.s3 = QtGui.QLineEdit()
                        self.la.addWidget(self.s3, 2, 3)
                        self.s3.setText(App.ActiveDocument.Label)
                        self.cb3.setObjectName(key)
                        keyLst.append(self.cb3.objectName())
                        self.checkBoxList.append(self.cb3)
                        self.lineTextList.append(self.s3)
                        self.cb3.clicked.connect(self.on_cb3_clicked)
                    if str(key).lower() in CommentChkLst:
                        t4 = QtGui.QLabel(value)
                        self.la.addWidget(t4, 3, 0)
                        self.cb4 = QtGui.QCheckBox(updateCb)
                        self.cb4.setObjectName(key)
                        self.cb4.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb4, 3, 1)
                        u4 = QtGui.QLabel(updateTxt)
                        self.la.addWidget(u4, 3, 2)
                        self.s4 = QtGui.QLineEdit()
                        self.la.addWidget(self.s4, 3, 3)
                        self.s4.setText(App.ActiveDocument.Comment)
                        self.cb4.setObjectName(key)
                        keyLst.append(self.cb4.objectName())
                        self.checkBoxList.append(self.cb4)
                        self.lineTextList.append(self.s4)
                        self.cb4.clicked.connect(self.on_cb4_clicked)
                    if str(key).lower() in CompanyChkLst:
                        t5 = QtGui.QLabel(value)
                        self.la.addWidget(t5, 4, 0)
                        self.cb5 = QtGui.QCheckBox(updateCb)
                        self.cb5.setObjectName(key)
                        self.cb5.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb5, 4, 1)
                        u5 = QtGui.QLabel(updateTxt)
                        self.la.addWidget(u5, 4, 2)
                        self.s5 = QtGui.QLineEdit()
                        self.la.addWidget(self.s5, 4, 3)
                        self.s5.setText(App.ActiveDocument.Company)
                        self.cb5.setObjectName(key)
                        keyLst.append(self.cb5.objectName())
                        self.checkBoxList.append(self.cb5)
                        self.lineTextList.append(self.s5)
                        self.cb5.clicked.connect(self.on_cb5_clicked)
                    if str(key).lower() in LicenseChkLst:
                        t6 = QtGui.QLabel(value)
                        self.la.addWidget(t6, 5, 0)
                        self.cb6 = QtGui.QCheckBox(updateCb)
                        self.cb6.setObjectName(key)
                        self.cb6.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb6, 5, 1)
                        u6 = QtGui.QLabel(updateTxt)
                        self.la.addWidget(u6, 5, 2)
                        self.s6 = QtGui.QLineEdit()
                        self.la.addWidget(self.s6, 5, 3)
                        self.s6.setText(App.ActiveDocument.License)
                        self.cb6.setObjectName(key)
                        keyLst.append(self.cb6.objectName())
                        self.checkBoxList.append(self.cb6)
                        self.lineTextList.append(self.s6)
                        self.cb6.clicked.connect(self.on_cb6_clicked)
                    if str(key).lower() in LastModifiedDateChkLst:
                        t7 = QtGui.QLabel(value)
                        self.la.addWidget(t7, 6, 0)
                        self.cb7 = QtGui.QCheckBox(updateCb)
                        self.cb7.setObjectName(key)
                        self.cb7.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb7, 6, 1)
                        u7 = QtGui.QLabel(updateTxt)
                        self.la.addWidget(u7, 6, 2)
                        self.s7 = QtGui.QLineEdit()
                        self.la.addWidget(self.s7, 6, 3)
                        self.cb7.setObjectName(key)
                        keyLst.append(self.cb7.objectName())
                        self.checkBoxList.append(self.cb7)
                        self.lineTextList.append(self.s7)
                        self.cb7.clicked.connect(self.on_cb7_clicked)
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
                        self.la.addWidget(t8, 7, 0)
                        self.cb8 = QtGui.QCheckBox(updateCb)
                        self.cb8.setObjectName(key)
                        self.cb8.setChecked(QtCore.Qt.Checked)
                        self.la.addWidget(self.cb8, 7, 1)
                        u8 = QtGui.QLabel(updateTxt)
                        self.la.addWidget(u8, 7, 2)
                        self.s8 = QtGui.QLineEdit()
                        self.la.addWidget(self.s8, 7, 3)
                        self.cb8.setObjectName(key)
                        keyLst.append(self.cb8.objectName())
                        self.checkBoxList.append(self.cb8)
                        self.lineTextList.append(self.s8)
                        self.cb8.clicked.connect(self.on_cb8_clicked)
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
                if len(keyLst) > 1:
                    self.cbAll = QtGui.QCheckBox(
                        QtCore.QT_TRANSLATE_NOOP(
                            "TechDraw_FillTemplateFields",
                            "Update All",
                        )
                    )
                    self.cbAll.setChecked(QtCore.Qt.Checked)
                    self.la.addWidget(self.cbAll, 8, 1)
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
                    self.button = self.okbox.button(QtGui.QDialogButtonBox.Ok)
                    self.button.setEnabled(True)
                    QtCore.QMetaObject.connectSlotsByName(self.dialog)
                    self.dialog.show()
                    self.dialog.exec_()
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
                    msgBox.setText(msg)
                    msgBox.setWindowTitle(msgTitle)
                    msgBox.exec_()

    def on_cbAll_clicked(self):
        if self.cbAll.isChecked():
            for cbEach in self.checkBoxList:
                cbEach.setChecked(QtCore.Qt.Checked)
            self.button.setEnabled(True)
        else:
            for cbEach in self.checkBoxList:
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
        i = 0
        for cb in self.checkBoxList:
            if cb.isChecked():
                self.texts[keyLst[i]] = self.lineTextList[i].text()
            i += 1
        self.page.Template.EditableTexts = self.texts
        self.close()

    def close(self):
        self.dialog.hide()
        return True

