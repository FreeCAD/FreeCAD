# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""The BIM TogglePanels command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class BIM_TogglePanels:

    def GetResources(self):
        return {
            "Pixmap": "BIM_TogglePanels",
            "MenuText": QT_TRANSLATE_NOOP("BIM_TogglePanels", "Toggle Bottom Panels"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_TogglePanels", "Toggles bottom dock panels on/off"
            ),
            "Accel": "Ctrl+0",
        }

    def Activated(self):
        from PySide import QtCore, QtGui

        mw = FreeCADGui.getMainWindow()
        togglebutton = None
        st = mw.statusBar()
        statuswidget = st.findChild(QtGui.QToolBar, "BIMStatusWidget")
        if statuswidget:
            if hasattr(statuswidget, "togglebutton"):
                togglebutton = statuswidget.togglebutton
        dockwidgets = mw.findChildren(QtGui.QDockWidget)
        bottomwidgets = [
            w
            for w in dockwidgets
            if (
                (mw.dockWidgetArea(w) == QtCore.Qt.BottomDockWidgetArea)
                and w.isVisible()
            )
        ]
        if bottomwidgets:
            hidden = ""
            for w in bottomwidgets:
                w.hide()
                hidden += w.objectName() + ";;"
                PARAMS.SetString("HiddenWidgets", hidden)
            if togglebutton:
                togglebutton.setChecked(False)
        else:
            widgets = PARAMS.GetString("HiddenWidgets",
                "Python console;;Report view;;Selection view;;"
            )
            widgets = [mw.findChild(QtGui.QWidget, w) for w in widgets.split(";;") if w]
            for w in widgets:
                w.show()
            if togglebutton:
                togglebutton.setChecked(True)


FreeCADGui.addCommand("BIM_TogglePanels", BIM_TogglePanels())
