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

__title__ = "FreeCAD FEM solver report"
__author__ = "Markus Hovorka"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import FreeCAD as App


INFO = 10
WARNING = 20
ERROR = 30


def display(report, title=None, text=None):
    if App.GuiUp:
        displayGui(report, title, text)
    else:
        displayLog(report)


def displayGui(report, title=None, text=None):
    import FreeCADGui as Gui
    from . import reportdialog
    if not report.isEmpty():
        mw = Gui.getMainWindow()
        dialog = reportdialog.ReportDialog(
            report, title, text, mw)
        dialog.exec_()


def displayLog(report):
    for i in report.infos:
        App.Console.PrintLog("%s\n" % i)
    for w in report.warnings:
        App.Console.PrintWarning("%s\n" % w)
    for e in report.errors:
        App.Console.PrintError("%s\n" % e)


class Report(object):

    def __init__(self):
        self.infos = []
        self.warnings = []
        self.errors = []

    def extend(self, report):
        self.infos.extend(report.infos)
        self.warnings.extend(report.warnings)
        self.errors.extend(report.errors)

    def getLevel(self):
        if self.errors:
            return ERROR
        if self.warnings:
            return WARNING
        if self.infos:
            return INFO
        return None

    def isEmpty(self):
        return not (self.infos or self.warnings or self.errors)

    def info(self, msg):
        self.infos.append(msg)

    def warning(self, msg):
        self.warnings.append(msg)

    def error(self, msg):
        self.errors.append(msg)

##  @}
