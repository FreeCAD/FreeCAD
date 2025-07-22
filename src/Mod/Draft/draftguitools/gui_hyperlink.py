# ***************************************************************************
# *   Copyright (c) 2023 <https://www.freecad.org>                          *
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

"""Provides GUI tools to open hyperlinks to internal/external documents."""

## @package gui_hyperlink
# \ingroup draftguitools
# \brief Provides GUI tools to open hyperlinks to internal/external documents

## \addtogroup draftguitools
# @{

import FreeCAD
import os
import re
from draftutils.messages import _msg, _toolmsg

if FreeCAD.GuiUp:
    import FreeCADGui
    import Draft_rc
    from PySide.QtCore import QUrl
    from PySide.QtGui import QDesktopServices
def QT_TRANSLATE_NOOP(ctx,txt):
    return txt
translate = FreeCAD.Qt.translate

from PySide import QtWidgets

__title__ = "FreeCAD Draft Workbench GUI Tools - Hyperlinks tools"
__author__ = ("")
__url__ = "https://www.freecad.org"


class Draft_Hyperlink:
    """The Draft_Hyperlink FreeCAD command definition."""

    def GetResources(self):
        d = {'Pixmap': '',
             'Accel': "",
             'MenuText': QT_TRANSLATE_NOOP("Draft_Hyperlink", "Open hyperlinks"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_Hyperlink", "Open linked documents")}
        return d

    def Activated(self):
        self.find_hyperlinks()

        ret = None
        if len(self.hyperlinks_list) > 1:
            m = QtWidgets.QMessageBox()
            m.setWindowTitle(translate("draft", "Opening multiple hyperlinks"))
            m.setText(
                translate(
                    "draft",
                    "Multiple hyperlinks found."
                )
            )
            m.setInformativeText(
                translate(
                    "draft",
                    "This may lead to the opening of various windows"
                )
            )
            m.setStandardButtons(m.Ok | m.Cancel)
            ret = m.exec_()

        if len(self.hyperlinks_list) == 1 or ret == m.Ok:
            for hyperlink in self.hyperlinks_list:
                self.open_hyperlink(hyperlink)

    def find_hyperlinks(self):
        self.hyperlinks_list = []

        for o in FreeCADGui.Selection.getCompleteSelection():
            if hasattr(o.Object, "Text"):

                for text in o.Object.Text:
                    hyperlinks = re.findall(r"((\w:[\\/]|%\w+%|\\\\\w+|/\w+|\w{3,5}://)[\w\\/: ]+\.[\S]+)", text)

                    for hyperlink in hyperlinks:
                        self.hyperlinks_list.append(hyperlink[0])

    def has_hyperlinks(self):
        self.find_hyperlinks()

        return len(self.hyperlinks_list) > 0

    def open_hyperlink(self, hyperlink):
        file_hyperlink = len(re.findall(r"^(\w:[\\/]|%\w+%|\\\\\w+|/\w+)", hyperlink)) > 0

        url = None
        if file_hyperlink:
            if not os.path.isfile(hyperlink):
                _msg(translate("draft", "File not found:") + " " + hyperlink)
                return
            url = QUrl.fromLocalFile(hyperlink)
        else:
            url = QUrl(hyperlink)

        _toolmsg(translate("draft", "Opening hyperlink") + " " + hyperlink)

        QDesktopServices.openUrl(url) #ToDo: add management to open FCStd files in the current instance and to open web pages with the Web Workbench

FreeCADGui.addCommand('Draft_Hyperlink', Draft_Hyperlink())

## @}
