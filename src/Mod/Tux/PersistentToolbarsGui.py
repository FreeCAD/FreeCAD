# Persistent toolbars for FreeCAD
# Copyright (C) 2016, 2017  triplus @ FreeCAD
#
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

"""Persistent toolbars for FreeCAD."""

import operator
import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore
from PySide import QtGui

conectedToolbars = []
timer = QtCore.QTimer()
mw = Gui.getMainWindow()


def pythonToolbars():
    """Manage Python based toolbar in BIM workbench."""

    active = Gui.activeWorkbench().__class__.__name__

    if active == "BIMWorkbench" and hasattr(Gui, "Snapper"):
        try:
            Gui.Snapper.show()
        except Exception:
            m = "Persistent toolbars: Snapper toolbar not managed.\n"
            App.Console.PrintMessage(m)
    else:
        pass


def isConnected(i):
    """Connect toolbar to onSave function."""

    if i not in conectedToolbars:
        conectedToolbars.append(i)
        i.topLevelChanged.connect(onSave)
    else:
        pass


def onRestore(active):
    """Restore current workbench toolbars position."""

    toolbars = {}
    tb = mw.findChildren(QtGui.QToolBar)
    pUser = App.ParamGet("User parameter:Tux/PersistentToolbars/User")
    pSystem = App.ParamGet("User parameter:Tux/PersistentToolbars/System")

    for i in tb:

        isConnected(i)

        if i.objectName() and not i.isFloating():
            toolbars[i.objectName()] = i
        else:
            pass

    if pUser.GetGroup(active).GetBool("Saved"):
        group = pUser.GetGroup(active)
    elif pSystem.GetGroup(active).GetBool("Saved"):
        group = pSystem.GetGroup(active)
    else:
        group = None

    if group:
        topRestore = group.GetString("Top").split(",")
        leftRestore = group.GetString("Left").split(",")
        rightRestore = group.GetString("Right").split(",")
        bottomRestore = group.GetString("Bottom").split(",")

        # Reduce flickering.
        for i in toolbars:
            if (
                i not in topRestore
                and i not in leftRestore
                and i not in rightRestore
                and i not in bottomRestore
            ):

                area = mw.toolBarArea(toolbars[i])

                if area == QtCore.Qt.ToolBarArea.LeftToolBarArea:
                    leftRestore.append(i)
                elif area == QtCore.Qt.ToolBarArea.RightToolBarArea:
                    rightRestore.append(i)
                elif area == QtCore.Qt.ToolBarArea.BottomToolBarArea:
                    bottomRestore.append(i)
                else:
                    topRestore.append(i)
            else:
                pass

        for i in topRestore:
            if i == "Break":
                mw.addToolBarBreak(QtCore.Qt.TopToolBarArea)
            elif i in toolbars:
                mw.addToolBar(QtCore.Qt.TopToolBarArea, toolbars[i])
            else:
                pass

        for i in leftRestore:
            if i == "Break":
                mw.addToolBarBreak(QtCore.Qt.LeftToolBarArea)
            elif i in toolbars:
                mw.addToolBar(QtCore.Qt.LeftToolBarArea, toolbars[i])
            else:
                pass

        for i in rightRestore:
            if i == "Break":
                mw.addToolBarBreak(QtCore.Qt.RightToolBarArea)
            elif i in toolbars:
                mw.addToolBar(QtCore.Qt.RightToolBarArea, toolbars[i])
            else:
                pass

        for i in bottomRestore:
            if i == "Break":
                mw.addToolBarBreak(QtCore.Qt.BottomToolBarArea)
            elif i in toolbars:
                mw.addToolBar(QtCore.Qt.BottomToolBarArea, toolbars[i])
            else:
                pass
    else:
        pass


def onSave():
    """Save current workbench toolbars position."""

    tb = mw.findChildren(QtGui.QToolBar)
    active = Gui.activeWorkbench().__class__.__name__
    p = App.ParamGet("User parameter:Tux/PersistentToolbars/User")
    group = p.GetGroup(active)

    top = []
    left = []
    right = []
    bottom = []

    for i in tb:
        if i.objectName() and i.isVisible() and not i.isFloating():

            area = mw.toolBarArea(i)

            x = i.geometry().x()
            y = i.geometry().y()
            b = mw.toolBarBreak(i)
            n = i.objectName()

            if area == QtCore.Qt.ToolBarArea.TopToolBarArea:
                top.append([x, y, b, n])
            elif area == QtCore.Qt.ToolBarArea.LeftToolBarArea:
                left.append([x, y, b, n])
            elif area == QtCore.Qt.ToolBarArea.RightToolBarArea:
                right.append([-x, y, b, n])
            elif area == QtCore.Qt.ToolBarArea.BottomToolBarArea:
                bottom.append([x, -y, b, n])
            else:
                pass
        else:
            pass

    top = sorted(top, key=operator.itemgetter(1, 0))
    left = sorted(left, key=operator.itemgetter(0, 1))
    right = sorted(right, key=operator.itemgetter(0, 1))
    bottom = sorted(bottom, key=operator.itemgetter(1, 0))

    topSave = []
    leftSave = []
    rightSave = []
    bottomSave = []

    for i in top:
        if i[2] is True:
            topSave.append("Break")
            topSave.append(i[3])
        else:
            topSave.append(i[3])

    for i in left:
        if i[2] is True:
            leftSave.append("Break")
            leftSave.append(i[3])
        else:
            leftSave.append(i[3])

    for i in right:
        if i[2] is True:
            rightSave.append("Break")
            rightSave.append(i[3])
        else:
            rightSave.append(i[3])

    for i in bottom:
        if i[2] is True:
            bottomSave.append("Break")
            bottomSave.append(i[3])
        else:
            bottomSave.append(i[3])

    group.SetBool("Saved", 1)
    group.SetString("Top", ",".join(topSave))
    group.SetString("Left", ",".join(leftSave))
    group.SetString("Right", ",".join(rightSave))
    group.SetString("Bottom", ",".join(bottomSave))


def onWorkbenchActivated():
    """When workbench gets activated restore toolbar position."""

    active = Gui.activeWorkbench().__class__.__name__

    if active:
        pythonToolbars()
        onRestore(active)
    else:
        pass


def onStart():
    """Start persistent toolbars."""
    if mw.property("eventLoop"):
        start = False
        try:
            mw.mainWindowClosed
            mw.workbenchActivated
            start = True
        except AttributeError:
            pass
        if start:
            timer.stop()
            onWorkbenchActivated()
            mw.mainWindowClosed.connect(onClose)
            mw.workbenchActivated.connect(onWorkbenchActivated)


def onClose():
    """Remove system provided presets on FreeCAD close."""

    p = App.ParamGet("User parameter:Tux/PersistentToolbars")
    p.RemGroup("System")


timer.timeout.connect(onStart)
timer.start(500)
