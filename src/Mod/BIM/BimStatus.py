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


def setStatusIcons(show=True):
    "shows or hides the BIM icons in the status bar"

    import FreeCADGui
    from PySide import QtCore, QtGui

    nudgeLabelsI = [
        translate("BIM", "Custom…"),
        '1/16"',
        '1/8"',
        '1/4"',
        '1"',
        '6"',
        "1'",
        translate("BIM", "Auto"),
    ]
    nudgeLabelsM = [
        translate("BIM", "Custom…"),
        "1 mm",
        "5 mm",
        "1 cm",
        "5 cm",
        "10 cm",
        "50 cm",
        translate("BIM", "Auto"),
    ]

    def toggleBimViews(state):
        FreeCADGui.runCommand("BIM_Views")

    def toggleBackground(state):
        FreeCADGui.runCommand("BIM_Background")

    def setNudge(action):
        utext = action.text().replace("&", "")
        if utext == nudgeLabelsM[0]:
            # load dialog
            form = FreeCADGui.PySideUic.loadUi(":/ui/dialogNudgeValue.ui")
            # center the dialog over FreeCAD window
            mw = FreeCADGui.getMainWindow()
            form.move(mw.frameGeometry().topLeft() + mw.rect().center() - form.rect().center())
            result = form.exec_()
            if not result:
                return
            utext = form.inputField.text()
        action.parent().parent().parent().setText(utext)

    def _insert_before_toggle(st, mw, widget):
        """Place widget into the permanent area, immediately before
        toggleBottomPanelsButton. Falls back to appending if the toggle is
        missing or insertion-by-index produces 'Index out of range' (which can
        happen when permanent widgets are still being added). Implementation
        removes every titled status-bar widget at or after the toggle and re-adds
        them, sandwiching our widget in the right place."""
        toggle = mw.findChild(QtGui.QToolButton, "toggleBottomPanelsButton")
        if not toggle:
            st.addPermanentWidget(widget)
            return
        siblings = [c for c in st.children() if isinstance(c, QtGui.QWidget) and c is not widget]
        siblings.sort(key=lambda w: w.x())
        try:
            anchor = siblings.index(toggle)
        except ValueError:
            st.addPermanentWidget(widget)
            return
        after = siblings[anchor:]
        # Preserve each widget's visibility: removeWidget() hides it and
        # addPermanentWidget() would otherwise reveal user-hidden widgets (and,
        # for widgets that persist visibility, write the preference back to true).
        visibilities = [w.isVisible() for w in after]
        for w in after:
            st.removeWidget(w)
        st.addPermanentWidget(widget)
        for w, was_visible in zip(after, visibilities):
            st.addPermanentWidget(w)
            w.setVisible(was_visible)

    # main code

    mw = FreeCADGui.getMainWindow()
    if mw:
        st = mw.statusBar()
        statuswidget = st.findChild(QtGui.QToolBar, "BIMStatusWidget")
        if show:
            if statuswidget:
                st.removeWidget(statuswidget)
                _insert_before_toggle(st, mw, statuswidget)
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
                statuswidget.setWindowTitle(text)
                statuswidget.setIconSize(QtCore.QSize(16, 16))
                _insert_before_toggle(st, mw, statuswidget)

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
                nudge = QtGui.QPushButton(nudgeLabelsM[-1])
                nudge.setIcon(QtGui.QIcon(":/icons/BIM_Nudge.svg"))
                nudge.setFlat(True)
                nudge.setToolTip(
                    translate(
                        "BIM",
                        "The value of the nudge movement (rotation is always 45°)."
                        "Alt+arrows to move\nAlt+, to rotate left"
                        "Alt+. to rotate right\nAlt+PgUp to extend extrusion"
                        "Alt+PgDown to shrink extrusion"
                        "Alt+/ to switch between auto and manual mode",
                    )
                )
                statuswidget.addWidget(nudge)
                statuswidget.nudge = nudge
                menu = QtGui.QMenu(nudge)
                gnudge = QtGui.QActionGroup(menu)
                for u in nudgeLabelsM:
                    a = QtGui.QAction(gnudge)
                    a.setText(u)
                    menu.addAction(a)
                nudge.setMenu(menu)
                gnudge.triggered.connect(setNudge)
                statuswidget.nudgeLabelsI = nudgeLabelsI
                statuswidget.nudgeLabelsM = nudgeLabelsM
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
