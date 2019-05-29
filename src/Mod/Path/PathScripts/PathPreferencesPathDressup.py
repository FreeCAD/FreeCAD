# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import FreeCADGui
import PathScripts.PathPreferences as PathPreferences

from PySide import QtCore, QtGui

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

_dressups = []

def RegisterDressup(dressup):
    _dressups.append(dressup)

class DressupPreferencesPage:
    def __init__(self, parent=None):
        self.form = QtGui.QToolBox()
        self.form.setWindowTitle(translate("Path_PreferencesPathDressup", 'Dressups'))
        pages = []
        for dressup in _dressups:
            page = dressup.preferencesPage()
            if hasattr(page, 'icon') and page.icon:
                self.form.addItem(page.form, page.icon, page.label)
            else:
                self.form.addItem(page.form, page.label)
            pages.append(page)
        self.pages = pages

    def saveSettings(self):
        for page in self.pages:
            page.saveSettings()

    def loadSettings(self):
        for page in self.pages:
            page.loadSettings()

