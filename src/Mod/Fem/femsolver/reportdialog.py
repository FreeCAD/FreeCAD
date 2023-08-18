# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM solver report dialog"
__author__ = "Markus Hovorka"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

# it is a Gui only module and should only be imported in Gui mode
# thus no guard is needed
from PySide import QtGui


ERROR_COLOR = "red"
WARNING_COLOR = "#ffaa00"
INFO_COLOR = "blue"


class ReportDialog(QtGui.QDialog):

    def __init__(self, report, title="Report", text=None, parent=None):
        super(ReportDialog, self).__init__(parent)
        msgDetails = QtGui.QTextEdit()
        msgDetails.setReadOnly(True)
        msgDetails.setHtml(self._getText(report))
        bttBox = QtGui.QDialogButtonBox(QtGui.QDialogButtonBox.Ok)
        bttBox.accepted.connect(self.close)
        layout = QtGui.QVBoxLayout()
        if text is not None:
            textLbl = QtGui.QLabel(text)
            textLbl.setWordWrap(True)
            layout.addWidget(textLbl)
        layout.addWidget(msgDetails)
        layout.addWidget(bttBox)
        self.setWindowTitle(title)
        self.setLayout(layout)
        self.resize(300, 200)

    def _getText(self, report):
        text = ""
        for i in report.infos:
            line = "<b>Info:</b> %s" % i
            text += "%s<br>" % self._getColoredLine(line, INFO_COLOR)
        for w in report.warnings:
            line = "<b>Warning:</b> %s" % w
            text += "%s<br>" % self._getColoredLine(line, WARNING_COLOR)
        for e in report.errors:
            line = "<b>Error:</b> %s" % e
            text += "%s<br>" % self._getColoredLine(line, ERROR_COLOR)
        return text

    def _getColoredLine(self, text, color):
        return '<font color="%s">%s</font>' % (color, text)

##  @}
