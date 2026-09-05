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

"""This module contains FreeCAD commands for the BIM workbench"""

import os

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


# Status bar buttons


def _get_nudge_tooltip():
    "create nudge tooltip with correct shortcuts"

    nudge_commands = [
        "BIM_Nudge_Up",
        "BIM_Nudge_Down",
        "BIM_Nudge_Left",
        "BIM_Nudge_Right",
        "BIM_Nudge_RotateLeft",
        "BIM_Nudge_RotateRight",
        "BIM_Nudge_Extend",
        "BIM_Nudge_Shrink",
        "BIM_Nudge_Switch",
    ]
    shortcuts = []
    for nudge_command in nudge_commands:
        shortcut = FreeCADGui.CommandAction(nudge_command).getCommand().getShortcut()
        shortcuts.append(shortcut if shortcut else "?")

    #: BIM: tooltip for the status bar nudge widget, %1-%9 are keyboard shortcuts
    tooltip = translate(
        "BIM",
        "The value of the nudge movement (rotation is always 45°).\n"
        "Nudge shortcuts:\n"
        "%1 to move up, %2 to move down.\n"
        "%3 to move left, %4 to move right.\n"
        "%5 to rotate left, %6 to rotate right.\n"
        "%7 to extend height, %8 to shrink height.\n"
        "%9 to switch between auto and manual mode.",
    )
    for index, shortcut in enumerate(shortcuts, start=1):
        tooltip = tooltip.replace(f"%{index}", shortcut)
    return tooltip


def setStatusIcons(show=True):
    "shows or hides the BIM icons in the status bar"

    import FreeCADGui
    from PySide import QtCore, QtGui
    from bimcommands import BimNudge

    nudgeLabels = (
        [translate("BIM", "Custom…")]
        + BimNudge._NUDGE_DISTANCES_STRINGS
        + [translate("BIM", "Auto")]
    )

    def toggleBimViews(state):
        FreeCADGui.runCommand("BIM_Views")

    def toggleBackground(state):
        FreeCADGui.runCommand("BIM_Background")

    def setNudge(action):
        utext = action.text().replace("&", "")
        if utext == nudgeLabels[0]:
            # load dialog
            form = FreeCADGui.PySideUic.loadUi(":/ui/dialogNudgeValue.ui")
            # center the dialog over FreeCAD window
            mw = FreeCADGui.getMainWindow()
            form.move(mw.frameGeometry().topLeft() + mw.rect().center() - form.rect().center())
            form.spinBox.setProperty("rawValue", BimNudge.BIM_Nudge().getNudgeValue("dist"))
            result = form.exec_()
            if not result:
                return
            utext = form.spinBox.text()
        action.parent().parent().parent().setText(utext)

    # main code

    mw = FreeCADGui.getMainWindow()
    if mw:
        st = mw.statusBar()
        statuswidget = st.findChild(QtGui.QToolBar, "BIMStatusWidget")
        if show:
            if statuswidget:
                statuswidget.show()
                if hasattr(statuswidget, "propertybuttons"):
                    statuswidget.propertybuttons.show()
            else:
                statuswidget = FreeCADGui.UiLoader().createWidget("Gui::ToolBar")
                statuswidget.setObjectName("BIMStatusWidget")
                text = translate(
                    "BIMStatusWidget",
                    "BIM Status Widget",
                    "A context menu action used to show or hide this toolbar widget",
                )
                statuswidget.setIconSize(QtCore.QSize(16, 16))
                # MainWindow owns placement/ordering/persistence/menu; we only register.
                mw.addStatusBarItem(
                    statuswidget,
                    id="BIMStatusWidget",
                    title=text,
                    slot="Right",
                    # Workbench band (550-699): just left of the Bottom Panel Toggle.
                    order=570,
                )

                # bim views widget toggle button
                from bimcommands import BimViews

                bimviewsbutton = QtGui.QAction()
                bimviewsbutton.setIcon(QtGui.QIcon(":/icons/BIM_Views.svg"))

                bimviewsbutton.setText("")
                bimviewsbutton.setToolTip(translate("BIM", "Toggles the BIM Views Manager on/off"))
                bimviewsbutton.setCheckable(True)
                if BimViews.findWidget():
                    bimviewsbutton.setChecked(True)
                statuswidget.bimviewsbutton = bimviewsbutton
                bimviewsbutton.triggered.connect(toggleBimViews)
                statuswidget.addAction(bimviewsbutton)

                # background toggle button
                bgbutton = QtGui.QAction()
                # bwidth = bgbutton.fontMetrics().boundingRect("AAAA").width()
                # bgbutton.setMaximumWidth(bwidth)
                bgbutton.setIcon(QtGui.QIcon(":/icons/BIM_Background.svg"))
                bgbutton.setText("")
                bgbutton.setToolTip(
                    translate("BIM", "Toggles the 3D View background between simple and gradient")
                )
                statuswidget.bgbutton = bgbutton
                bgbutton.triggered.connect(toggleBackground)
                statuswidget.addAction(bgbutton)

                # ifc widgets
                try:
                    from nativeifc import ifc_status
                except:
                    pass
                else:
                    ifc_status.set_status_widget(statuswidget)

                # nudge button
                nudge = QtGui.QPushButton(nudgeLabels[-1])
                nudge.setIcon(QtGui.QIcon(":/icons/BIM_Nudge.svg"))
                nudge.setFlat(True)
                nudge.setToolTip(_get_nudge_tooltip())
                statuswidget.addWidget(nudge)
                statuswidget.nudge = nudge
                menu = QtGui.QMenu(nudge)
                gnudge = QtGui.QActionGroup(menu)
                for u in nudgeLabels:
                    a = QtGui.QAction(gnudge)
                    a.setText(u)
                    menu.addAction(a)
                nudge.setMenu(menu)
                gnudge.triggered.connect(setNudge)
                statuswidget.show()

        else:
            if statuswidget is None:
                # when switching workbenches, the toolbar sometimes "jumps"
                # out of the status bar to any other dock area...
                statuswidget = mw.findChild(QtGui.QToolBar, "BIMStatusWidget")
            if statuswidget:
                statuswidget.hide()
                statuswidget.toggleViewAction().setVisible(False)
                if hasattr(statuswidget, "propertybuttons"):
                    statuswidget.propertybuttons.hide()
